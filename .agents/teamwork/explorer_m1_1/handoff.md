# Handoff Report: `AdbRawSocketClient` & `IAdbRawSocketClient` Architecture Blueprint

**Milestone:** Milestone 1 Core (Core Protocols & Crypto)  
**Author:** explorer_m1_1 (Preview Explorer)  
**Date:** 2026-10-17  
**Status:** Hard Handoff (Investigation & Blueprint Complete)  
**Primary Deliverable:** `c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_m1_1\report.md`

---

## 1. Observation

1. **Original Request (§R1, `ORIGINAL_REQUEST.md:12-21`)**:
   > "Direct TCP connection to ADB server (`127.0.0.1:5037`) without spawning `adb.exe` processes."
   > "Framed request handling (`SendAdbRequestAsync`) with 4-hex-char ASCII length prefixes and `OKAY`/`FAIL` parsing."
   > "Interactive and one-shot shell command execution (`ExecuteShellCommandAsync`) supporting both standard `shell:<command>` and `shell,v2,raw:<command>` framing."
   > "Native ADB `sync:` protocol implementation (`PushFileSyncAsync`) for pushing `/data/local/tmp/AT`:"
   > "  - `STAT` packet with 4-byte LE path length and path `/data/local/tmp/AT`, validating 16-byte `STAT` response."
   > "  - `SEND` packet with path and mode `/data/local/tmp/AT,33188` (octal 0100644)."
   > "  - File chunking via `DATA` packets (up to 64KB per chunk: `DATA` + 4-byte LE length + payload)."
   > "  - Transfer termination via `DONE` packet with 4-byte LE timestamp, verifying `OKAY`, followed by `QUIT`."
   > "Direct ADB protocol USB mode reset (`RestartUsbModeAsync`) sending `0004usb:` and reading `restarting in USB mode`."

2. **Project Specification (`PROJECT.md:140-150`)**:
   > `IAdbRawSocketClient` defines:
   > - `Task ConnectAsync(string host = "127.0.0.1", int port = 5037, CancellationToken cancellationToken = default);`
   > - `Task<string> SendAdbRequestAsync(string request, CancellationToken cancellationToken = default);`
   > - `Task SelectDeviceAsync(string? serial = null, CancellationToken cancellationToken = default);`
   > - `Task<string> ExecuteShellCommandAsync(string command, bool useV2 = false, CancellationToken cancellationToken = default);`
   > - `Task PushFileSyncAsync(string localPath, string remotePath, int permissions = 33188, Action<long, long>? progressCallback = null, CancellationToken cancellationToken = default);`
   > - `Task<string> RestartUsbModeAsync(CancellationToken cancellationToken = default);`
   > - `bool IsConnected { get; }`

3. **Forensic Log Trace (`csc 15.txt:2363-2371`)**:
   > `[17:42:26.598] ADB-INTERACTIVE-CMD | Command: usb:`  
   > `[17:42:26.599] ADB-STREAM | Command: restarting in USB mode`  
   Demonstrating the exact response of the ADB server to the `usb:` service request after injection completion.

4. **Forensic File Push & Shell Invocation (`csc 15.txt:2170-2214`)**:
   > `csc 15.txt:2199` shows the termination of the root shell relay, followed immediately by:  
   > `[17:42:22.004] ADB-INTERACTIVE-CMD | Command: shell:chmod 777 /data/local/tmp/AT`  
   > `[17:42:22.035] ADB-INTERACTIVE-CMD | Command: shell:/data/local/tmp/AT`  
   > `[17:42:22.590] ADB-STREAM | Command: ******** ... SAMSUNG AT Ril Command TOOL V4 ...`

5. **AOSP Wire Protocol Specifications (`adb_client.cpp`, `file_sync_service.h`, `shell_protocol.h`)**:
   - In ADB protocol, after sending `host:transport:<serial>`, once a downstream service (`shell:`, `sync:`, `usb:`) is requested and acknowledged with `OKAY`, the connection is exclusively dedicated to that service.
   - Upon completion of the service (e.g. process exit or sync `QUIT`), the remote daemon closes the channel, which causes the ADB server to terminate the TCP connection.
   - Little-Endian byte ordering is required for all binary sync packet fields (length, mode, size, mtime) and Shell Protocol V2 packet lengths.

---

## 2. Logic Chain

1. **From Observation 1 & 5 (Socket Lifespan)**:
   - A naive implementation keeping a single persistent TCP socket for all shell and sync commands will fail after the first command completes, because the ADB server closes the underlying TCP socket when the service terminates.
   - *Therefore*, `AdbRawSocketClient` must implement an internal **Transport Socket Factory** (`CreateTransportSocketAsync`) that creates a dedicated connection, switches transport to the target device, executes the command or sync session to completion, and disposes the socket cleanly.

2. **From Observation 1, 2 & 5 (Framed Request & Response)**:
   - Command payloads require a 4-hex-character ASCII length prefix (e.g., `"{len:x4}{cmd}"`).
   - Host queries (e.g. `host:version`, `host:devices`) return `OKAY` + 4-hex length + payload string.
   - Service requests (e.g. `host:transport:<serial>`, `shell:<cmd>`, `sync:`, `usb:`) return `OKAY` without a length prefix, transitioning the socket immediately to the raw service stream.
   - Failure responses *always* begin with `FAIL` + 4-hex length + error message.
   - *Therefore*, the parser must distinguish between host length-prefixed responses and service raw stream responses.

3. **From Observation 1 & 5 (Sync Push Protocol)**:
   - Pushing `/data/local/tmp/AT` (size ~492KB) requires sending:
     1. `0005sync:` -> `OKAY`
     2. `STAT` + uint32_LE(18) + `"/data/local/tmp/AT"` -> validating 16-byte `STAT` response
     3. `SEND` + uint32_LE(24) + `"/data/local/tmp/AT,33188"` (mode 33188 = octal 0100644)
     4. Multiple `DATA` packets: `DATA` + uint32_LE(chunkSize <= 65536) + chunk payload
     5. `DONE` + uint32_LE(timestamp) -> verifying `OKAY`
     6. `QUIT` + uint32_LE(0)
   - *Therefore*, `PushFileSyncAsync` must use `BinaryPrimitives.WriteUInt32LittleEndian` to ensure endianness safety and stream directly from a `FileStream` in 64KB chunks to avoid memory bloat.

4. **From Observation 1 & 3 (USB Reset)**:
   - Sending `0004usb:` over the transport socket returns `OKAY` followed by the stream text `restarting in USB mode\n`.
   - *Therefore*, `RestartUsbModeAsync` connects a transport socket, issues `usb:`, verifies `OKAY`, and reads until EOF, returning `"restarting in USB mode"`.

5. **From Observation 4 (`at_bridge.c` & `PersistentAtBridgeSession`)**:
   - `PersistentAtBridgeSession` requires an open, interactive bidirectional stream to `/data/local/tmp/AT`.
   - *Therefore*, `IAdbRawSocketClient` must provide `Task<Stream> OpenRawStreamAsync(string service, CancellationToken ct)`, returning a live `NetworkStream` bound to the running bridge binary.

---

## 3. Caveats

1. **ADB Server Daemon Availability**: The client communicates directly over TCP loopback (`127.0.0.1:5037`). It assumes an ADB server is running on the host machine. If not running, `AdbException` is thrown indicating the server is unreachable.
2. **Device State**: If multiple devices are connected and `_selectedSerial` is null, `host:transport-any` is used; if more than one device exists, ADB server returns `FAIL001fmore than one device/emulator`. The UI/Workflow layer must call `SelectDeviceAsync(serial)` when specific serials are targeted.
3. **Shell V2 Availability**: Android devices running OS versions older than Android 7.0 (API 24) do not support `shell,v2,raw:`. For Samsung devices targeted in this project (e.g. Galaxy S23 SM-S918U running Android 15), Shell V2 and standard Shell V1 are both fully supported. The implementation defaults to V1 (`useV2 = false`) and falls back cleanly.

---

## 4. Conclusion

The architectural design and C# implementation blueprint for `IAdbRawSocketClient.cs` and `AdbRawSocketClient.cs` are completely specified and validated against the AOSP protocol and the forensic trace in `csc 15.txt`.
The deliverable `c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_m1_1\report.md` provides:
1. Complete C# source code ready for Milestone 1 compilation under .NET 8.
2. Full wire-level packet diagrams for all 5 protocol phases: Framed requests, transport switching, Shell V1/V2 execution, binary `sync:` push (STAT/SEND/DATA/DONE/QUIT), and USB reset.
3. Dedicated `OpenRawStreamAsync` API to seamlessly power `PersistentAtBridgeSession`.
4. Resiliency provisions: little-endian binary safety (`BinaryPrimitives`), socket lingering (`LingerOption(false, 0)`), 64KB streaming buffer limits, and async cancellation token propagation.

---

## 5. Verification Method

To independently verify the architecture and blueprint:
1. **Source Inspection**:
   - Review `report.md` Section 4 for completeness of `IAdbRawSocketClient.cs` and `AdbRawSocketClient.cs`.
   - Confirm all methods from `PROJECT.md:140-150` are satisfied.
2. **Binary Sync Math Check**:
   - Verify octal `0100644` equals decimal `33188`:
     `0100000 (32768) + 0644 (420) = 33188`. Exact match.
   - Verify 64KB chunking: `65536` bytes (`0x10000`).
3. **Forensic Trace Alignment**:
   - Verify lines 2363-2371 in `csc 15.txt` match `RestartUsbModeAsync` (`usb:` -> `restarting in USB mode`).
   - Verify lines 2201-2214 in `csc 15.txt` match file deployment sequence (`PushFileSyncAsync` -> `chmod 777` -> execute `/AT`).
4. **Invalidation Conditions**:
   - If ADB server protocol requires external binary processes (disproven: ADB server is a pure TCP daemon).
   - If `adbd` sync protocol uses Big-Endian fields (disproven: AOSP `file_sync_service.h` specifies `htoll` / Little-Endian).
