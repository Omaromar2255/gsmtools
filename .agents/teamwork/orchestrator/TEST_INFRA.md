# E2E Test Infra: GsmTools.Desktop

## Test Philosophy
- Opaque-box, requirement-driven. No dependency on internal implementation details.
- Methodology: Category-Partition + Boundary Value Analysis (BVA) + Pairwise Combinatorial Testing + Real-World Workload Testing (replay of csc 15.txt).

## Feature Inventory & Test Coverage Matrix
| # | Feature | Source (requirement) | Tier 1 (>=5) | Tier 2 (>=5) | Tier 3 | Tier 4 |
|---|---------|---------------------|:------------:|:------------:|:------:|:------:|
| 1 | AdbTcpFraming | ORIGINAL_REQUEST §R1 | 5 | 5 | ✓ | ✓ |
| 2 | AdbTransportSwitch | ORIGINAL_REQUEST §R1 | 5 | 5 | ✓ | ✓ |
| 3 | AdbShellExecution | ORIGINAL_REQUEST §R1 | 5 | 5 | ✓ | ✓ |
| 4 | AdbSyncPush | ORIGINAL_REQUEST §R1 | 5 | 5 | ✓ | ✓ |
| 5 | AdbUsbReset | ORIGINAL_REQUEST §R1 | 5 | 5 | ✓ | ✓ |
| 6 | ImeiParcelParsing | ORIGINAL_REQUEST §R2 | 5 | 5 | ✓ | ✓ |
| 7 | Xorshift32Cipher | at_bridge.c, §R3 | 5 | 5 | ✓ | ✓ |
| 8 | AtBannerParsing | at_bridge.c, §R3 | 5 | 5 | ✓ | ✓ |
| 9 | AtAuthHandshake | at_bridge.c, §R3 | 5 | 5 | ✓ | ✓ |
| 10 | AtFramingDelimiters | at_bridge.c, §R3 | 5 | 5 | ✓ | ✓ |
| 11 | AtUidFallback | ORIGINAL_REQUEST §R3 | 5 | 5 | ✓ | ✓ |
| 12-19 | DevicePreChecks (1-8) | csc 15.txt, §R4 | 8 | 8 | ✓ | ✓ |
| 20 | CscValidation | ORIGINAL_REQUEST §R4 | 5 | 5 | ✓ | ✓ |
| 21 | Uid1000Recovery | csc 15.txt, §R4 | 5 | 5 | ✓ | ✓ |
| 22 | AtBinaryDeployment | csc 15.txt, §R4 | 5 | 5 | ✓ | ✓ |
| 23 | RadioLogcatFlush | csc 15.txt, §R4 | 5 | 5 | ✓ | ✓ |
| 24 | CscFiveCommandInjection | csc 15.txt, §R4 | 5 | 5 | ✓ | ✓ |
| 25 | PostCleanupOps | csc 15.txt, §R4 | 5 | 5 | ✓ | ✓ |
| 26 | CscUsbReset | csc 15.txt, §R4 | 5 | 5 | ✓ | ✓ |

## Test Architecture
- **Framework**: xUnit (`net8.0`), `Microsoft.NET.Test.Sdk`, `xunit.runner.visualstudio`.
- **Mock ADB Server**: In-memory TCP server listening on loopback (`127.0.0.1:0`), fully mimicking ADB server handshake (`000chost:version`, `0012host:transport:...`), shell streaming, and binary `sync:` (`STAT`, `SEND`, `DATA`, `DONE`, `QUIT`).
- **Mock AT Bridge**: In-memory TCP server or shell responder simulating `at_bridge` banner, prompt `<< `, `AUTH:<seed>`, Xorshift32 encrypted frames, and Delimiters `.OK..#`.
- **Directory**: `GsmTools.Desktop.Tests/`

## Real-World Application Scenarios (Tier 4)
| # | Scenario | Features Exercised | Complexity |
|---|----------|--------------------|------------|
| 1 | Full Device Telemetry Audit (Galaxy S23 Ultra) | F1, F2, F3, F6, F12-F19 | Medium |
| 2 | End-to-End CSC Change to XAA (Run 1 Playback) | F1-F10, F20, F22-F26 | High |
| 3 | End-to-End CSC Change to USC with Port 2600 Refusal & Recovery (Run 2 Playback) | F1-F11, F20-F26 | High |
| 4 | Encrypted AT Bridge Session with Non-Zero Seed | F7, F8, F9, F10, F24 | High |
| 5 | USB Disconnection & Re-enumeration Handling | F5, F26 | Medium |

## Coverage Thresholds
- Tier 1: ≥5 per feature
- Tier 2: ≥5 per feature
- Tier 3: Pairwise coverage of major feature interactions
- Tier 4: ≥5 realistic application scenarios
