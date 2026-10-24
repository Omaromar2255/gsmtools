# DISPATCH: Survey Spec Miner 1 (AT Bridge & Crypto)
Working directory: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\spec_miner_survey_1\

## 2026-10-16T23:58:23Z
You are spec_miner_survey_1 (archetype: teamwork_preview_spec_miner).
Your working directory is: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\spec_miner_survey_1\
Original request path: c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\ORIGINAL_REQUEST.md
Parent conversation ID: 1e93a4d7-fd07-4705-a29e-755f41b77fe1

Your task:
Perform a deep-dive specification mining of the AT Bridge source code at:
c:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\at_bridge.c
and related files in at-bridge.

Specifically investigate and document in detail:
1. Startup banner string format, fields (INFO|MODEL=...|CSC=...|AND=...|PATCH=...|DBG=...|UID=...|CH=...), and prompt characters (e.g. `<< `).
2. The exact authentication protocol: command format (AUTH:<8-Hex-Seed>), seed interpretation, response (.OK_AUTH..), error conditions.
3. The exact Xorshift32 stream cipher algorithm and state in transform_buffer, state transitions, key generation, and byte-by-byte or chunk transformations. Verify exact bitwise operations, integer overflow/wrapping semantics in C and equivalent in C#.
4. Command and packet framing: raw hex mode (Seed = 0) vs encrypted mode (Seed != 0), delimiters (.OK..#, << ), IPC command formatting, EXIT command.
5. Smart fallback / relay mechanisms: toybox nc localhost 2600 executing /data/local/tmp/AT, socket handling, UID checks.

Write your findings to:
- c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\spec_miner_survey_1\report.md
- c:\Users\pc5\Documents\GitHub\gsmtools\.agents\teamwork\spec_miner_survey_1\handoff.md

When complete, update progress.md and send a message back to parent (1e93a4d7-fd07-4705-a29e-755f41b77fe1) with a concise summary and path to your handoff.md.
