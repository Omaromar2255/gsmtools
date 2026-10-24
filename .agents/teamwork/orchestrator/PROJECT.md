# Project: GsmTools.Desktop

## Architecture
`GsmTools.Desktop` is a forensic-grade C# (.NET 8 WPF) desktop application designed for mobile servicing, device telemetry acquisition, and Samsung CSC (Carrier Sales Code) injection matching the forensic trace in `csc 15.txt` and native AT bridge `at_bridge.c`.

### Architecture Layers
1. **Core Layer (`GsmTools.Desktop/Core/`)**:
   - `AdbRawSocketClient`: Direct TCP socket client communicating with ADB server at `127.0.0.1:5037` without spawning external processes. Implements length-prefixed ASCII framing, `OKAY`/`FAIL` parsing, `shell:` and `shell,v2,raw:` command streams, binary `sync:` push (`STAT`, `SEND`, `DATA` in 64KB chunks, `DONE`, `QUIT`), and `0004usb:` device reset.
   - `ImeiParcelParser`: Decodes Binder parcel hex dumps from `service call iphonesubinfo 1 s16 com.android.shell`, discarding the 8-byte status/length header and decoding 16-bit UTF-16LE characters into a clean 15-digit decimal IMEI string.
   - `Xorshift32StreamCipher`: Implements the exact Marsaglia [13, 17, 5] stream cipher matching `transform_buffer` in `at_bridge.c`. Supports dynamic seeding and per-packet state reinitialization.
   - `PersistentAtBridgeSession`: Manages an interactive shell/TCP session to `/data/local/tmp/AT` (or relay via `toybox nc localhost 2600`). Parses the startup banner `INFO|...`, prompt `<< `, executes `AUTH:<seed>`, formats raw or encrypted IPC frames, and handles `.OK..#` delimiters.

2. **Workflows Layer (`GsmTools.Desktop/Workflows/`)**:
   - `SamsungCscWorkflow`: Orchestrates device telemetry and CSC change.
     - `ReadDeviceInfoAsync`: 8 pre-checks (RSU app, Knox tsdaemon, getprop telemetry, IMEI parcel parse, SELinux enforcement, KEYTool.SRV4 presence, port 2600 UID 1000 check, and diagnostic packages).
     - `ExecuteCscChangeAsync`: CSC validation (3 uppercase letters -> ASCII hex), initial audit, UID 1000 readiness recovery (`svc power stayon true`, polling port 2600, stopping `ssud`), push `./AT` via sync, `chmod 777`, interactive session, `logcat -b radio -c`, 5-command CSC injection sequence, post-cleanup (uninstall/clear), and `0004usb:` reset.
   - Models: `DeviceInfo`, `WorkflowStepProgress`, `CscChangeResult`.

3. **UI Layer (`GsmTools.Desktop/UI/` & Root WPF)**:
   - MVVM architecture: `MainWindowViewModel`, `ViewModelBase`, `RelayCommand`.
   - `MainWindow.xaml`: Sleek dark forensic styling (`#12131A` background, glowing accents), telemetry cards, security badges, CSC dropdown with popular carriers (XAA, USC, TMB, ATT, VZW, GCF, FKR), encryption toggle, progress bar, and millisecond-precision forensic console (`OUT ->` and `IN <-` logs).

4. **Testing Infrastructure (`GsmTools.Desktop.Tests/`)**:
   - `MockAdbServer`: In-memory TCP server listening on loopback, simulating ADB protocol, sync push, shell command execution, and USB reset.
   - Opaque-box test suites covering Tiers 1-4 and Tier 5 adversarial tests.

---

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | AdbTcpFraming | 4-hex-char ASCII length prefix & OKAY/FAIL parsing over TCP 5037 | M1 | ORIGINAL_REQUEST §R1 |
| 2 | AdbTransportSwitch | `host:transport:<serial>` transport switching | M1 | ORIGINAL_REQUEST §R1 |
| 3 | AdbShellExecution | Standard `shell:<cmd>` and `shell,v2,raw:<cmd>` command execution | M1 | ORIGINAL_REQUEST §R1 |
| 4 | AdbSyncPush | Binary `sync:` protocol: STAT, SEND (path,mode), DATA (64KB chunks), DONE, QUIT | M1 | ORIGINAL_REQUEST §R1 |
| 5 | AdbUsbReset | Direct `0004usb:` request reading `restarting in USB mode` | M1 | ORIGINAL_REQUEST §R1 |
| 6 | ImeiParcelParsing | Parcel hex dump decoding, skipping 8B header, UTF-16LE to 15-digit IMEI | M1 | ORIGINAL_REQUEST §R2 |
| 7 | Xorshift32Cipher | Exact Marsaglia [13, 17, 5] stream cipher with per-buffer seed reset | M1 | at_bridge.c, ORIGINAL_REQUEST §R3 |
| 8 | AtBannerParsing | Parsing `INFO\|MODEL=...\|CSC=...\|AND=...\|PATCH=...\|DBG=...\|UID=...\|CH=...` & prompt `<< ` | M1 | at_bridge.c, ORIGINAL_REQUEST §R3 |
| 9 | AtAuthHandshake | `AUTH:<seed>` handshake returning `.OK_AUTH..\n<< ` | M1 | at_bridge.c, ORIGINAL_REQUEST §R3 |
| 10 | AtFramingDelimiters | Delimiters `.OK..#` and `.OK..<HEX>#` handling | M1 | at_bridge.c, ORIGINAL_REQUEST §R3 |
| 11 | AtUidFallback | Smart fallback via `toybox nc localhost 2600` when UID=2000 and CH=NONE | M1 | ORIGINAL_REQUEST §R3 |
| 12 | DevicePreCheckRsu | Check 1: `pm list package\|grep rsuapp` | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 13 | DevicePreCheckTsdaemon | Check 2: `ps -A\|grep tsdaemon;ps\|grep tsdaemon` | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 14 | DevicePreCheckGetprop | Check 3: Querying model, serial, CSC, product code, build, version, security patch | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 15 | DevicePreCheckImei | Check 4: Querying `service call iphonesubinfo 1 s16 com.android.shell` and parsing | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 16 | DevicePreCheckSelinux | Check 5: `getenforce` (Enforcing vs Permissive) | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 17 | DevicePreCheckKeytool | Check 6: `pm path KEYTool.SRV4` presence | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 18 | DevicePreCheckPort2600 | Check 7: `echo 'id' \|toybox nc localhost 2600` | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 19 | DevicePreCheckDiagnostics | Check 8: Dumpsys inspection of `repaircal`, `mobiledoctor`, `retailagent.global` | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 20 | CscValidation | Validate 3-char target CSC and convert to ASCII hex (e.g. XAA -> 584141) | M2 | ORIGINAL_REQUEST §R4 |
| 21 | Uid1000Recovery | `svc power stayon true`, 500ms polling on port 2600, stop ssud | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 22 | AtBinaryDeployment | Push `./AT` via sync, `chmod 777 /data/local/tmp/AT`, start interactive session | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 23 | RadioLogcatFlush | Execute `logcat -b radio -c` before injection | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 24 | CscFiveCommandInjection | Transmit 5 hex frames with injected CSC, verify `.OK..#`, send EXIT | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 25 | PostCleanupOps | Non-fatal uninstall AutoPreconfig, omadmspr, clear mobiledoctor | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 26 | CscUsbReset | Trigger `RestartUsbModeAsync` after injection | M2 | csc 15.txt, ORIGINAL_REQUEST §R4 |
| 27 | DarkWpfTheme | Dark forensic palette, typography, glowing badges | M3 | ORIGINAL_REQUEST §R5 |
| 28 | TelemetryCardUI | Display Model, Serial, IMEI, CSC, Product Code, Android, Build, Security Patch | M3 | ORIGINAL_REQUEST §R5 |
| 29 | StatusBadgesUI | Badges for SELinux, RSU, tsdaemon, Port 2600 UID 1000, Retail Mode, AT Channel | M3 | ORIGINAL_REQUEST §R5 |
| 30 | CscSelectionPanelUI | Dropdown (XAA, USC, TMB, ATT, VZW, GCF, FKR), Encryption toggle, Action buttons, Progress | M3 | ORIGINAL_REQUEST §R5 |
| 31 | LiveConsoleUI | Monospace console with `OUT ->` / `IN <-`, millisecond timestamps, export/copy | M3 | ORIGINAL_REQUEST §R5 |
| 32 | BuildAndPackaging | `net8.0-windows` csproj, 0 errors, 0 warnings, `./AT` copied PreserveNewest | M3 | ORIGINAL_REQUEST Acceptance |
| 33 | E2ETestSuite | Mock ADB Server, Tiers 1-4 comprehensive tests passing 100% | M4 | ORIGINAL_REQUEST Acceptance |
| 34 | AdversarialHardening | Tier 5 adversarial stress testing and coverage hardening | M4 | Project Pattern |

---

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Core Protocols & Crypto | `AdbRawSocketClient`, `ImeiParcelParser`, `Xorshift32StreamCipher`, `PersistentAtBridgeSession`, `GsmTools.Desktop.csproj` | none | PLANNED |
| M2 | Samsung CSC Workflow | `SamsungCscWorkflow`, `DeviceInfo`, `WorkflowStepProgress`, 8 pre-checks, UID 1000 readiness, 5-frame injection, cleanup, reset | M1 | PLANNED |
| M3 | Modern Dark WPF UI & Packaging | `MainWindow.xaml`, `MainWindowViewModel`, dark theme styling, console logging, build packaging, 0 warnings/errors | M2 | PLANNED |
| M4 | E2E Testing & Hardening | Full E2E suite (Tiers 1-4) via `MockAdbServer`, pass 100%, followed by Tier 5 adversarial coverage hardening | M3 & E2E Track | PLANNED |

---

## Code Layout
```
c:\Users\pc5\Documents\GitHub\gsmtools\
├── AT                                      # Compiled arm64 binary
├── at-bridge\                              # Reference C source
├── csc 15.txt                              # Forensic trace reference
├── GsmTools.Desktop\                       # WPF Desktop Project (.NET 8)
│   ├── GsmTools.Desktop.csproj
│   ├── App.xaml
│   ├── App.xaml.cs
│   ├── MainWindow.xaml
│   ├── MainWindow.xaml.cs
│   ├── Core\
│   │   ├── AdbRawSocketClient.cs
│   │   ├── ImeiParcelParser.cs
│   │   ├── Xorshift32StreamCipher.cs
│   │   └── PersistentAtBridgeSession.cs
│   ├── Workflows\
│   │   ├── SamsungCscWorkflow.cs
│   │   └── Models\
│   │       ├── DeviceInfo.cs
│   │       ├── WorkflowStepProgress.cs
│   │       └── CscChangeResult.cs
│   ├── ViewModels\
│   │   ├── ViewModelBase.cs
│   │   ├── RelayCommand.cs
│   │   └── MainWindowViewModel.cs
│   └── UI\
│       ├── Converters\
│       ├── Styles\
│       │   └── DarkTheme.xaml
│       └── Controls\
│           └── ConsoleLogItem.cs
└── GsmTools.Desktop.Tests\                 # Test Project (.NET 8)
    ├── GsmTools.Desktop.Tests.csproj
    ├── Mocks\
    │   ├── MockAdbServer.cs
    │   └── MockAtBridge.cs
    ├── Unit\
    │   ├── AdbClientTests.cs
    │   ├── ImeiParserTests.cs
    │   ├── XorshiftCipherTests.cs
    │   └── AtBridgeSessionTests.cs
    ├── Workflow\
    │   └── SamsungCscWorkflowTests.cs
    └── E2E\
        ├── Tier1_FeatureCoverageTests.cs
        ├── Tier2_BoundaryTests.cs
        ├── Tier3_PairwiseTests.cs
        ├── Tier4_RealWorldScenarioTests.cs
        └── Tier5_AdversarialTests.cs
```

---

## Interface Contracts

### 1. `AdbRawSocketClient`
```csharp
namespace GsmTools.Desktop.Core;

public interface IAdbRawSocketClient : IAsyncDisposable
{
    Task ConnectAsync(string host = "127.0.0.1", int port = 5037, CancellationToken cancellationToken = default);
    Task<string> SendAdbRequestAsync(string request, CancellationToken cancellationToken = default);
    Task SelectDeviceAsync(string? serial = null, CancellationToken cancellationToken = default);
    Task<string> ExecuteShellCommandAsync(string command, bool useV2 = false, CancellationToken cancellationToken = default);
    Task PushFileSyncAsync(string localPath, string remotePath, int permissions = 33188, Action<long, long>? progressCallback = null, CancellationToken cancellationToken = default);
    Task<string> RestartUsbModeAsync(CancellationToken cancellationToken = default);
    bool IsConnected { get; }
}
```

### 2. `ImeiParcelParser`
```csharp
namespace GsmTools.Desktop.Core;

public static class ImeiParcelParser
{
    public static string ParseImeiFromParcelDump(string rawParcelOutput);
}
```

### 3. `Xorshift32StreamCipher`
```csharp
namespace GsmTools.Desktop.Core;

public static class Xorshift32StreamCipher
{
    public static void TransformBuffer(byte[] buffer, int offset, int length, uint seed);
    public static byte[] Transform(byte[] data, uint seed);
}
```

### 4. `PersistentAtBridgeSession`
```csharp
namespace GsmTools.Desktop.Core;

public record AtBridgeBanner(string Model, string Csc, string AndroidVersion, string SecurityPatch, string DebugLevel, int Uid, string ActiveChannel);

public interface IPersistentAtBridgeSession : IAsyncDisposable
{
    Task<AtBridgeBanner> StartSessionAsync(string? serial = null, uint sessionSeed = 0, CancellationToken cancellationToken = default);
    Task<string> SendIpcHexCommandAsync(string hexCommand, CancellationToken cancellationToken = default);
    Task TerminateSessionAsync(CancellationToken cancellationToken = default);
    AtBridgeBanner? Banner { get; }
    uint SessionSeed { get; }
    bool IsActive { get; }
}
```

### 5. `SamsungCscWorkflow`
```csharp
namespace GsmTools.Desktop.Workflows;

public interface ISamsungCscWorkflow
{
    Task<DeviceInfo> ReadDeviceInfoAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<CscChangeResult> ExecuteCscChangeAsync(string targetCsc, bool useEncryptedAtSession = false, Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}
```
