# GsmTools AT Bridge (v4.0)

On-device bridge binary pushed to `/data/local/tmp/AT` by the desktop workflows.
Single C source (`jni/at_bridge.c`) + comprehensive command dictionary
(`jni/all_gsm_commands.h`). Builds fully static — no runtime dependencies.

## Channels (priority order)

| # | Channel | Path / mechanism |
|---|---------|-------------------|
| 1 | `CH_SECRIL_LIB` | `dlopen("libsecril-client.so")` → `send_recv_cmd` (strongest) |
| 2 | `CH_DEV_IPC_RAW` | `/dev/umts_ipc0` → `umts_ipc1` → `umts_rfs0` → `/dev/diag` |
| 3 | `CH_RILD_PARCEL` | `/dev/socket/rild{,-oem,-debug,2}` + OEM-hook parcel (req 59) |
| 4 | `CH_DEV_AT_TTY` | `/dev/umts_atc0` → `atc1` → `at0` → `at_distributor[0]` → `smd11` → `smd0` → `smd7` → `ttyGS0` → `ttyACM0` → `ttyUSB0` |
| 5 | `CH_TCP_RELAY_2600` | TCP `127.0.0.1:2600` (UID-1000 companion relay) |

## Build

```bash
./build.sh            # all ABIs (arm64-v8a, armeabi-v7a) + host-linux test build
./build.sh host       # host test build only (gcc)
./build.sh deploy     # build + adb push to /data/local/tmp/AT
```

Outputs land in `payloads/bin/<abi>/` with primary copies at `payloads/AT`
(arm64) and `payloads/AT_32` (arm). Windows: `build.bat`.

## Session protocol (consumed by GsmTools.Desktop + python tools)

```
INFO|MODEL=SM-S928U|CSC=XAU|AND=14|PATCH=2025-08-01|DBG=0x494C|UID=2000|CH=/dev/umts_atc0
<< BOOST
.OK_BOOST..
<< CSC:XAA
.OK_CSC..
<< 0601000E0000584141000058414 1     (raw hex IPC, optionally AUTH:-encrypted)
.OK..<RESP_HEX>#
```

Machine markers (never change without updating the C# client):
`.OK..%s#` `.OK..#` `.OK_CSC..` `.OK_AUTH..` `.OK_BOOST..` `.OK_RELOCK..`
`.OK_RADIO..` `.ERR_IPC..` `.ERR_SYNTAX..` `.ERR_HEX_SYNTAX..`
`.ERR_CSC_INVALID..` `.ERR_RELOCK_INVALID..` `.ERR_UNKNOWN_ACTION..`
`.ERR_AT_TIMEOUT..` `.TIMEOUT..` `.CH=%s..` plus pipe-lines
`SIMLOCK|...`, `DEVINFO|...`, `AKSEED|...`, `PROPS|...`, `VERSION|...`.

## Session cipher

`AUTH:<SEED_HEX>` enables Xorshift32 [13,17,5] over HEX IPC payloads and their
responses (symmetric — same transform encrypts/decrypts; state resets per
buffer). AT text commands stay readable by design.

## Host testing

The same source compiles on desktop Linux (`./build.sh host`) with a small
compat shim (`jni/gt_host_compat.h`): system properties are read from the
file named by `$GT_PROPS_FILE`. This is what the mock-ADB end-to-end tests
(`tests/`) use to validate the full session protocol without a phone.
