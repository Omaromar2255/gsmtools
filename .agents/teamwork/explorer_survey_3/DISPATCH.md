## 2026-10-16T23:58:24Z

You are explorer_survey_3 (archetype: teamwork_preview_explorer).
Your working directory is: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_survey_3\
Original request path: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\ORIGINAL_REQUEST.md
Parent conversation ID: 1e93a4d7-fd07-4705-a29e-755f41b77fe1

Your task:
Investigate and map the technical architecture, ADB protocol specifications, parcel parsing, and WPF desktop structure required for GsmTools.Desktop.

Specifically investigate and document in detail:
1. Native ADB TCP Socket Protocol:
   - Connecting directly to 127.0.0.1:5037 without adb.exe.
   - Framing: 4-hex-char ASCII length prefix, OKAY/FAIL responses, host:transport:<serial>, host:devices, host:version.
   - Shell execution: shell:<cmd> vs shell,v2,raw:<cmd> framing and streams.
   - ADB sync: protocol specification: STAT, SEND, DATA (chunks up to 64KB), DONE, QUIT packets. Exact byte layout, little-endian length/timestamp fields, mode strings (e.g. /data/local/tmp/AT,33188).
   - USB mode reset: 0004usb: command and response validation.
2. ImeiParcelParser:
   - Android binder parcel serialization format from `service call iphonesubinfo 1 s16 com.android.shell`.
   - Hex dump output format (0x00000000:, 0x00000010:, 0x00000020:).
   - Skipping the 8-byte parcel header (status code + string length), decoding UTF-16LE characters, extracting 15-digit decimal IMEI string.
3. WPF Desktop UI Architecture & Project Structure:
   - Target framework: net8.0-windows, UseWPF=true.
   - Clean architecture: Core (AdbRawSocketClient, ImeiParcelParser, PersistentAtBridgeSession, Xorshift32StreamCipher), Workflows (SamsungCscWorkflow, DeviceInfo, WorkflowStepProgress), UI (MainWindow, MainWindowViewModel, ConsoleLogger, Converters, Dark Theme styling).
   - Project configuration to copy `./AT` binary to output directory (PreserveNewest).
4. Testing strategy & Test Infra:
   - Mocking TCP sockets / ADB server in tests.
   - Unit tests for protocol framing, Xorshift32 cipher against known vectors, IMEI parcel parser against real dumps.
   - End-to-end simulated workflow testing without requiring a physical Android device.

Write your findings to:
- c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_survey_3\report.md
- c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_survey_3\handoff.md

When complete, update progress.md and send a message back to parent (1e93a4d7-fd07-4705-a29e-755f41b77fe1) with a concise summary and path to your handoff.md.
