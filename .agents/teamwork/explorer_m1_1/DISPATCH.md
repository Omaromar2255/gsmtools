## 2026-10-17T00:34:42Z
You are explorer_m1_1 (archetype: teamwork_preview_explorer).
Your working directory is: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_m1_1\
Original request path: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\ORIGINAL_REQUEST.md
Scope document: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\orchestrator\PROJECT.md
Parent conversation ID: 1e93a4d7-fd07-4705-a29e-755f41b77fe1

Your mission:
Investigate and design the C# implementation blueprint for `AdbRawSocketClient.cs` and `IAdbRawSocketClient.cs` (Milestone 1 Core):
1. Connection management to 127.0.0.1:5037 via raw System.Net.Sockets.TcpClient / Socket.
2. Framed request handling: `SendAdbRequestAsync` with 4-hex-char ASCII length prefix and OKAY/FAIL parsing.
3. Transport switching: `host:transport:<serial>`.
4. Command execution: `ExecuteShellCommandAsync` supporting standard `shell:<cmd>` and `shell,v2,raw:<cmd>` streams.
5. Native `sync:` file push: `PushFileSyncAsync` for `/data/local/tmp/AT`:
   - `STAT` packet with 4-byte LE length and path, validating 16-byte response.
   - `SEND` packet with path and mode `/data/local/tmp/AT,33188` (octal 0100644).
   - `DATA` chunking up to 64KB (65536 bytes): `DATA` + 4-byte LE length + payload bytes.
   - `DONE` packet with 4-byte LE timestamp, verifying `OKAY`, followed by `QUIT`.
6. Device reset: `RestartUsbModeAsync` sending `0004usb:` and reading `restarting in USB mode`.

Deliverables:
- c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_m1_1\report.md
- c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\explorer_m1_1\handoff.md
Send a completion message back to parent when done.
