# Original User Request

## Initial Request — 2026-10-16T23:49:53Z

Build a modern, forensic-grade C# (.NET 8 WPF) desktop application named `GsmTools.Desktop` to orchestrate device recognition, privilege escalation checks, and Samsung CSC (Carrier Sales Code) injection matching the exact forensic trace in `csc 15.txt`, integrating seamlessly with the custom `/AT` binary bridge.

Working directory: c:\Users\pc5\Documents\GitHub\gsmtools
Integrity mode: development

## Requirements

### R1. Raw ADB TCP Socket Client (`Core/AdbRawSocketClient.cs`)
- Direct TCP connection to ADB server (`127.0.0.1:5037`) without spawning `adb.exe` processes.
- Framed request handling (`SendAdbRequestAsync`) with 4-hex-char ASCII length prefixes and `OKAY`/`FAIL` parsing.
- Interactive and one-shot shell command execution (`ExecuteShellCommandAsync`) supporting both standard `shell:<command>` and `shell,v2,raw:<command>` framing.
- Native ADB `sync:` protocol implementation (`PushFileSyncAsync`) for pushing `/data/local/tmp/AT`:
  - `STAT` packet with 4-byte LE path length and path `/data/local/tmp/AT`, validating 16-byte `STAT` response.
  - `SEND` packet with path and mode `/data/local/tmp/AT,33188` (octal 0100644).
  - File chunking via `DATA` packets (up to 64KB per chunk: `DATA` + 4-byte LE length + payload).
  - Transfer termination via `DONE` packet with 4-byte LE timestamp, verifying `OKAY`, followed by `QUIT`.
- Direct ADB protocol USB mode reset (`RestartUsbModeAsync`) sending `0004usb:` and reading `restarting in USB mode`.

### R2. IMEI Parcel Parser (`Core/ImeiParcelParser.cs`)
- Parse hex dump output from `service call iphonesubinfo 1 s16 com.android.shell`.
- Parse lines starting with `0x00000000:`, `0x00000010:`, `0x00000020:`.
- Skip the first 8 bytes (Parcel status code and string length), extract 16-bit UTF-16LE characters into a clean 15-digit decimal IMEI string (e.g. `355752920542633`).

### R3. Persistent Interactive AT Bridge Session (`Core/PersistentAtBridgeSession.cs`)
- Maintain an open TCP/shell session to `/data/local/tmp/AT` (or fallback relay via `toybox nc localhost 2600`).
- Parse startup banner `INFO|MODEL=...|CSC=...|AND=...|PATCH=...|DBG=...|UID=...|CH=...` and wait for prompt `<< `.
- Smart UID 1000 fallback: If shell runs under `UID=2000` and channel is `CH=NONE` but port 2600 is available, relay session through `toybox nc localhost 2600` executing `/data/local/tmp/AT`.
- Support dual operation modes:
  - Raw Hex Mode: `Seed = 0`, sends raw hex IPC frames (`0601000e...`).
  - Encrypted Mode: Sends `AUTH:<8-Hex-Seed>`, verifies `.OK_AUTH..`, and applies Xorshift32 stream cipher matching `transform_buffer` in `at_bridge.c`.
- Send IPC hex packets and wait for `.OK..#` delimiter and prompt `<< `.

### R4. Samsung CSC Workflow Orchestrator (`Workflows/SamsungCscWorkflow.cs`)
- **ReadDeviceInfoAsync:**
  1. `pm list package|grep rsuapp` (Carrier RSU detection).
  2. `ps -A|grep tsdaemon;ps|grep tsdaemon` (Knox tsdaemon detection).
  3. `getprop` telemetry (Model, Serial, Current CSC, Product Code, Build Number, Android Version, Security Patch).
  4. `service call iphonesubinfo 1 s16 com.android.shell` -> IMEI via `ImeiParcelParser`.
  5. `getenforce` (SELinux Enforcing / Permissive).
  6. `pm path KEYTool.SRV4` (KEYTool presence check).
  7. `echo 'id' |toybox nc localhost 2600` (UID 1000 check).
  8. Diagnostic packages inspection (`repaircal`, `mobiledoctor`, `retailagent.global`).
- **ExecuteCscChangeAsync(targetCsc, useEncryptedAtSession):**
  1. Validate target CSC (3 letters, convert to ASCII hex e.g. `XAA` -> `584141`).
  2. Run initial device audit.
  3. UID 1000 bridge readiness: If port 2600 is not immediately available, execute `svc power stayon true`, inspect packages, and poll `echo 'id' |toybox nc localhost 2600` every 500ms (up to 30s) until `uid=1000(system)` is verified. Send `echo 'setprop ctl.stop ssud'|toybox nc localhost 2600`.
  4. Push `/AT` via `PushFileSyncAsync`, run `chmod 777 /data/local/tmp/AT`, and establish interactive session.
  5. Clear radio logcat: `logcat -b radio -c`.
  6. Transmit the 5-command CSC injection sequence:
     - Cmd 1: `0601000e0000{hexCsc}0000{hexCsc}`
     - Cmd 2: `060100090000{hexCsc}`
     - Cmd 3: `060100120000{hexCsc}0000{hexCsc}{hexCsc}`
     - Cmd 4: `060100120000`
     - Cmd 5: `060100090000{hexCsc}`
     - Send `EXIT`.
  7. Post-cleanup: `pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor`.
  8. USB reset: Direct `0004usb:` request via `RestartUsbModeAsync()`.

### R5. Modern Dark WPF Desktop UI (`MainWindow.xaml`)
- Dark Forensic theme styling (sleek dark palette, glowing status badges, high-contrast monospace console).
- Device Telemetry Card (Model, Serial, IMEI, CSC, Product Code, Android Ver, Build, Security Patch).
- Security & Privilege Status Badges (SELinux, RSU/tsdaemon, Port 2600 UID 1000, Retail Mode version, AT Channel).
- CSC Selection & Execution Panel (Dropdown with popular CSCs like XAA, USC, TMB, ATT, VZW, GCF, FKR; Encryption toggle; "Read Info" and "Change CSC" buttons; Step Progress Bar).
- Live Forensic Console (`OUT ->` and `IN <-` streams with millisecond timestamps `[HH:mm:ss.fff]`, colored badges, and export/copy log buttons).

## Verification Resources
- Forensic log: `csc 15.txt`
- AT bridge C source: `at-bridge/jni/at_bridge.c`
- Compiled binary: `./AT` and `at-bridge/libs/arm64-v8a/AT`

## Acceptance Criteria

### Build & Packaging
- [ ] `dotnet build GsmTools.Desktop\GsmTools.Desktop.csproj` succeeds with 0 errors and 0 warnings.
- [ ] Compiled binary `AT` is automatically copied to output directory (`CopyToOutputDirectory = PreserveNewest`).

### Protocol & Functional Accuracy
- [ ] `AdbRawSocketClient` handles ADB requests, sync push (`STAT`/`SEND`/`DATA`/`DONE`), and `usb:` reset over TCP socket without calling `adb.exe`.
- [ ] `ImeiParcelParser` accurately parses the IMEI from sample parcel data matching `355752920542633`.
- [ ] `PersistentAtBridgeSession` correctly implements Xorshift32 bitwise transformations matching `at_bridge.c`.
- [ ] `SamsungCscWorkflow` reproduces all 8 pre-checks, UID 1000 confirmation, the 5 injection frames, post-cleanup, and USB restart matching `csc 15.txt`.
- [ ] Dark Mode UI displays all telemetry fields, status badges, progress indicator, and live color-coded forensic log.
