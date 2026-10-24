# Universal Device Telemetry Methodology

Summary of Methodology from SKILL.md:
- Complete zero-loss, universal monitoring of mobile servicing and GSM tools.
- Captures 100% of raw USB, Serial/COM, AT commands, ADB socket protocol, and file transfers.
- Protocol Analysis: Length-prefixed ADB framing (4-hex-char), sync subprotocol (STAT/SEND/DATA/DONE/QUIT), interactive shell streams.
- AT Bridge analysis: seed-based encryption, dynamic stream cipher (Xorshift32), delimitation (.OK..#).
- Forensic replay matching csc 15.txt verbatim.
