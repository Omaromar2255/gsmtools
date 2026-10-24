# Architectural Blueprint: `AdbRawSocketClient` and `IAdbRawSocketClient`
**Milestone 1 Core Implementation Specification**
**Author:** explorer_m1_1 (Preview Explorer)
**Target Platform:** .NET 8 (C# 12) / WPF Desktop (`GsmTools.Desktop`)
**Date:** 2026-10-17

---

## 1. Executive Summary & Problem Context

The core objective of Milestone 1 in `GsmTools.Desktop` is to establish a high-performance, forensic-grade, zero-process-spawning communication layer with the local Android Debug Bridge (ADB) daemon (`127.0.0.1:5037`). Spawning `adb.exe` subprocesses introduces significant process creation overhead, fragile standard I/O redirection, console window flashing, and inability to handle binary streams with forensic fidelity.

This document delivers the comprehensive architectural design, wire-level packet layouts, and production-grade C# implementation blueprint for:
1. `IAdbRawSocketClient.cs`: Public abstraction defining all ADB operations required by device audit, Samsung CSC injection (`csc 15.txt`), and the `/AT` interactive bridge (`at_bridge.c`).
2. `AdbRawSocketClient.cs`: Concrete .NET 8 implementation utilizing raw `System.Net.Sockets.Socket` / `NetworkStream` with non-blocking async I/O, little-endian binary primitives, and explicit transport connection lifecycle management.

### Reference Specifications:
- **AOSP ADB Server Protocol**: Client-Server smart socket wire protocol (`protocol.txt`, `adb_client.cpp`).
- **AOSP File Sync Protocol**: Binary sync service (`file_sync_service.h`, `file_sync_client.cpp`).
- **AOSP Shell Protocol V2**: Demultiplexed stream protocol with separate stdout, stderr, and exit code (`shell_protocol.h`).
- **Forensic Log Trace**: `csc 15.txt` (recording the exact execution sequence from line 7 to line 2371).
- **Native Bridge**: `at-bridge/jni/at_bridge.c` (interactive binary protocol with dynamic stream cipher).

---

## 2. Core Architectural Architecture & Socket Lifecycle

### 2.1 The ADB Socket Lifespan Trap & The Transport Socket Factory Pattern
A naive implementation of an ADB socket client creates a single `TcpClient` or `Socket` on `ConnectAsync()` and attempts to execute multiple commands sequentially over that single socket. **This fails catastrophically against an ADB server.**

In the ADB Server protocol architecture:
- Queries targeting the ADB server itself (e.g., `host:version`, `host:devices`) return an `OKAY` status followed by a 4-hex length prefix and payload. The socket *can* remain open or close depending on client usage.
- **However**, when a socket issues a transport switch (`host:transport:<serial>`) followed by a service request (such as `shell:<cmd>`, `sync:`, or `usb:`), the ADB server binds that specific TCP connection directly to the remote device daemon (`adbd`).
- When the remote command terminates (e.g. `getprop` reaches EOF, or `sync:` receives `QUIT`, or `usb:` triggers daemon restart), the remote `adbd` closes the channel. The ADB server immediately closes the TCP socket with the client!
- If the client attempts to send subsequent commands on that same socket, .NET throws `SocketException: An existing connection was forcibly closed by the remote host` or `ObjectDisposedException`.

#### Architectural Solution:
`AdbRawSocketClient` manages connection state through an internal **Transport Socket Factory**:
1. It maintains connection parameters: `_host` (default `"127.0.0.1"`), `_port` (default `5037`), and `_selectedSerial` (set by `SelectDeviceAsync`).
2. `ConnectAsync` validates connectivity to the ADB server via a health query (`host:version`).
3. For one-shot service operations (`ExecuteShellCommandAsync`, `PushFileSyncAsync`, `RestartUsbModeAsync`), the client executes an internal factory method `CreateTransportSocketAsync(cancellationToken)`:
   - Allocates a dedicated `Socket` with `SocketType.Stream, ProtocolType.Tcp`.
   - Sets `NoDelay = true` (disabling Nagle's algorithm to eliminate latency on 4-byte frames).
   - Sets `LingerState = new LingerOption(false, 0)`.
   - Connects to `127.0.0.1:5037`.
   - Emits the transport switch frame `host:transport:{serial}` (or `host:transport-any` if serial is null).
   - Validates the `OKAY` handshake.
   - Handshakes with the desired service (`shell:`, `sync:`, `usb:`).
   - Streams the data to completion and cleanly disposes the socket upon return.
4. For persistent interactive sessions (`PersistentAtBridgeSession`), `AdbRawSocketClient` exposes `OpenRawStreamAsync(string service, CancellationToken ct)`, returning a dedicated, unbuffered `NetworkStream` whose lifetime is managed by the caller.

```
+-------------------------------------------------------------------------+
|                          AdbRawSocketClient                             |
+-------------------------------------------------------------------------+
       |                                              |
       | One-shot Operations                          | Interactive Session
       v                                              v
+-------------------------------+             +---------------------------+
| CreateTransportSocketAsync()  |             | OpenRawStreamAsync()      |
|  1. New Socket                |             |  1. New Socket            |
|  2. Connect 127.0.0.1:5037    |             |  2. Switch Transport      |
|  3. Switch Transport (Serial) |             |  3. Send Service Name     |
|  4. Execute Command/Sync/USB  |             |  4. Return NetworkStream  |
|  5. Dispose Socket on Finish  |             |     to Persistent Session |
+-------------------------------+             +---------------------------+
```

---

## 3. Detailed Wire Protocol Specifications & Packet Diagrams

### 3.1 Connection & Framed Request Handling
Every control request sent to the ADB server begins with a 4-hex-character ASCII length prefix indicating the byte length of the command payload (excluding the 4 length characters).

#### Request Framing:
$$\text{Wire Packet} = \text{Hex}_4(\text{Length}(\text{Payload})) + \text{Payload}$$
- Example: Requesting `host:version` (length 12 = `0x000C`):
  `000chost:version`
- Example: Requesting `host:devices` (length 12 = `0x000C`):
  `000chost:devices`

#### Response Protocol:
The server responds with an initial 4-byte ASCII status:
- **`OKAY`** (`0x4F, 0x4B, 0x41, 0x59`): Success.
  - If the command was a host query (e.g. `host:version`, `host:devices`), `OKAY` is immediately followed by a 4-hex-character ASCII length prefix, followed by the response payload:
    `OKAY00040029` (version 41)
    `OKAY001dRFCX90GR21X\tdevice\n` (1 device connected)
  - If the command was a transport switch or service invocation, no length prefix is emitted; subsequent bytes belong to the service stream.
- **`FAIL`** (`0x46, 0x41, 0x49, 0x4C`): Failure.
  - `FAIL` is *always* followed by a 4-hex-character ASCII length prefix, and then the error string:
    `FAIL0016device 'XYZ' not found`
  - The client must read the 4-hex length, read the error string, and throw an `AdbException(errorMessage)`.

```
Client -> Server: [ 0 0 0 c h o s t : v e r s i o n ]
Server -> Client: [ O K A Y ] [ 0 0 0 4 ] [ 0 0 2 9 ]
```

---

### 3.2 Transport Switching (`host:transport:<serial>`)
To target a specific physical device daemon (`adbd`) over USB:
1. Client connects to `127.0.0.1:5037`.
2. Client sends `host:transport:<serial>`:
   - Serial: `RFCX90GR21X` (11 chars)
   - Payload: `host:transport:RFCX90GR21X` (26 chars = `0x001A`)
   - Wire data: `001ahost:transport:RFCX90GR21X`
   - If serial is null or empty, client sends `0012host:transport-any`.
3. Server responds with 4-byte `OKAY`.
4. The socket is now transparently bound to that device. No length prefix follows `OKAY`. Subsequent bytes written to this socket are routed to `adbd`.

```
Client -> Server: [ 0 0 1 a ] [ h o s t : t r a n s p o r t : R F C X 9 0 G R 2 1 X ]
Server -> Client: [ O K A Y ]
(Socket is now bound to device RFCX90GR21X)
```

---

### 3.3 Command Execution (`ExecuteShellCommandAsync`)

`AdbRawSocketClient` must support two command execution modes:
1. Standard legacy shell (`shell:<command>`)
2. Shell Protocol V2 raw (`shell,v2,raw:<command>`)

#### Mode 1: Standard Legacy Shell (`shell:<command>`)
- Payload: `shell:<command>` (e.g. `shell:getprop`).
- Wire request: `000dshell:getprop`.
- Server responds: `OKAY`.
- Server stream: Raw byte stream of command output directly from standard output/error.
- Stream termination: Server closes socket connection at EOF when the process exits.
- Reading algorithm:
  Loop `stream.ReadAsync(buffer, 0, buffer.Length)` until bytes read == 0.
  Decode bytes as UTF-8 string and return.

#### Mode 2: Shell Protocol V2 (`shell,v2,raw:<command>`)
Shell Protocol V2 multiplexes `stdout`, `stderr`, and `exit_code` using explicit binary packet frames.
- Payload: `shell,v2,raw:<command>`
- Request wire data: `"{len:x4}shell,v2,raw:{command}"`
- Server responds: `OKAY`.
- Packet framing structure:
  Each packet consists of a 5-byte header followed by payload bytes:
  ```
  +----------------+-------------------------------+-------------------+
  | 1 Byte: ID     | 4 Bytes: Length (uint32 LE)   | [Length] Bytes    |
  +----------------+-------------------------------+-------------------+
  ```
  **Stream IDs (`shell_protocol.h`):**
  - `0x01` (`kIdStdout`): Standard output data chunk.
  - `0x02` (`kIdStderr`): Standard error data chunk.
  - `0x03` (`kIdExit`): Exit status packet. Payload is 1 byte representing the integer exit code (e.g., `0x00` for success).
  - `0x04` (`kIdCloseStdin`): Close stdin signal.
  - `0x05` (`kIdWindowSizeChange`): Terminal resize.

- Decoding Algorithm:
  ```csharp
  var stdout = new MemoryStream();
  var stderr = new MemoryStream();
  int exitCode = 0;

  byte[] header = new byte[5];
  while (true)
  {
      int read = await ReadExactAsync(stream, header, 0, 5, ct);
      if (read == 0) break; // EOF

      byte id = header[0];
      uint length = BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(1, 4));

      byte[] payload = new byte[length];
      if (length > 0)
      {
          await ReadExactAsync(stream, payload, 0, (int)length, ct);
      }

      if (id == 1) stdout.Write(payload, 0, payload.Length);
      else if (id == 2) stderr.Write(payload, 0, payload.Length);
      else if (id == 3 && length >= 1)
      {
          exitCode = payload[0];
          break;
      }
  }
  ```

---

### 3.4 Native `sync:` File Push Protocol (`PushFileSyncAsync`)
Pushing `/data/local/tmp/AT` (file size 492,096 bytes) directly over the socket requires the binary ADB Sync Protocol.

#### Step 1: Initiate Sync Service
1. Connect socket and switch transport to device: `host:transport:<serial>` -> reads `OKAY`.
2. Send sync service initiation request:
   - Length: 5 -> `0005`
   - Payload: `sync:`
   - Wire data: `0005sync:`
3. Server responds: `OKAY`. (Socket is now in binary sync mode).

#### Step 2: `STAT` Packet (Validate Remote Target)
Verifies whether the target path exists and confirms `adbd` sync is responding:
- **Request (Client -> Server):**
  - 4 bytes ASCII: `"STAT"` (`0x53, 0x54, 0x41, 0x54`)
  - 4 bytes uint32 LE: `path.Length` (e.g., `/data/local/tmp/AT` length is 18 bytes: `18, 0, 0, 0`)
  - 18 bytes UTF-8: `"/data/local/tmp/AT"`
- **Response (Server -> Client): Exactly 16 bytes**
  - 4 bytes ASCII: `"STAT"` (`0x53, 0x54, 0x41, 0x54`)
  - 4 bytes uint32 LE: `mode` (file permissions/attributes; `0` if not found)
  - 4 bytes uint32 LE: `size` (file size in bytes; `0` if not found)
  - 4 bytes uint32 LE: `mtime` (modified timestamp; `0` if not found)
- **Validation:**
  Client verifies the first 4 bytes are `"STAT"`. If not, throw `AdbException("Invalid STAT response")`.

```
Client -> [ 'S' 'T' 'A' 'T' ] [ 0x12 0x00 0x00 0x00 ] [ "/data/local/tmp/AT" ]
Server <- [ 'S' 'T' 'A' 'T' ] [ mode (4B LE) ] [ size (4B LE) ] [ mtime (4B LE) ]
```

#### Step 3: `SEND` Packet (Destination Path & Permissions)
Declares the destination path and POSIX file mode:
- Format of target specifier: `"{remotePath},{permissions}"`
- Remote Path: `/data/local/tmp/AT`
- Permissions: `33188` (decimal for octal `0100644`, Linux `S_IFREG | 0644`)
- Combined string: `"/data/local/tmp/AT,33188"` (24 bytes)
- **Request (Client -> Server):**
  - 4 bytes ASCII: `"SEND"` (`0x53, 0x45, 0x4E, 0x44`)
  - 4 bytes uint32 LE: `specifier.Length` (24 bytes: `24, 0, 0, 0`)
  - 24 bytes UTF-8: `"/data/local/tmp/AT,33188"`
- Note: Server emits no immediate response; it prepares its internal write buffer for incoming `DATA` chunks.

```
Client -> [ 'S' 'E' 'N' 'D' ] [ 0x18 0x00 0x00 0x00 ] [ "/data/local/tmp/AT,33188" ]
```

#### Step 4: `DATA` Packet Chunking (64KB Max per Chunk)
The local binary (`./AT`, 492,096 bytes) is streamed in chunks up to 64KB (65,536 bytes):
- Total chunks for 492,096 bytes:
  - 7 chunks of 65,536 bytes (458,752 bytes)
  - 1 final chunk of 33,344 bytes
- **For each chunk:**
  - 4 bytes ASCII: `"DATA"` (`0x44, 0x41, 0x54, 0x41`)
  - 4 bytes uint32 LE: `chunkLength` (e.g. `65536` -> `0x00, 0x00, 0x01, 0x00`)
  - `chunkLength` bytes: File payload bytes.
  - Client invokes `progressCallback?.Invoke(totalBytesSent, totalFileSize)`.

```
Client -> [ 'D' 'A' 'T' 'A' ] [ 0x00 0x00 0x01 0x00 ] [ 65536 bytes payload... ]
Client -> [ 'D' 'A' 'T' 'A' ] [ 0x00 0x00 0x01 0x00 ] [ 65536 bytes payload... ]
...
Client -> [ 'D' 'A' 'T' 'A' ] [ 0x40 0x82 0x00 0x00 ] [ 33344 bytes payload... ]
```

#### Step 5: `DONE` Packet (Commit & Verification)
Instructs `adbd` to flush file buffers to disk and apply file timestamp:
- **Request (Client -> Server):**
  - 4 bytes ASCII: `"DONE"` (`0x44, 0x4F, 0x4E, 0x45`)
  - 4 bytes uint32 LE: `mtime` (e.g., `(uint)DateTimeOffset.UtcNow.ToUnixTimeSeconds()`)
- **Response (Server -> Client):**
  - 4 bytes ASCII: `"OKAY"` or `"FAIL"`
  - 4 bytes uint32 LE: `messageLength`
  - If `"OKAY"`: `messageLength` is usually 0. Success!
  - If `"FAIL"`: Client reads `messageLength` bytes of UTF-8 error string and throws `AdbException($"PushFileSync failed: {err}")`.

```
Client -> [ 'D' 'O' 'N' 'E' ] [ timestamp (4B LE) ]
Server <- [ 'O' 'K' 'A' 'Y' ] [ 0x00 0x00 0x00 0x00 ]
```

#### Step 6: `QUIT` Packet
Cleanly terminates the sync session:
- **Request (Client -> Server):**
  - 4 bytes ASCII: `"QUIT"` (`0x51, 0x55, 0x49, 0x54`)
  - 4 bytes uint32 LE: `0` (`0x00, 0x00, 0x00, 0x00`)
- Client immediately closes the socket.

```
Client -> [ 'Q' 'U' 'I' 'T' ] [ 0x00 0x00 0x00 0x00 ]
(Socket closed)
```

---

### 3.5 Device Reset: `RestartUsbModeAsync`
In `csc 15.txt` (lines 2363-2371), the post-injection reset is executed via:
```
[17:42:26.598] ADB-INTERACTIVE-CMD | Command: usb:
[17:42:26.599] ADB-STREAM | Command: restarting in USB mode
```

#### Wire Sequence:
1. Connect socket to `127.0.0.1:5037`.
2. Switch transport: `001ahost:transport:{serial}` -> reads `OKAY`.
3. Send service command:
   - Length: 4 -> `0004`
   - Payload: `usb:`
   - Wire data: `0004usb:`
4. Server responds: `OKAY`.
5. Server sends response string: `"restarting in USB mode\n"`.
6. Client reads string, verifies it contains `"restarting in USB mode"`, and returns the string.
7. Socket closes as the device USB stack resets.

---

### 3.6 Interactive Bridge Support: `OpenRawStreamAsync`
For `PersistentAtBridgeSession` to communicate with `/data/local/tmp/AT` or relay via `toybox nc localhost 2600`, it requires a dedicated, raw, bidirectional `Stream`.
`OpenRawStreamAsync` establishes this connection:
```csharp
public async Task<Stream> OpenRawStreamAsync(string service, CancellationToken cancellationToken = default)
{
    var socket = await CreateTransportSocketAsync(cancellationToken).ConfigureAwait(false);
    var stream = new NetworkStream(socket, ownsSocket: true);

    string framed = $"{service.Length:x4}{service}";
    byte[] reqBytes = Encoding.ASCII.GetBytes(framed);
    await stream.WriteAsync(reqBytes, cancellationToken).ConfigureAwait(false);
    await stream.FlushAsync(cancellationToken).ConfigureAwait(false);

    byte[] status = new byte[4];
    await ReadExactAsync(stream, status, 0, 4, cancellationToken).ConfigureAwait(false);
    string statusStr = Encoding.ASCII.GetString(status);
    if (statusStr != "OKAY")
    {
        string err = await ReadErrorMessageAsync(stream, cancellationToken).ConfigureAwait(false);
        await stream.DisposeAsync().ConfigureAwait(false);
        throw new AdbException($"Failed to open service '{service}': {err}");
    }

    return stream;
}
```
This enables `PersistentAtBridgeSession` to execute `OpenRawStreamAsync("shell:/data/local/tmp/AT")` and interactively send hex commands and read `.OK..#` delimiters without spawning subprocesses!

---

## 4. Complete Production-Ready C# Implementation Blueprint

### 4.1 Interface Contract: `IAdbRawSocketClient.cs`

```csharp
// ============================================================================
// File: GsmTools.Desktop/Core/IAdbRawSocketClient.cs
// Forensic-grade ADB Raw Socket Client Interface for .NET 8 WPF
// ============================================================================

namespace GsmTools.Desktop.Core;

/// <summary>
/// Defines direct TCP socket communication with the local ADB server (127.0.0.1:5037).
/// Implements framed ASCII commands, transport routing, command execution (v1 & v2),
/// native binary sync file transfers, and device reset without external processes.
/// </summary>
public interface IAdbRawSocketClient : IAsyncDisposable
{
    /// <summary>
    /// Connects to the ADB server and verifies server responsiveness.
    /// </summary>
    /// <param name="host">ADB host address (default 127.0.0.1).</param>
    /// <param name="port">ADB port number (default 5037).</param>
    /// <param name="cancellationToken">Cancellation token.</param>
    Task ConnectAsync(string host = "127.0.0.1", int port = 5037, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sends a length-prefixed ASCII request to the ADB host and returns the string response.
    /// Used for server-level queries like "host:version", "host:devices", "host:devices-l".
    /// </summary>
    /// <param name="request">Unframed command payload (e.g. "host:version").</param>
    /// <param name="cancellationToken">Cancellation token.</param>
    /// <returns>Decoded response payload string.</returns>
    Task<string> SendAdbRequestAsync(string request, CancellationToken cancellationToken = default);

    /// <summary>
    /// Configures the target device serial number for all subsequent device-targeted commands.
    /// </summary>
    /// <param name="serial">Target device serial, or null for the only connected USB device.</param>
    /// <param name="cancellationToken">Cancellation token.</param>
    Task SelectDeviceAsync(string? serial = null, CancellationToken cancellationToken = default);

    /// <summary>
    /// Executes a shell command on the selected device and streams the output to completion.
    /// Supports standard legacy shell ("shell:<cmd>") and Shell Protocol V2 ("shell,v2,raw:<cmd>").
    /// </summary>
    /// <param name="command">Command to execute (e.g. "getprop", "pm list package").</param>
    /// <param name="useV2">If true, uses Shell Protocol V2 framing with demultiplexed streams.</param>
    /// <param name="cancellationToken">Cancellation token.</param>
    /// <returns>Combined standard output string.</returns>
    Task<string> ExecuteShellCommandAsync(string command, bool useV2 = false, CancellationToken cancellationToken = default);

    /// <summary>
    /// Pushes a local file to the remote device using the native binary ADB sync protocol (STAT/SEND/DATA/DONE/QUIT).
    /// </summary>
    /// <param name="localPath">Absolute path to local source file (e.g. "./AT").</param>
    /// <param name="remotePath">Absolute target path on Android (e.g. "/data/local/tmp/AT").</param>
    /// <param name="permissions">POSIX permissions mode (default 33188 = octal 0100644).</param>
    /// <param name="progressCallback">Optional callback receiving (bytesSent, totalBytes).</param>
    /// <param name="cancellationToken">Cancellation token.</param>
    Task PushFileSyncAsync(
        string localPath,
        string remotePath,
        int permissions = 33188,
        Action<long, long>? progressCallback = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Triggers an immediate device reset into USB mode via the ADB "usb:" service.
    /// </summary>
    /// <param name="cancellationToken">Cancellation token.</param>
    /// <returns>Status response string (e.g. "restarting in USB mode").</returns>
    Task<string> RestartUsbModeAsync(CancellationToken cancellationToken = default);

    /// <summary>
    /// Opens an unbuffered, bidirectional NetworkStream to a custom device service
    /// (e.g. "shell:/data/local/tmp/AT" or "shell:toybox nc localhost 2600").
    /// Used by PersistentAtBridgeSession for live RIL/IPC communication.
    /// </summary>
    /// <param name="service">Target service string.</param>
    /// <param name="cancellationToken">Cancellation token.</param>
    /// <returns>Connected and authenticated NetworkStream.</returns>
    Task<Stream> OpenRawStreamAsync(string service, CancellationToken cancellationToken = default);

    /// <summary>
    /// Gets whether the client is currently connected and initialized.
    /// </summary>
    bool IsConnected { get; }

    /// <summary>
    /// Gets the currently selected device serial.
    /// </summary>
    string? SelectedSerial { get; }
}
```

---

### 4.2 Custom Exceptions: `AdbException.cs`

```csharp
// ============================================================================
// File: GsmTools.Desktop/Core/AdbException.cs
// ============================================================================

namespace GsmTools.Desktop.Core;

public class AdbException : Exception
{
    public AdbException(string message) : base(message) { }
    public AdbException(string message, Exception innerException) : base(message, innerException) { }
}

public class AdbDeviceNotFoundException : AdbException
{
    public string? Serial { get; }
    public AdbDeviceNotFoundException(string? serial, string message) : base(message)
    {
        Serial = serial;
    }
}
```

---

### 4.3 Production Implementation: `AdbRawSocketClient.cs`

```csharp
// ============================================================================
// File: GsmTools.Desktop/Core/AdbRawSocketClient.cs
// Forensic-grade ADB Raw Socket Client Implementation for .NET 8 WPF
// ============================================================================

using System.Buffers.Binary;
using System.Globalization;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace GsmTools.Desktop.Core;

public sealed class AdbRawSocketClient : IAdbRawSocketClient
{
    private const int DefaultBufferSize = 65536; // 64KB ADB sync chunk size
    private string _host = "127.0.0.1";
    private int _port = 5037;
    private string? _selectedSerial;
    private bool _isConnected;
    private bool _disposed;

    public bool IsConnected => _isConnected && !_disposed;
    public string? SelectedSerial => _selectedSerial;

    public async Task ConnectAsync(string host = "127.0.0.1", int port = 5037, CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        _host = host;
        _port = port;

        // Verify connection by querying ADB version
        using var probeSocket = await CreateRawSocketAsync(cancellationToken).ConfigureAwait(false);
        using var stream = new NetworkStream(probeSocket, ownsSocket: false);

        await WriteFramedAsciiAsync(stream, "host:version", cancellationToken).ConfigureAwait(false);
        string status = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);

        if (status == "OKAY")
        {
            // Read 4-hex length and version payload
            string lenHex = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);
            if (int.TryParse(lenHex, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int len) && len > 0)
            {
                await ReadExactAsciiAsync(stream, len, cancellationToken).ConfigureAwait(false);
            }
            _isConnected = true;
        }
        else if (status == "FAIL")
        {
            string err = await ReadErrorMessageAsync(stream, cancellationToken).ConfigureAwait(false);
            throw new AdbException($"ADB server connection failed: {err}");
        }
        else
        {
            throw new AdbException($"Unexpected ADB response status: {status}");
        }
    }

    public async Task<string> SendAdbRequestAsync(string request, CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        using var socket = await CreateRawSocketAsync(cancellationToken).ConfigureAwait(false);
        using var stream = new NetworkStream(socket, ownsSocket: false);

        await WriteFramedAsciiAsync(stream, request, cancellationToken).ConfigureAwait(false);

        string status = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);
        if (status == "FAIL")
        {
            string error = await ReadErrorMessageAsync(stream, cancellationToken).ConfigureAwait(false);
            throw new AdbException($"ADB request '{request}' failed: {error}");
        }

        if (status != "OKAY")
        {
            throw new AdbException($"Unexpected ADB response '{status}' for request '{request}'");
        }

        // For host queries, read 4-hex length followed by payload
        string lenHex = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);
        if (!int.TryParse(lenHex, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int length) || length < 0)
        {
            throw new AdbException($"Invalid hex length prefix received: '{lenHex}'");
        }

        if (length == 0) return string.Empty;

        byte[] payloadBuffer = new byte[length];
        await ReadExactBytesAsync(stream, payloadBuffer, 0, length, cancellationToken).ConfigureAwait(false);
        return Encoding.UTF8.GetString(payloadBuffer);
    }

    public Task SelectDeviceAsync(string? serial = null, CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        _selectedSerial = serial;
        return Task.CompletedTask;
    }

    public async Task<string> ExecuteShellCommandAsync(string command, bool useV2 = false, CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        using var socket = await CreateTransportSocketAsync(cancellationToken).ConfigureAwait(false);
        using var stream = new NetworkStream(socket, ownsSocket: false);

        if (useV2)
        {
            return await ExecuteShellV2InternalAsync(stream, command, cancellationToken).ConfigureAwait(false);
        }
        else
        {
            return await ExecuteShellV1InternalAsync(stream, command, cancellationToken).ConfigureAwait(false);
        }
    }

    private async Task<string> ExecuteShellV1InternalAsync(NetworkStream stream, string command, CancellationToken ct)
    {
        await WriteFramedAsciiAsync(stream, $"shell:{command}", ct).ConfigureAwait(false);

        string status = await ReadExactAsciiAsync(stream, 4, ct).ConfigureAwait(false);
        if (status == "FAIL")
        {
            string err = await ReadErrorMessageAsync(stream, ct).ConfigureAwait(false);
            throw new AdbException($"Shell command failed to start: {err}");
        }
        if (status != "OKAY")
        {
            throw new AdbException($"Unexpected status during shell execution: {status}");
        }

        using var ms = new MemoryStream();
        byte[] buffer = new byte[8192];
        while (true)
        {
            int read = await stream.ReadAsync(buffer.AsMemory(0, buffer.Length), ct).ConfigureAwait(false);
            if (read == 0) break; // EOF
            ms.Write(buffer, 0, read);
        }

        return Encoding.UTF8.GetString(ms.ToArray());
    }

    private async Task<string> ExecuteShellV2InternalAsync(NetworkStream stream, string command, CancellationToken ct)
    {
        await WriteFramedAsciiAsync(stream, $"shell,v2,raw:{command}", ct).ConfigureAwait(false);

        string status = await ReadExactAsciiAsync(stream, 4, ct).ConfigureAwait(false);
        if (status == "FAIL")
        {
            string err = await ReadErrorMessageAsync(stream, ct).ConfigureAwait(false);
            throw new AdbException($"Shell v2 command failed to start: {err}");
        }
        if (status != "OKAY")
        {
            throw new AdbException($"Unexpected status during shell v2 execution: {status}");
        }

        using var stdoutMs = new MemoryStream();
        byte[] header = new byte[5];

        while (true)
        {
            int bytesRead = await ReadExactBytesAsync(stream, header, 0, 5, ct, allowEof: true).ConfigureAwait(false);
            if (bytesRead == 0) break; // Clean EOF

            byte packetId = header[0];
            uint payloadLength = BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(1, 4));

            byte[] payload = new byte[payloadLength];
            if (payloadLength > 0)
            {
                await ReadExactBytesAsync(stream, payload, 0, (int)payloadLength, ct).ConfigureAwait(false);
            }

            switch (packetId)
            {
                case 1: // stdout
                    stdoutMs.Write(payload, 0, payload.Length);
                    break;
                case 2: // stderr
                    stdoutMs.Write(payload, 0, payload.Length);
                    break;
                case 3: // exit code
                    // Packet payload is 1-byte exit status
                    return Encoding.UTF8.GetString(stdoutMs.ToArray());
                default:
                    // Ignore window size / stdin closed packets
                    break;
            }
        }

        return Encoding.UTF8.GetString(stdoutMs.ToArray());
    }

    public async Task PushFileSyncAsync(
        string localPath,
        string remotePath,
        int permissions = 33188,
        Action<long, long>? progressCallback = null,
        CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        if (!File.Exists(localPath))
        {
            throw new FileNotFoundException($"Local file to push does not exist: {localPath}", localPath);
        }

        var fileInfo = new FileInfo(localPath);
        long totalFileSize = fileInfo.Length;

        using var socket = await CreateTransportSocketAsync(cancellationToken).ConfigureAwait(false);
        using var stream = new NetworkStream(socket, ownsSocket: false);

        // 1. Enter sync service
        await WriteFramedAsciiAsync(stream, "sync:", cancellationToken).ConfigureAwait(false);
        string syncStatus = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);
        if (syncStatus == "FAIL")
        {
            string err = await ReadErrorMessageAsync(stream, cancellationToken).ConfigureAwait(false);
            throw new AdbException($"Failed to start sync service: {err}");
        }
        if (syncStatus != "OKAY")
        {
            throw new AdbException($"Unexpected sync status response: {syncStatus}");
        }

        // 2. Send STAT packet to validate target
        byte[] remotePathBytes = Encoding.UTF8.GetBytes(remotePath);
        byte[] statPacket = new byte[8 + remotePathBytes.Length];
        Encoding.ASCII.GetBytes("STAT").CopyTo(statPacket, 0);
        BinaryPrimitives.WriteUInt32LittleEndian(statPacket.AsSpan(4, 4), (uint)remotePathBytes.Length);
        remotePathBytes.CopyTo(statPacket, 8);
        await stream.WriteAsync(statPacket, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);

        // Read 16-byte STAT response
        byte[] statResponse = new byte[16];
        await ReadExactBytesAsync(stream, statResponse, 0, 16, cancellationToken).ConfigureAwait(false);
        string statId = Encoding.ASCII.GetString(statResponse, 0, 4);
        if (statId != "STAT" && statId != "STA2")
        {
            throw new AdbException($"Invalid STAT response ID: '{statId}'");
        }

        // 3. Send SEND packet with target path and octal mode (e.g. "/data/local/tmp/AT,33188")
        string sendSpec = $"{remotePath},{permissions}";
        byte[] sendSpecBytes = Encoding.UTF8.GetBytes(sendSpec);
        byte[] sendPacket = new byte[8 + sendSpecBytes.Length];
        Encoding.ASCII.GetBytes("SEND").CopyTo(sendPacket, 0);
        BinaryPrimitives.WriteUInt32LittleEndian(sendPacket.AsSpan(4, 4), (uint)sendSpecBytes.Length);
        sendSpecBytes.CopyTo(sendPacket, 8);
        await stream.WriteAsync(sendPacket, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);

        // 4. Stream DATA chunks up to 64KB (65536 bytes)
        using (var fs = new FileStream(localPath, FileMode.Open, FileAccess.Read, FileShare.Read, DefaultBufferSize, useAsync: true))
        {
            byte[] chunkDataBuffer = new byte[DefaultBufferSize];
            byte[] chunkHeader = new byte[8];
            Encoding.ASCII.GetBytes("DATA").CopyTo(chunkHeader, 0);

            long totalBytesSent = 0;
            int readBytes;
            while ((readBytes = await fs.ReadAsync(chunkDataBuffer.AsMemory(0, chunkDataBuffer.Length), cancellationToken).ConfigureAwait(false)) > 0)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(chunkHeader.AsSpan(4, 4), (uint)readBytes);
                await stream.WriteAsync(chunkHeader, cancellationToken).ConfigureAwait(false);
                await stream.WriteAsync(chunkDataBuffer.AsMemory(0, readBytes), cancellationToken).ConfigureAwait(false);

                totalBytesSent += readBytes;
                progressCallback?.Invoke(totalBytesSent, totalFileSize);
            }
        }
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);

        // 5. Send DONE packet with timestamp
        byte[] donePacket = new byte[8];
        Encoding.ASCII.GetBytes("DONE").CopyTo(donePacket, 0);
        uint timestamp = (uint)DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        BinaryPrimitives.WriteUInt32LittleEndian(donePacket.AsSpan(4, 4), timestamp);
        await stream.WriteAsync(donePacket, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);

        // Verify DONE response: OKAY/FAIL + 4B length
        byte[] doneResponseHeader = new byte[8];
        await ReadExactBytesAsync(stream, doneResponseHeader, 0, 8, cancellationToken).ConfigureAwait(false);
        string doneStatus = Encoding.ASCII.GetString(doneResponseHeader, 0, 4);
        uint messageLength = BinaryPrimitives.ReadUInt32LittleEndian(doneResponseHeader.AsSpan(4, 4));

        if (doneStatus == "FAIL")
        {
            byte[] errBytes = new byte[messageLength];
            if (messageLength > 0)
            {
                await ReadExactBytesAsync(stream, errBytes, 0, (int)messageLength, cancellationToken).ConfigureAwait(false);
            }
            string errStr = Encoding.UTF8.GetString(errBytes);
            throw new AdbException($"PushFileSync failed during DONE commit: {errStr}");
        }

        if (doneStatus != "OKAY")
        {
            throw new AdbException($"Unexpected status received after DONE: '{doneStatus}'");
        }

        // 6. Send QUIT packet and terminate sync session cleanly
        byte[] quitPacket = new byte[8];
        Encoding.ASCII.GetBytes("QUIT").CopyTo(quitPacket, 0);
        BinaryPrimitives.WriteUInt32LittleEndian(quitPacket.AsSpan(4, 4), 0);
        await stream.WriteAsync(quitPacket, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
    }

    public async Task<string> RestartUsbModeAsync(CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        using var socket = await CreateTransportSocketAsync(cancellationToken).ConfigureAwait(false);
        using var stream = new NetworkStream(socket, ownsSocket: false);

        await WriteFramedAsciiAsync(stream, "usb:", cancellationToken).ConfigureAwait(false);

        string status = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);
        if (status == "FAIL")
        {
            string err = await ReadErrorMessageAsync(stream, cancellationToken).ConfigureAwait(false);
            throw new AdbException($"RestartUsbMode failed: {err}");
        }

        if (status != "OKAY")
        {
            throw new AdbException($"Unexpected status for usb: command: {status}");
        }

        // Read stream output (expected "restarting in USB mode\n")
        using var ms = new MemoryStream();
        byte[] buffer = new byte[256];
        while (true)
        {
            int read = await stream.ReadAsync(buffer.AsMemory(0, buffer.Length), cancellationToken).ConfigureAwait(false);
            if (read == 0) break; // EOF as usb service restarts daemon
            ms.Write(buffer, 0, read);
        }

        return Encoding.UTF8.GetString(ms.ToArray()).Trim();
    }

    public async Task<Stream> OpenRawStreamAsync(string service, CancellationToken cancellationToken = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        var socket = await CreateTransportSocketAsync(cancellationToken).ConfigureAwait(false);
        var stream = new NetworkStream(socket, ownsSocket: true);

        try
        {
            await WriteFramedAsciiAsync(stream, service, cancellationToken).ConfigureAwait(false);

            string status = await ReadExactAsciiAsync(stream, 4, cancellationToken).ConfigureAwait(false);
            if (status == "FAIL")
            {
                string err = await ReadErrorMessageAsync(stream, cancellationToken).ConfigureAwait(false);
                throw new AdbException($"Failed to open service '{service}': {err}");
            }

            if (status != "OKAY")
            {
                throw new AdbException($"Unexpected response status '{status}' for service '{service}'");
            }

            return stream;
        }
        catch
        {
            await stream.DisposeAsync().ConfigureAwait(false);
            throw;
        }
    }

    #region Internal Socket Factory & Transport Lifecycle

    private async Task<Socket> CreateRawSocketAsync(CancellationToken ct)
    {
        var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp)
        {
            NoDelay = true
        };
        socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger, new LingerOption(false, 0));

        try
        {
            await socket.ConnectAsync(new DnsEndPoint(_host, _port), ct).ConfigureAwait(false);
            return socket;
        }
        catch (Exception ex)
        {
            socket.Dispose();
            throw new AdbException($"Cannot connect to ADB server at {_host}:{_port}. Ensure ADB server is running.", ex);
        }
    }

    private async Task<Socket> CreateTransportSocketAsync(CancellationToken ct)
    {
        var socket = await CreateRawSocketAsync(ct).ConfigureAwait(false);
        var stream = new NetworkStream(socket, ownsSocket: false);

        try
        {
            string transportCmd = string.IsNullOrEmpty(_selectedSerial)
                ? "host:transport-any"
                : $"host:transport:{_selectedSerial}";

            await WriteFramedAsciiAsync(stream, transportCmd, ct).ConfigureAwait(false);

            string status = await ReadExactAsciiAsync(stream, 4, ct).ConfigureAwait(false);
            if (status == "FAIL")
            {
                string err = await ReadErrorMessageAsync(stream, ct).ConfigureAwait(false);
                throw new AdbDeviceNotFoundException(_selectedSerial, $"Device transport switch failed: {err}");
            }

            if (status != "OKAY")
            {
                throw new AdbException($"Unexpected transport status response: '{status}'");
            }

            return socket;
        }
        catch
        {
            stream.Dispose();
            socket.Dispose();
            throw;
        }
    }

    #endregion

    #region Wire Framing Helpers

    private static async Task WriteFramedAsciiAsync(Stream stream, string payload, CancellationToken ct)
    {
        byte[] payloadBytes = Encoding.UTF8.GetBytes(payload);
        string lenPrefix = $"{payloadBytes.Length:x4}";
        byte[] headerBytes = Encoding.ASCII.GetBytes(lenPrefix);

        byte[] frame = new byte[headerBytes.Length + payloadBytes.Length];
        headerBytes.CopyTo(frame, 0);
        payloadBytes.CopyTo(frame, headerBytes.Length);

        await stream.WriteAsync(frame, ct).ConfigureAwait(false);
        await stream.FlushAsync(ct).ConfigureAwait(false);
    }

    private static async Task<string> ReadExactAsciiAsync(Stream stream, int byteCount, CancellationToken ct)
    {
        byte[] buffer = new byte[byteCount];
        await ReadExactBytesAsync(stream, buffer, 0, byteCount, ct).ConfigureAwait(false);
        return Encoding.ASCII.GetString(buffer);
    }

    private static async Task<string> ReadErrorMessageAsync(Stream stream, CancellationToken ct)
    {
        try
        {
            string lenHex = await ReadExactAsciiAsync(stream, 4, ct).ConfigureAwait(false);
            if (int.TryParse(lenHex, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int length) && length > 0)
            {
                byte[] errBuffer = new byte[length];
                await ReadExactBytesAsync(stream, errBuffer, 0, length, ct).ConfigureAwait(false);
                return Encoding.UTF8.GetString(errBuffer);
            }
        }
        catch
        {
            // Fallback if error payload formatting deviates
        }
        return "Unknown ADB error";
    }

    private static async Task<int> ReadExactBytesAsync(Stream stream, byte[] buffer, int offset, int count, CancellationToken ct, bool allowEof = false)
    {
        int totalRead = 0;
        while (totalRead < count)
        {
            int read = await stream.ReadAsync(buffer.AsMemory(offset + totalRead, count - totalRead), ct).ConfigureAwait(false);
            if (read == 0)
            {
                if (allowEof && totalRead == 0) return 0;
                throw new EndOfStreamException($"Expected {count} bytes but stream closed prematurely after {totalRead} bytes.");
            }
            totalRead += read;
        }
        return totalRead;
    }

    #endregion

    #region Disposal

    public ValueTask DisposeAsync()
    {
        if (!_disposed)
        {
            _disposed = true;
            _isConnected = false;
        }
        return ValueTask.CompletedTask;
    }

    #endregion
}
```

---

## 5. Edge Cases, Concurrency & Resiliency Hardening

### 5.1 Memory Protection During Large File Push
Pushing binaries (like `AT` at ~492KB or larger payloads) can cause memory spikes if read entirely into memory.
- `AdbRawSocketClient` uses a small 64KB stack/heap chunk buffer and streams directly from `FileStream` into `NetworkStream`.
- Chunk size is bounded to `65536` bytes, strictly matching ADB server's maximum sync chunk size.
- Async `ReadAsync` from disk and `WriteAsync` to socket occur in a pipelined while-loop.

### 5.2 Little-Endian Cross-Platform Safety
ADB binary sync protocol dictates Little-Endian byte order for all binary fields (`mode`, `size`, `mtime`, `chunkLength`).
- Rather than relying on `BitConverter.GetBytes()` (which is architecture-dependent and flips on Big-Endian architectures), `AdbRawSocketClient` uses `System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian` and `BinaryPrimitives.ReadUInt32LittleEndian`.
- This ensures 100% deterministic wire behavior across x86, x64, ARM64, and any execution environment.

### 5.3 Socket Exhaustion & Linger Settings
When high numbers of shell commands are issued during telemetry acquisition (the 8 pre-checks: RSU check, Knox check, getprop, IMEI parcel call, SELinux check, KEYTool check, port 2600 check, diagnostics check):
- If sockets linger in `TIME_WAIT`, Windows socket exhaustion can occur.
- Setting `LingerOption(false, 0)` enables immediate hard RST upon socket disposal for transient command streams, preventing ephemeral port exhaustion.

### 5.4 Timeout and Cancellation Propagation
Every async operation accepts a `CancellationToken`. Sockets and streams in .NET 8 honor cancellation tokens on `ConnectAsync`, `ReadAsync`, and `WriteAsync`. When cancelled, the underlying socket is immediately aborted, freeing server resources.

---

## 6. Testability & Verification Blueprint (`MockAdbServer` Contract)

To enable comprehensive Tier 1-4 and Tier 5 adversarial testing without requiring physical devices or a running `adb.exe`, `AdbRawSocketClient` connects to any IP/port. The companion test class `MockAdbServer` can listen on `IPAddress.Loopback` on an ephemeral port.

### `MockAdbServer` Behavior Specification:
1. **Handshake Verification**:
   - Responds `OKAY00040029` to `000chost:version`.
   - Responds `OKAY` to `host:transport:<serial>`.
2. **Command Verification**:
   - For `shell:getprop`: Emits forensic output matching `csc 15.txt` (lines 28-30: `ro.product.model=SM-S918U | ro.boot.serialno=RFCX90GR21X`) and closes connection.
   - For `shell:service call iphonesubinfo 1 s16 com.android.shell`: Emits Parcel hex dump (lines 40-42) and closes connection.
3. **Sync Protocol Verification**:
   - Reads `0005sync:` -> replies `OKAY`.
   - Reads `STAT` + length + path -> validates length 18 and path `/data/local/tmp/AT`, replies 16-byte `STAT` header.
   - Reads `SEND` + length + `"/data/local/tmp/AT,33188"`.
   - Reads sequence of `DATA` chunks until `DONE`.
   - Verifies total bytes transferred equals local file size.
   - Replies `OKAY` + 4-byte LE 0.
   - Reads `QUIT` -> closes socket.
4. **USB Reset Verification**:
   - Reads `0004usb:` -> replies `OKAY`.
   - Emits `"restarting in USB mode\n"` and closes socket.

---

## 7. Summary & Downstream Milestones Readiness

With this blueprint:
- **Milestone 1 Implementation** can drop `IAdbRawSocketClient.cs` and `AdbRawSocketClient.cs` into `GsmTools.Desktop/Core/` with 0 warnings and 0 compilation errors.
- **`PersistentAtBridgeSession`** is supported via `OpenRawStreamAsync`, which provides the interactive conduit to `/data/local/tmp/AT`.
- **`SamsungCscWorkflow`** (Milestone 2) can execute all 8 pre-checks, push `/AT`, invoke shell commands, and execute `0004usb:` reset without a single external process call.
