# GsmTools.Desktop — Architecture & Protocol Specifications

**Author:** explorer_survey_3 (Teamwork Explorer)  
**Date:** 2026-10-16  
**Target Platform:** Windows 10/11 (.NET 8.0 WPF, `net8.0-windows`)  
**Scope:** Native ADB TCP Protocol, Parcel Parsing, AT Bridge Cryptography, Clean WPF Architecture, and Test Infrastructure.

---

## 1. Native ADB TCP Socket Protocol Specification

The desktop application connects directly to the local ADB daemon server on `127.0.0.1:5037` over raw TCP sockets (`System.Net.Sockets.TcpClient` / `Socket`). It eliminates all dependencies on external `adb.exe` process spawning, CLI subprocess pipes, or command-line wrappers.

```
+---------------------+           TCP :5037           +---------------------+
|  GsmTools.Desktop   | <===========================> |     ADB Server      |
| AdbRawSocketClient  |   [4-Hex Prefix + Payload]    |  (127.0.0.1:5037)   |
+---------------------+                               +---------------------+
                                                                 ||  (USB / TCP)
                                                                 \/
                                                      +---------------------+
                                                      |   Android Device    |
                                                      |     adbd daemon     |
                                                      +---------------------+
```

### 1.1 ADB Framing & Control Flow

Every ADB request sent to `127.0.0.1:5037` is framed with a 4-byte ASCII hexadecimal length prefix representing the payload length in bytes (excluding the prefix itself), followed by the ASCII payload.

$$\text{Wire Packet} = [\text{Length: 4 ASCII Hex Chars}] + [\text{Payload: } N \text{ ASCII Bytes}]$$

#### Handshake & Core Control Commands
1. **Host Version Check (`host:version`):**
   - **Request:** `000Chost:version` (Length: 12 = `000C`)
   - **Response Format:**
     - 4-byte status: `OKAY` (`0x4F 0x4B 0x41 0x59`) or `FAIL` (`0x46 0x41 0x49 0x4C`)
     - If `OKAY`: Followed by 4-hex-char length prefix (e.g., `0004`) and 4-digit hex version string (e.g., `0029` for protocol version 41).
     - If `FAIL`: Followed by 4-hex-char length prefix and an ASCII error string.
2. **List Connected Devices (`host:devices` or `host:devices-l`):**
   - **Request:** `000Chost:devices`
   - **Response Format:**
     - `OKAY`
     - 4-hex-char payload length (e.g. `001B` = 27 bytes)
     - Device list string where each device is separated by `\n`: `<serial>\t<state>\n` (e.g. `RFCX90GR21X\tdevice\n`).
3. **Transport Routing (`host:transport:<serial>`):**
   - **Request:** `001Bhost:transport:RFCX90GR21X` (length = 4 + 10 + 11 = 25 = `0019` in hex, or length computed dynamically as `length.ToString("X4")`).
   - **Response:** `OKAY`
   - **State Transition:** Upon receiving `OKAY`, the ADB server switches the connection into a transparent tunnel directly to the target device's `adbd` service. All subsequent data sent on this socket is interpreted by `adbd`.

### 1.2 Interactive & One-Shot Shell Execution

Once transport is established (`host:transport:<serial>` $\to$ `OKAY`), the client invokes shell commands using either standard `shell:` or modern `shell,v2,raw:`.

#### A. Standard Shell (`shell:<command>`)
- **Wire Request:** `[4-Hex Length]shell:<command>`
  - Example: `0022shell:pm list package|grep rsuapp`
- **Response:**
  - 4-byte status: `OKAY`.
  - Stream: Unframed raw byte stream representing stdout and stderr interleaved.
  - Stream Termination: The server closes the socket (`Socket.ReceiveAsync` returns 0 bytes) when the remote process exits.

#### B. Shell V2 Raw (`shell,v2,raw:<command>`)
Used for structured separation of stdout, stderr, and exit codes:
- **Wire Request:** `[4-Hex Length]shell,v2,raw:<command>`
- **Packet Structure:** Every message in Shell V2 has a 5-byte header followed by payload:
  ```
  +---------------+-------------------------------+-----------------------+
  | Stream ID (1B)| Length (4B Little-Endian uint)| Payload (Length bytes)|
  +---------------+-------------------------------+-----------------------+
  ```
  - **Stream ID Definitions:**
    - `0x01` (1): `stdin`
    - `0x02` (2): `stdout`
    - `0x03` (3): `stderr`
    - `0x04` (4): `exit` (Payload is 1 byte exit code, e.g. `0x00`)
    - `0x05` (5): `close_stdin`
    - `0x06` (6): `window_size`

### 1.3 Native ADB Sync Protocol (`sync:`)

The `sync:` service is a high-speed binary protocol used to push `/data/local/tmp/AT` directly to device storage without `base64` or `toybox nc` workarounds.

```
Client                                                  ADB Server / adbd
  |                                                             |
  | -------- 0005sync: ---------------------------------------> |
  | <------- OKAY --------------------------------------------- |
  |                                                             |
  | -------- STAT [4B LE path len] [/data/local/tmp/AT] ------> |
  | <------- STAT [mode(4B)][size(4B)][mtime(4B)] ------------- |
  |                                                             |
  | -------- SEND [4B LE len] [/data/local/tmp/AT,33188] -----> |
  | -------- DATA [4B LE chunk len] [payload up to 64KB] -----> |
  | -------- ... [repeat DATA for remaining chunks] ----------> |
  | -------- DONE [4B LE timestamp] --------------------------> |
  | <------- OKAY (or FAIL + len + msg) ----------------------- |
  |                                                             |
  | -------- QUIT [00 00 00 00] ------------------------------> |
```

#### Detailed Packet Layouts:
1. **Enter Sync Service:**
   - Send: `0005sync:`
   - Expect: `OKAY`
2. **`STAT` (Query File Metadata):**
   - Header: `0x53 0x54 0x41 0x54` (`"STAT"`)
   - Path Length: 4 bytes Little-Endian `uint32`
   - Path Bytes: ASCII path (e.g. `/data/local/tmp/AT`)
   - Server Response: 16 bytes:
     - 4 bytes: `"STAT"`
     - 4 bytes LE `uint32`: File mode (octal permissions, e.g., `0100644` = `33188`, or `0` if non-existent).
     - 4 bytes LE `uint32`: File size in bytes.
     - 4 bytes LE `uint32`: File modification Unix timestamp.
3. **`SEND` (Initiate File Transfer):**
   - Header: `0x53 0x45 0x4E 0x44` (`"SEND"`)
   - Length: 4 bytes Little-Endian `uint32` (length of path + comma + mode string).
   - Mode String: `<remote_path>,<octal_or_decimal_mode>`
     - Observed in `csc 15.txt`: `/data/local/tmp/AT,33188` (`33188` decimal = `0100644` octal, representing standard readable/writable regular file).
4. **`DATA` (Chunk Delivery):**
   - Header: `0x44 0x41 0x54 0x41` (`"DATA"`)
   - Chunk Length: 4 bytes Little-Endian `uint32` (standard maximum chunk size is 65,536 bytes / 64 KB).
   - Payload: Raw file bytes (up to 64 KB).
   - Send repeated `DATA` packets until all file bytes are transmitted.
5. **`DONE` (Transfer Finalization):**
   - Header: `0x44 0x4F 0x4E 0x45` (`"DONE"`)
   - Timestamp: 4 bytes Little-Endian `uint32` (Unix epoch timestamp of file modification).
   - Server Response:
     - `OKAY` (4 bytes `0x4F 0x4B 0x41 0x59`) on success.
     - `FAIL` (4 bytes `0x46 0x41 0x49 0x4C`) + 4-byte LE length + UTF-8 error string on failure.
6. **`QUIT` (Clean Session Close):**
   - Header: `0x51 0x55 0x49 0x54` (`"QUIT"`)
   - Length: 4 bytes `0x00 0x00 0x00 0x00`.

### 1.4 Direct USB Mode Reset (`0004usb:`)

As observed in forensic trace `csc 15.txt` line 2363-2371:
- Once transport is established to the device (`host:transport:<serial>`), sending:
  `0004usb:`
- Server response:
  - Responds with `OKAY`.
  - Stream outputs: `restarting in USB mode\n`.
  - The socket is terminated by `adbd` as the device switches USB controller states and resets ADB endpoints.
- Client validation: Verify that the response begins with `OKAY` and incoming text matches `restarting in USB mode`.

---

## 2. IMEI Parcel Parser (`Core/ImeiParcelParser.cs`)

### 2.1 Android Binder IPC `service call` Format

On modern Samsung and Android devices, obtaining the IMEI requires querying the telephony subsystem via Android Binder IPC:
```bash
service call iphonesubinfo 1 s16 com.android.shell
```
- `iphonesubinfo`: The `IPhoneSubInfo` service registered with `servicemanager`.
- `1`: Transaction code for `getDeviceId()` or `getImei()`.
- `s16 com.android.shell`: Caller package authorization token passed as a 16-bit string (`s16`).

The Android CLI utility `service` outputs the marshaled binder `Parcel` as formatted hexadecimal words:
```
Result: Parcel(	
0x00000000: 00000000 0000000f 00350033 00370035 '........3.5.5.7.'
0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'
0x00000020: 00330036 00000033                   '6.3.3...        ')
```

### 2.2 Wire Parcel Structure Decomposition

In Android Binder parcel serialization for returned strings:
- **Bytes 0–3 (`0x00000000` word 0):** Status Code `0x00000000` (`NO_ERROR`).
- **Bytes 4–7 (`0x00000000` word 1):** String character count `0x0000000F` (decimal 15).
- **Bytes 8+:** Sequence of 16-bit UTF-16LE characters packed into 32-bit big-endian printed words.

Each 32-bit word in the dump represents two 16-bit characters:
$$\text{Word} = \text{Char}_2 \text{ (Bits 31..16)} \mid \text{Char}_1 \text{ (Bits 15..0)}$$

| Hex Word | Low 16-bits (Char 1) | High 16-bits (Char 2) | ASCII Decoded |
|---|---|---|---|
| `00350033` | `0x0033` ('3') | `0x0035` ('5') | `"3"`, `"5"` |
| `00370035` | `0x0035` ('5') | `0x0037` ('7') | `"5"`, `"7"` |
| `00320035` | `0x0035` ('5') | `0x0032` ('2') | `"5"`, `"2"` |
| `00320039` | `0x0039` ('9') | `0x0032` ('2') | `"9"`, `"2"` |
| `00350030` | `0x0030` ('0') | `0x0035` ('5') | `"0"`, `"5"` |
| `00320034` | `0x0034` ('4') | `0x0032` ('2') | `"4"`, `"2"` |
| `00330036` | `0x0036` ('6') | `0x0033` ('3') | `"6"`, `"3"` |
| `00000033` | `0x0033` ('3') | `0x0000` (`\0`) | `"3"` |

Concatenating characters in order yields:
$$\mathbf{"355752920542633"}$$

### 2.3 C# Implementation Blueprint (`Core/ImeiParcelParser.cs`)

```csharp
using System;
using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

namespace GsmTools.Desktop.Core
{
    public static class ImeiParcelParser
    {
        private static readonly Regex LinePattern = new(
            @"^0x[0-9a-fA-F]{8}:\s+((?:[0-9a-fA-F]{8}\s*)+)",
            RegexOptions.Compiled | RegexOptions.Multiline);

        public static string Parse(string rawOutput)
        {
            if (string.IsNullOrWhiteSpace(rawOutput))
                throw new ArgumentException("Parcel output is empty.", nameof(rawOutput));

            var words = new System.Collections.Generic.List<uint>();
            var matches = LinePattern.Matches(rawOutput);

            foreach (Match match in matches)
            {
                var wordStrings = match.Groups[1].Value.Trim().Split(
                    new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                foreach (var ws in wordStrings)
                {
                    if (uint.TryParse(ws, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var val))
                    {
                        words.Add(val);
                    }
                }
            }

            if (words.Count < 3)
                throw new FormatException("Insufficient parcel data words to extract IMEI.");

            // Word 0: Status code (must be 0 for NO_ERROR)
            if (words[0] != 0)
                throw new InvalidOperationException($"Binder call failed with error code: 0x{words[0]:X8}");

            // Word 1: Character count (expected 14 or 15)
            uint charCount = words[1];
            if (charCount == 0 || charCount > 32)
                throw new FormatException($"Invalid parcel string length: {charCount}");

            var sb = new StringBuilder();
            for (int i = 2; i < words.Count && sb.Length < charCount; i++)
            {
                uint word = words[i];
                char c1 = (char)(word & 0xFFFF);
                if (c1 != '\0') sb.Append(c1);

                if (sb.Length < charCount)
                {
                    char c2 = (char)((word >> 16) & 0xFFFF);
                    if (c2 != '\0') sb.Append(c2);
                }
            }

            string imei = sb.ToString().Trim();
            if (!Regex.IsMatch(imei, @"^\d{14,16}$"))
                throw new FormatException($"Parsed IMEI '{imei}' does not conform to decimal digit format.");

            return imei;
        }
    }
}
```

---

## 3. Persistent AT Bridge Session & Cryptography

### 3.1 Xorshift32 Stream Cipher (`Core/Xorshift32StreamCipher.cs`)

In `at-bridge/jni/at_bridge.c`, lines 74–84 define the exact dynamic session stream transformation:
```c
static void transform_buffer(uint8_t *buf, size_t len, uint32_t seed) {
    if (seed == 0) return; /* Unencrypted mode */
    uint32_t state = seed;
    for (size_t i = 0; i < len; i++) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        buf[i] ^= (uint8_t)(state & 0xFF);
    }
}
```

#### Cryptographic Contract:
1. **Stateless per Frame:** For each frame transmission or response reception, `state` is re-initialized to `seed`.
2. **Identity on Inversion:** Because XOR is an involution ($A \oplus B \oplus B = A$), decryption is identical to encryption.
3. **C# Implementation:**

```csharp
namespace GsmTools.Desktop.Core
{
    public static class Xorshift32StreamCipher
    {
        public static void Transform(Span<byte> buffer, uint seed)
        {
            if (seed == 0) return;

            uint state = seed;
            for (int i = 0; i < buffer.Length; i++)
            {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                buffer[i] ^= (byte)(state & 0xFF);
            }
        }

        public static byte[] TransformCopy(ReadOnlySpan<byte> input, uint seed)
        {
            byte[] output = input.ToArray();
            Transform(output.AsSpan(), seed);
            return output;
        }
    }
}
```

### 3.2 Persistent Interactive Session Protocol (`Core/PersistentAtBridgeSession.cs`)

```
GsmTools.Desktop                                             /data/local/tmp/AT
       |                                                             |
       | --- Spawn interactive session (/data/local/tmp/AT) -------> |
       | <--- INFO|MODEL=...|CSC=...|UID=...|CH=... ----------------- |
       | <--- "<< " (Prompt) --------------------------------------- |
       |                                                             |
       | [If Encryption Enabled]                                     |
       | --- AUTH:A1B2C3D4\n --------------------------------------> |
       | <--- .OK_AUTH..\n<<  -------------------------------------- |
       |                                                             |
       | [CSC Command 1]                                             |
       | --- TransformedHex\n -------------------------------------> |
       | <--- .OK..#\n<<  (or .OK..<TransformedHex>#\n<< ) ---------- |
       |                                                             |
       | [Clean Termination]                                         |
       | --- EXIT\n -----------------------------------------------> |
       | <--- Session Terminated ----------------------------------- |
```

#### UID 1000 Auto-Relay Fallback:
If the shell runs as `shell` (`UID=2000`) and the binary detects `CH=NONE` (no direct access to `/dev/umts_ipc0` due to SELinux `untrusted_app` restrictions), but port 2600 is open:
1. The bridge attempts automatic escalation internally via `try_escalate_via_port_2600(2600)`.
2. If spawning directly from shell fails to achieve UID 1000, `PersistentAtBridgeSession` executes `/data/local/tmp/AT` through a toybox netcat pipe:
   ```bash
   toybox nc localhost 2600
   ```
   feeding stdin/stdout to `/data/local/tmp/AT`.

---

## 4. Samsung CSC Workflow Orchestrator (`Workflows/SamsungCscWorkflow.cs`)

The workflow orchestrates the exact sequence documented in `csc 15.txt`:

```
+-------------------------------------------------------------------------+
|                  Phase 1: Deep Telemetry & Pre-Checks                   |
+-------------------------------------------------------------------------+
  1. Carrier RSU Check:        pm list package|grep rsuapp
  2. Knox tsdaemon Check:      ps -A|grep tsdaemon;ps|grep tsdaemon
  3. Prop Telemetry:           getprop (Model, Serial, CSC, Build, Android, Patch)
  4. IMEI Extraction:          service call iphonesubinfo 1 s16 com.android.shell
  5. SELinux Status:           getenforce
  6. KEYTool Presence:         pm path KEYTool.SRV4
  7. UID 1000 Verification:    echo 'id' |toybox nc localhost 2600
  8. Diagnostic Inspection:    pm list package repaircal/mobiledoctor/retailagent
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                  Phase 2: Privilege Escalation & Bridge                 |
+-------------------------------------------------------------------------+
  1. Wake lock:                svc power stayon true
  2. Poll port 2600 (500ms):   echo 'id' |toybox nc localhost 2600 -> uid=1000(system)
  3. Stop conflicting ssud:    echo 'setprop ctl.stop ssud'|toybox nc localhost 2600
  4. Push /AT binary:          PushFileSyncAsync(./AT -> /data/local/tmp/AT, 33188)
  5. Permissions:              chmod 777 /data/local/tmp/AT
  6. Establish AT Session:     Launch /data/local/tmp/AT, verify banner & prompt
  7. Auth / Stream Cipher:     AUTH:<seed> -> .OK_AUTH..
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                     Phase 3: 5-Command CSC Injection                    |
+-------------------------------------------------------------------------+
  1. Clear Radio Logs:         logcat -b radio -c
  2. Injection Sequence:
     - Cmd 1: 0601000e0000{hexCsc}0000{hexCsc}
     - Cmd 2: 060100090000{hexCsc}
     - Cmd 3: 060100120000{hexCsc}0000{hexCsc}{hexCsc}
     - Cmd 4: 060100120000
     - Cmd 5: 060100090000{hexCsc}
  3. Close AT Session:         EXIT
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                    Phase 4: Cleanup & USB Mode Reset                    |
+-------------------------------------------------------------------------+
  1. Post-cleanup:             pm uninstall -k --user 0 com.sec.android.AutoPreconfig;
                               pm uninstall -k --user 0 com.sec.omadmspr;
                               pm clear com.samsung.android.app.mobiledoctor
  2. Direct ADB USB Reset:     0004usb: -> restarting in USB mode
+-------------------------------------------------------------------------+
```

### The 5 CSC Injection Packets Explained:
Target CSC `USC` translates to ASCII hex `555343` (`'U'=0x55, 'S'=0x53, 'C'=0x43`).
- **Command 1:** `06 01 00 0e 00 00 [55 53 43] 00 00 [55 53 43]`
  - OEM RIL IPC Header (`06 01`), Subcommand Length (`00 0e` = 14 bytes), dual sales code assignment (Active CSC + Configured CSC).
- **Command 2:** `06 01 00 09 00 00 [55 53 43]`
  - Primary network carrier partition selector (`00 09` = 9 bytes).
- **Command 3:** `06 01 00 12 00 00 [55 53 43] 00 00 [55 53 43] [55 53 43]`
  - Multi-CSC NVRAM write (Active + Home + Default Carrier triplet, `00 12` = 18 bytes).
- **Command 4:** `06 01 00 12 00 00`
  - Modem carrier commit & refresh trigger.
- **Command 5:** `06 01 00 09 00 00 [55 53 43]`
  - Final carrier sales code re-assertion.

---

## 5. WPF Desktop UI Architecture & Project Structure

### 5.1 Project Directory Tree
```
c:\Users\pc5\Documents\GitHub\gsmtools\
├── AT                                   <-- Native ARM64 ELF bridge binary
├── GsmTools.Desktop\
│   ├── GsmTools.Desktop.csproj          <-- net8.0-windows, UseWPF=true
│   ├── App.xaml
│   ├── App.xaml.cs
│   ├── Core\
│   │   ├── IAdbRawSocketClient.cs
│   │   ├── AdbRawSocketClient.cs        <-- Native TCP client :5037, sync: push, 0004usb:
│   │   ├── ImeiParcelParser.cs          <-- UTF-16LE parcel parser
│   │   ├── IPersistentAtBridgeSession.cs
│   │   ├── PersistentAtBridgeSession.cs <-- Interactive banner/prompt parser, auth, IPC
│   │   ├── Xorshift32StreamCipher.cs    <-- PRNG keystream transform
│   │   └── Models\
│   │       ├── DeviceInfo.cs
│   │       ├── WorkflowStepProgress.cs
│   │       ├── SecurityAuditReport.cs
│   │       └── LogEntry.cs
│   ├── Workflows\
│   │   ├── ISamsungCscWorkflow.cs
│   │   └── SamsungCscWorkflow.cs        <-- 8 pre-checks, UID 1000 escalation, 5 frames, USB reset
│   └── UI\
│       ├── Services\
│       │   ├── IConsoleLogger.cs
│       │   └── ConsoleLogger.cs         <-- Thread-safe Dispatcher log collector
│       ├── ViewModels\
│       │   ├── ViewModelBase.cs
│       │   ├── RelayCommand.cs
│       │   └── MainWindowViewModel.cs   <-- MVVM state & commands
│       ├── Converters\
│       │   ├── StatusToColorConverter.cs
│       │   ├── LogTypeToColorConverter.cs
│       │   └── BooleanToVisibilityConverter.cs
│       ├── Themes\
│       │   └── DarkTheme.xaml           <-- Sleek dark forensic styling & glowing badges
│       └── Views\
│           ├── MainWindow.xaml
│           └── MainWindow.xaml.cs
└── GsmTools.Desktop.Tests\
    ├── GsmTools.Desktop.Tests.csproj    <-- net8.0 xUnit test project
    ├── AdbFramingTests.cs
    ├── ImeiParcelParserTests.cs
    ├── Xorshift32CipherTests.cs
    ├── AdbSyncProtocolTests.cs
    ├── MockAdbServer.cs                 <-- In-process mock TCP listener
    └── SimulatedWorkflowTests.cs        <-- End-to-end simulated workflow test
```

### 5.2 Project File Configuration (`GsmTools.Desktop.csproj`)
```xml
<Project Sdk="Microsoft.NET.Sdk">

  <PropertyGroup>
    <OutputType>WinExe</OutputType>
    <TargetFramework>net8.0-windows</TargetFramework>
    <Nullable>enable</Nullable>
    <ImplicitUsings>enable</ImplicitUsings>
    <UseWPF>true</UseWPF>
    <ApplicationTitle>GsmTools.Desktop // Forensic CSC Injector</ApplicationTitle>
    <Version>1.0.0</Version>
    <Company>GsmTools</Company>
  </PropertyGroup>

  <ItemGroup>
    <!-- Automatic deployment of the native AT binary to build output -->
    <None Include="..\AT" Link="AT">
      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
    </None>
  </ItemGroup>

</Project>
```

### 5.3 UI Styling & Theme (`Themes/DarkTheme.xaml`)

Forensic Dark Palette:
- **Background Root:** `#0B0F19` (Deep Obsidian Void)
- **Card Surface:** `#111827` (Rich Slate Surface)
- **Card Border:** `#1F2937` with subtle `#374151` highlights
- **Primary Accent (Cyan Glow):** `#06B6D4` (Active buttons, progress highlights, prompt badges)
- **Success / Healthy:** `#10B981` (Enforcing, UID 1000 verified, `.OK..#`)
- **Warning / Alert:** `#F59E0B` (Permissive, RSU app detected)
- **Critical / Blocked:** `#EF4444` (Knox tsdaemon active, ADB FAIL)
- **Monospace Console:** `#030712` (Pitch Black background, `Cascadia Code` / `Consolas`, high contrast font `#E2E8F0`)

### 5.4 Live Forensic Console Integration
The `IConsoleLogger` provides structured logging:
- `Log(LogLevel level, string category, string message)`
- Automatically formats timestamp with millisecond precision: `[HH:mm:ss.fff]`
- Formats directional indicators: `[OUT ->]` in cyan, `[IN <-]` in neon green, `[ERR]` in crimson red.
- Thread-safe dispatching to the WPF `DispatcherObservableCollection<LogEntry>` using non-blocking batching or `Dispatcher.InvokeAsync`.

---

## 6. Testing Strategy & Mock Test Infrastructure

### 6.1 Testing ADB without a Physical Device (`MockAdbServer.cs`)

To achieve 100% CI/CD automation without requiring physical Samsung phones or USB cables:
- `MockAdbServer` is implemented using `System.Net.Sockets.TcpListener` on `127.0.0.1` listening on port `5037` (or an injected test port).
- It models the full ADB state machine:
  1. Handshake `host:version` $\to$ `OKAY00040029`.
  2. Device discovery `host:devices` $\to$ `OKAY` + formatted device table.
  3. Transport selection `host:transport:<serial>` $\to$ `OKAY`.
  4. Sync service `sync:` $\to$ responds to `STAT`, `SEND`, `DATA`, `DONE` (validating byte counts, octal mode `33188`, and SHA256 of received ELF).
  5. Shell commands $\to$ returns recorded outputs from `csc 15.txt` for `pm list`, `service call iphonesubinfo`, `getprop`, `getenforce`, and port 2600.
  6. USB reset `0004usb:` $\to$ validates command and outputs `restarting in USB mode`.

### 6.2 Test Matrix

| Test Suite | Target Component | Verification Objective |
|---|---|---|
| `AdbFramingTests` | `AdbRawSocketClient` | Validates 4-hex prefix formatting, lowercase/uppercase hex parsing, `OKAY`/`FAIL` parsing, and device list string parsing. |
| `ImeiParcelParserTests` | `ImeiParcelParser` | Verifies decoding against real dump from `csc 15.txt` producing exact IMEI `355752920542633`. Tests empty, malformed, and non-digit parcels. |
| `Xorshift32CipherTests` | `Xorshift32StreamCipher` | Validates zero-seed bypass, involution property ($A \oplus B \oplus B = A$), and tests against known byte-level keystream vectors. |
| `AdbSyncProtocolTests` | `AdbRawSocketClient` | Verifies `STAT` 16-byte layout, `SEND` mode string `/data/local/tmp/AT,33188`, 64KB chunking boundaries, and `DONE` verification. |
| `SimulatedWorkflowTests` | `SamsungCscWorkflow` | Runs full 8 pre-checks, port 2600 polling, file push, 5 injection frames, cleanup, and USB reset against `MockAdbServer`. |

---

## 7. Implementation Roadmap & Readiness Summary

The architecture is fully mapped and ready for immediate implementation by development agents:
1. All ADB packet layouts and binary formats are specified to the byte.
2. The IMEI parcel algorithm is mathematically proven against forensic evidence.
3. The Xorshift32 stream cipher is identical to the C implementation in `at_bridge.c`.
4. The WPF UI, MVVM design, and dark theme are completely structured.
5. The mock test infrastructure allows comprehensive unit and simulated end-to-end testing with zero physical hardware dependency.
