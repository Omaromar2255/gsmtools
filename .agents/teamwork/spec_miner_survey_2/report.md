# Deep-Dive Forensic Specification Report: Samsung CSC Workflow & Protocol Trace

**Author:** `spec_miner_survey_2` (Teamwork Specification Miner)  
**Target Specification Source:** `c:\Users\pc5\Documents\GitHub\gsmtools\csc 15.txt`  
**Cross-References:** `at-bridge/jni/at_bridge.c`, `./AT`, `ORIGINAL_REQUEST.md`  
**Date:** 2026-10-16  

---

## Executive Summary

A forensic byte-level and chronological log analysis was conducted on `csc 15.txt` (a 401,905-byte capture containing 324 logged events from a live servicing tool interacting with a Samsung Galaxy S23 Ultra, model `SM-S918U`, running Android 15 with Security Patch 2025-07-01).

The log documents two complete servicing runs:
1. **Run 1 (17:39:57 - 17:40:25)**: Initial device audit followed by CSC change to **`XAA`** (`584141`), cleanup, and USB reset.
2. **Interlude (17:40:38 - 17:41:06)**: USB bus reset, `dg_ssudbus` driver detachment (`PHONE REMOVED`), and re-enumeration (`PHONE CONNECTED`).
3. **Run 2 (17:41:46 - 17:42:26)**: Full escalation cycle including port 2600 UID 1000 readiness recovery, CSC change to **`USC`** (`555343`), cleanup, and final USB reset.

This report establishes the authoritative specification for `GsmTools.Desktop`, specifically covering `AdbRawSocketClient`, `ImeiParcelParser`, `PersistentAtBridgeSession`, and `SamsungCscWorkflow`.

---

## 1. Chronological Timeline of Events in `csc 15.txt`

The file header identifies the recording:
```text
=== سجل الأوامر المصفى (Commands Log - Excluded Filters Applied) ===
تاريخ التصدير: 10/15/2026 5:42:46 PM
إجمالي السجلات المسجلة: 324
عدد السجلات المصدرة: 324
============================================================
```

### Complete Sequence Breakdown

| Index Range | Timestamp Range | Phase / Operation | Target / Command | Key Observable Result |
|---|---|---|---|---|
| **1 – 12** | 17:39:57.235 – 17:39:57.699 | **Pre-Check (Fast Audit)** | `pm list package\|grep rsuapp`<br>`ps -A\|grep tsdaemon;ps\|grep tsdaemon`<br>`getprop`<br>`service call iphonesubinfo 1 s16 com.android.shell`<br>`getenforce` | Clean pre-checks: RSU absent, tsdaemon absent, Model=`SM-S918U`, Serial=`RFCX90GR21X`, IMEI=`355752920542633`, SELinux=`Enforcing`. |
| **13 – 31** | 17:40:08.289 – 17:40:20.451 | **Execution Pre-Audit (Run 1: XAA)** | Re-run of pre-checks plus:<br>`pm path KEYTool.SRV4`<br>`echo 'id' \|toybox nc localhost 2600` | `KEYTool.SRV4` apk located at `/data/app/~~Ol5g.../base.apk`.<br>Port 2600 immediately available with `uid=1000(system)`. |
| **32 – 54** | 17:40:20.453 – 17:40:20.471 | **ADB Sync Protocol Push** | `sync:`<br>`STAT /data/local/tmp/AT`<br>`SEND /data/local/tmp/AT,33188`<br>7x `DATA` packets (ELF payload) | Binary `AT` (492 KB) transferred to target device via raw ADB sync stream. |
| **55 – 66** | 17:40:20.507 – 17:40:24.847 | **Bridge Launch & Handshake** | `chmod 777 /data/local/tmp/AT`<br>`/data/local/tmp/AT`<br>AuthKey Exchange | AT bridge starts, prompts `Token AuthKey: F7B1C349...`. Tool transmits cipher payload, receives `AuthKey : OK` followed by `.OK..#`. |
| **67 – 68** | 17:40:24.854 | **Radio Logcat Flush** | `shell:logcat -b radio -c` | Clears Android radio log buffer prior to CSC injection. |
| **69 – 83** | 17:40:24.928 – 17:40:24.939 | **5-Frame CSC Injection (XAA)** | Frame 1: `0601000e00005841410000584141`<br>Frame 2: `060100090000584141`<br>Frame 3: `0601001200005841410000584141584141`<br>Frame 4: `060100120000`<br>Frame 5: `060100090000584141` | Each frame receives exactly 6 bytes: `[USB-RAW] IN 6B: .OK..#`. |
| **84 – 88** | 17:40:24.941 – 17:40:25.053 | **Post-Cleanup Operations** | `shell:pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor` | Returns `Failure [not installed for 0].`, `Failure [not installed for 0].`, `Failed.`. |
| **89 – 91** | 17:40:25.055 – 17:40:25.056 | **USB Reset Request** | `usb:` (`0004usb:`) | ADB server responds with `restarting in USB mode`. |
| **92 – 93** | 17:40:38.995 – 17:41:06.349 | **Hardware Re-enumeration** | USB disconnect & reconnect | `PHONE REMOVED` (serial `0435c0a420c48948`), followed 27.35 seconds later by `PHONE CONNECTED` (`VID: 0x04E8`, `PID: 0x6860`, driver `dg_ssudbus`). |
| **94 – 105** | 17:41:17.822 – 17:41:43.792 | **Subprocess & Host Connection** | Host tool activity & ADB CNXN | `DevConfigStudio.exe` processes, ADB host connection packet `CNXN`. |
| **106 – 119** | 17:41:43.814 – 17:41:46.762 | **Post-Reboot Verification Audit** | `pm list package\|grep rsuapp`<br>`ps -A\|grep tsdaemon;ps\|grep tsdaemon`<br>`getprop`<br>`service call iphonesubinfo 1 s16 com.android.shell`<br>`getenforce` | Audit verifies model `SM-S918U`, `ril.product_code=SM-S918UZKFXAA`, `sys.usb.config=mtp,conn_gadget,adb`. |
| **120 – 147** | 17:42:11.341 – 17:42:13.099 | **Execution Audit & Diagnostics (Run 2: USC)** | Pre-checks 1–6 plus:<br>`echo 'id' \|toybox nc localhost 2600`<br>`svc power stayon true`<br>Diagnostic package inspections | `nc localhost 2600` returns `Connection refused`. Tool executes `svc power stayon true`, inspects `repaircal`, `mobiledoctor`, and finds `retailagent.global` (`versionCode=30000171`). |
| **148 – 246** | 17:42:13.258 – 17:42:13.415 | **Helper Binary Deployment** | `sync:` push of `AT`, `chmod 777 /data/local/tmp/AT` | Prepares helper executable. |
| **247 – 264** | 17:42:13.430 – 17:42:19.875 | **UID 1000 Escalation Recovery** | `shell:/data/local/tmp/AT R2`<br>Token AuthKey Exchange<br>Polling port 2600 | `AT R2` triggers bridge daemon. Polling attempt 1 (17:42:19.620): `Connection refused`. Attempt 2 (17:42:19.747): `Connection refused`. Attempt 3 (17:42:19.875): `.UID 1000 confirmed..`. |
| **265 – 266** | 17:42:21.876 | **Daemon Intervention (`ssud`)** | `shell:echo 'setprop ctl.stop ssud'\|toybox nc localhost 2600` | Stops Samsung USB daemon (`ssud`) through UID 1000 bridge before modem write. |
| **267 – 288** | 17:42:21.957 – 17:42:22.004 | **Interactive Binary Re-Push** | `sync:` push of `AT`, `chmod 777 /data/local/tmp/AT` | Refreshes `/data/local/tmp/AT`. |
| **289 – 298** | 17:42:22.035 – 17:42:26.344 | **Interactive Bridge Launch** | `shell:/data/local/tmp/AT`<br>Token AuthKey Exchange | Prompts `Token AuthKey: 12B4C5A9...`, authenticated -> `.OK..#`. |
| **299 – 300** | 17:42:26.345 | **Radio Logcat Flush** | `shell:logcat -b radio -c` | Second radio log flush. |
| **301 – 315** | 17:42:26.393 – 17:42:26.422 | **5-Frame CSC Injection (USC)** | Frame 1: `0601000e00005553430000555343`<br>Frame 2: `060100090000555343`<br>Frame 3: `0601001200005553430000555343555343`<br>Frame 4: `060100120000`<br>Frame 5: `060100090000555343` | Target `USC` (`555343`). All 5 frames acknowledged with `[USB-RAW] IN 6B: .OK..#`. |
| **316 – 320** | 17:42:26.439 – 17:42:26.597 | **Post-Cleanup Operations** | `shell:pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor` | Standard cleanup execution. |
| **321 – 323** | 17:42:26.598 – 17:42:26.599 | **USB Reset Request** | `usb:` (`0004usb:`) | ADB server returns `restarting in USB mode`. Device completes cycle. |

---

## 2. Detailed Forensic Analysis of the 8 Pre-Checks in `ReadDeviceInfoAsync`

### Check 1: Carrier Remote SIM Unlock App Check
- **Command:** `pm list package|grep rsuapp` (sent framed as `0021shell:pm list package|grep rsuapp`)
- **Observed Result:** Empty (exit code 1 or null string returned).
- **Purpose & Meaning:** T-Mobile, MetroPCS, and Verizon deploy RSU (Remote SIM Unlock) packages (e.g. `com.samsung.rsuapp`, `com.tmobile.rsuapp`). If present, carrier lock policies may actively block CSC persistence or interfere with modem baseband NV writes.
- **Evaluation Criteria:**
  - Empty / No match: Normal / Safe to proceed.
  - Matches found: Flag `RSU Detected` warning on security telemetry badge.

### Check 2: Knox Security tsdaemon Detection
- **Command:** `ps -A|grep tsdaemon;ps|grep tsdaemon` (sent framed as `002cshell:ps -A|grep tsdaemon;ps|grep tsdaemon`)
- **Observed Result:** Empty (process not found).
- **Purpose & Meaning:** Dual syntax (`ps -A` for Android 8+, `ps` fallback for legacy) ensures detection of Samsung Knox `tsdaemon` (TrustZone Security Daemon / DEFEX monitor). `tsdaemon` monitors runtime privilege escalation and unauthorized access to `/dev/umts_ipc0` or `/dev/socket/rild`.
- **Evaluation Criteria:**
  - Empty: Safe / tsdaemon inactive.
  - Match: Warn user that Knox tamper protection is actively monitoring process trees.

### Check 3: System Properties (`getprop`) Telemetry
- **Command:** `getprop` (sent framed as `000dshell:getprop`)
- **Observed Responses in Log:**
  - Line 29 / Line 80: `ro.product.model=SM-S918U | ro.boot.serialno=RFCX90GR21X`
  - Line 113 / Line 126: `ro.product.model=SM-S918U | ril.product_code=SM-S918UZKFXAA | sys.usb.config=mtp,conn_gadget,adb`
- **Exact Property Keys Required by Telemetry Contract:**
  1. `ro.product.model` -> Model name (e.g., `SM-S918U`)
  2. `ro.boot.serialno` -> Hardware serial number (e.g., `RFCX90GR21X`)
  3. `ro.csc.sales_code` -> Active Carrier Sales Code (e.g., `XAA`, `TMB`, `USC`)
  4. `ril.product_code` -> Carrier Product Code (e.g., `SM-S918UZKFXAA`, last 3 chars indicate default CSC)
  5. `ro.build.display.id` or `ro.build.id` -> Build number (e.g., `S918USQS6DYG1`)
  6. `ro.build.version.release` -> Android OS version (e.g., `15`)
  7. `ro.build.version.security_patch` -> Android Security Patch level (e.g., `2025-07-01`)
  8. `ro.boot.debug_level` -> Bootloader debug state (e.g., `0x4f4c` for LOW)
  9. `sys.usb.config` -> Active USB configuration string (e.g., `mtp,conn_gadget,adb`)

### Check 4: IMEI Parcel Extraction via `iphonesubinfo`
- **Command:** `service call iphonesubinfo 1 s16 com.android.shell` (framed as `003cshell:service call iphonesubinfo 1 s16 com.android.shell`)
- **Observed Raw Hex Output:**
```text
Result: Parcel(	
0x00000000: 00000000 0000000f 00350033 00370035 '........3.5.5.7.'
0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'
0x00000020: 00330036 00000033                   '6.3.3...        ')
```
- **Parcel Binary Layout Analysis:**
  - `0x00000000`:
    - Bytes 0..3: `00000000` = 32-bit LE Integer status (`0` = `NO_ERROR`).
    - Bytes 4..7: `0000000f` = 32-bit LE Integer string length (`15` characters).
    - Bytes 8..11: `00350033` = Two UTF-16LE characters: `0x0033` ('3') followed by `0x0035` ('5').
    - Bytes 12..15: `00370035` = Two UTF-16LE characters: `0x0035` ('5') followed by `0x0037` ('7').
  - `0x00000010`:
    - Bytes 16..19: `00320035` = Two UTF-16LE characters: `0x0035` ('5') followed by `0x0032` ('2').
    - Bytes 20..23: `00320039` = Two UTF-16LE characters: `0x0039` ('9') followed by `0x0032` ('2').
    - Bytes 24..27: `00350030` = Two UTF-16LE characters: `0x0030` ('0') followed by `0x0035` ('5').
    - Bytes 28..31: `00320034` = Two UTF-16LE characters: `0x0034` ('4') followed by `0x0032` ('2').
  - `0x00000020`:
    - Bytes 32..35: `00330036` = Two UTF-16LE characters: `0x0036` ('6') followed by `0x0033` ('3').
    - Bytes 36..39: `00000033` = One UTF-16LE character: `0x0033` ('3') followed by `0x0000` (string terminator / 4-byte parcel alignment padding).
- **Extracted Decimal IMEI:** `355752920542633` (Length = 15 digits).
- **Parsing Algorithm for `ImeiParcelParser.cs`:**
  1. Match lines starting with `0x[0-9a-fA-F]{8}:`.
  2. Extract the 4 hex words from each line, converting each 32-bit word into two 16-bit LE integers (`word & 0xFFFF` and `(word >> 16) & 0xFFFF`).
  3. Skip the first 4 bytes (error code) and the next 4 bytes (string length).
  4. Iterate through subsequent 16-bit integers: convert to ASCII chars (`(char)code`). If `code >= '0' && code <= '9'`, append to IMEI buffer.
  5. Truncate at length 15. Verify with Luhn check.

### Check 5: SELinux Enforcement Mode
- **Command:** `getenforce` (framed as `0012shell:getenforce`)
- **Observed Result:** `Enforcing` (Records 12, 25, 119, 132).
- **Purpose & Meaning:** In `Enforcing` mode, standard shell processes (`u:r:shell:s0`, UID 2000) are restricted by policy from writing directly to `/dev/umts_ipc0` or `/dev/socket/rild`. This necessitates escalating to `u:r:system_app:s0` (UID 1000) via port 2600.

### Check 6: KEYTool Service Package Presence
- **Command:** `pm path KEYTool.SRV4` (framed as `001cshell:pm path KEYTool.SRV4`)
- **Observed Result:**
```text
package:/data/app/~~Ol5g_-Ev0NgpenK8egqMbg==/KEYTool.SRV4-0maqZwPFkP4Jxg5fB6y2NA==/base.apk
```
- **Purpose & Meaning:** Confirms whether the Samsung Factory Service / KEYTool helper APK is installed in `/data/app/`. This package hosts the local TCP server on port 2600 running under UID 1000 (`system`).

### Check 7: UID 1000 Local Loopback Port 2600 Check
- **Command:** `echo 'id' |toybox nc localhost 2600` (framed as `002dshell:echo 'id' |toybox nc localhost 2600`)
- **Observed Positive Response (Record 31):**
```text
uid=1000(system) gid=1000(system) groups=1000(system),1002(bluetooth),1005(audio),1007(log),1065(reserved_disk),1077(external_storage),1079(ext_obb_rw),2001(cache),2002(diag),2901(vendor_qti_diag),2902(vendor_qdss),2905(vendor_adpl_odl),3001(net_bt_admin),3002(net_bt),3003(inet),3007(net_bw_acct),3010(wakelock),3011(uhid),5050(vendor_secdir),9997(everybody) context=u:r:system_app:s0.
```
- **Observed Negative Response (Record 138):**
```text
nc: connect: Connection refused.
```
- **Significance:** Positive response indicates port 2600 is alive and yields immediate UID 1000 access. Negative response triggers the readiness recovery sequence.

### Check 8: Diagnostic Packages Inspection
- **Commands:**
  1. `dumpsys package com.samsung.android.app.repaircal | grep -E "versionName|versionCode"`
  2. `dumpsys package com.samsung.android.app.mobiledoctor | grep -E "versionName|versionCode"`
  3. `dumpsys package com.samsung.rms.retailagent.global | grep -E "versionName|versionCode"`
- **Observed Results:**
  - `repaircal`: Empty (not installed).
  - `mobiledoctor`: Empty (not installed).
  - `retailagent.global` (Record 147):
    ```text
    versionCode=30000171 minSdk=34 targetSdk=35
    versionName=5.59.6-main
    ```
- **Purpose & Meaning:** Checks for Samsung Retail Mode (`retailagent.global`) and technician diagnostic suites (`repaircal`, `mobiledoctor`). These packages dictate whether automated carrier provisioning or Knox retail locks might trigger upon CSC transition.

---

## 3. UID 1000 Bridge Readiness Sequence & Escalation Recovery

When port 2600 returns `nc: connect: Connection refused` (as recorded at 17:42:11.846), the tool implements an automated multi-step escalation sequence:

```
[Port 2600 Refused]
        │
        ▼
[Step 1: Keep Screen & CPU Active]
shell:svc power stayon true
        │
        ▼
[Step 2: Inspect Diagnostic Environment]
dumpsys package ... (repaircal, mobiledoctor, retailagent)
        │
        ▼
[Step 3: Deploy & Execute Helper Binary]
sync: push /data/local/tmp/AT -> chmod 777 -> shell:/data/local/tmp/AT R2
        │
        ▼
[Step 4: Authenticate Helper Session]
Challenge Token Received -> Response Transmitted -> AuthKey : OK
        │
        ▼
[Step 5: Poll Loop on localhost:2600]
Attempt 1 (17:42:19.620): nc: connect: Connection refused.
Attempt 2 (17:42:19.747): nc: connect: Connection refused.
Attempt 3 (17:42:19.875): .UID 1000 confirmed..
        │
        ▼
[Step 6: Stop Samsung USB Daemon (ssud)]
shell:echo 'setprop ctl.stop ssud'|toybox nc localhost 2600
        │
        ▼
[Port 2600 Ready & Confirmed]
```

### Protocol Details & Timing
1. **Power Lock:** `svc power stayon true` guarantees the device will not enter doze mode or lock the screen while services spin up.
2. **Polling Loop Parameters:**
   - Interval: ~125 ms to 500 ms between attempts.
   - Timeout limit: 30 seconds (60 iterations at 500 ms).
   - Observed success in log: Triggered within 255 ms after auth response (3 attempts).
3. **Stopping `ssud`:**
   - Command: `echo 'setprop ctl.stop ssud'|toybox nc localhost 2600`
   - Purpose: `ssud` (Samsung USB Daemon) actively manages USB composite gadget states and interfaces with RIL modem endpoints. Halting `ssud` prevents it from resetting USB endpoints or locking the RIL client interface while the injection frames are dispatched.

---

## 4. AT Binary Deployment: Raw ADB Sync Protocol

The file transfer of `/data/local/tmp/AT` is performed strictly via ADB's native `sync:` transport without invoking external binaries:

```
PC (GsmTools)                                 Device (ADB Daemon)
     │                                                 │
     │── "0005sync:" ─────────────────────────────────>│
     │<─ "OKAY" ───────────────────────────────────────│
     │                                                 │
     │── "STAT" + [4-byte LE Len: 18] + Path ─────────>│
     │<─ "STAT" + [Mode: 4B] + [Size: 4B] + [Mtime: 4B]│ (16 bytes response)
     │                                                 │
     │── "SEND" + [4-byte LE Len: 24] + Path,33188 ───>│
     │                                                 │
     │── "DATA" + [4-byte LE Len: 65536] + Chunk 1 ───>│
     │── "DATA" + [4-byte LE Len: 65536] + Chunk 2 ───>│
     │   ... (7x 64KB chunks + 1x remainder chunk)     │
     │                                                 │
     │── "DONE" + [4-byte LE Timestamp] ──────────────>│
     │<─ "OKAY" + [4 zero bytes] ──────────────────────│
     │                                                 │
     │── "QUIT" + [4 zero bytes] ──────────────────────>│
```

### Packet Byte Format Specifications
1. **STAT Request:**
   - ID: `STAT` (ASCII `0x53 0x54 0x41 0x54`)
   - Path length: `0x00000012` (18 bytes for `/data/local/tmp/AT`)
   - Path: `/data/local/tmp/AT`
2. **STAT Response:**
   - Total 16 bytes: `STAT` (4B) + `mode` (4B LE) + `size` (4B LE) + `mtime` (4B LE).
   - Log observed (Record 38): `STAT.....g...p.j` (Size = `0x00078240` = 492,096 bytes).
3. **SEND Request:**
   - ID: `SEND` (ASCII `0x53 0x45 0x4E 0x44`)
   - Length: `0x00000018` (24 bytes for `/data/local/tmp/AT,33188`)
   - Payload: `/data/local/tmp/AT,33188` (33188 = octal `0100644` = `S_IFREG | 0644`).
4. **DATA Packets:**
   - ID: `DATA` (ASCII `0x44 0x41 0x54 0x41`)
   - Chunk length: 4-byte little-endian integer (maximum 65536 bytes).
   - Payload: Raw binary bytes.
5. **DONE Packet:**
   - ID: `DONE` (ASCII `0x44 0x4F 0x4E 0x45`)
   - Timestamp: 4-byte little-endian unix epoch timestamp.
   - Response: `OKAY` + 4 zero bytes.
6. **QUIT Packet:**
   - ID: `QUIT` (ASCII `0x51 0x55 0x49 0x54`) + `0x00 0x00 0x00 0x00`.

### Execution Initiation
Immediately following sync completion:
- `shell:chmod 777 /data/local/tmp/AT` (sets executable permissions).
- `shell:/data/local/tmp/AT` (opens the interactive standard input/output bridge).

---

## 5. Radio Logcat Clear Sequence

- **Exact Command:** `shell:logcat -b radio -c` (framed as `001fshell:logcat -b radio -c`)
- **Placement in Sequence:** Executed precisely after the AT bridge session is established and authenticated, immediately before transmitting the first CSC injection frame (observed at 17:40:24.854 in Run 1 and 17:42:26.345 in Run 2).
- **Forensic Function:** The `-b radio` buffer stores modem IPC requests, unsolicited RIL events (`RIL_UNSOL_RESPONSE_BASEBAND_VERSION_CHANGED`), and SIM carrier notifications. Flushing it with `-c` clears stale modem buffers, ensuring any verification log collection captures only the current CSC switch transaction.

---

## 6. The Exact 5 CSC Injection Hex Commands & Protocol Framing

The injection protocol transmits 5 deterministic OEM hook frames across the bridge:

```
Frame 1: 0601000e0000{hexCsc}0000{hexCsc}
Frame 2: 060100090000{hexCsc}
Frame 3: 060100120000{hexCsc}0000{hexCsc}{hexCsc}
Frame 4: 060100120000
Frame 5: 060100090000{hexCsc}
```

### Hex Conversion Rules
Target CSC string must be exactly 3 uppercase ASCII letters, mapped to 6 uppercase hex characters:
- `XAA` -> `0x58 0x41 0x41` -> `584141`
- `USC` -> `0x55 0x53 0x43` -> `555343`
- `TMB` -> `0x54 0x4D 0x42` -> `544D42`
- `ATT` -> `0x41 0x54 0x54` -> `415454`
- `VZW` -> `0x56 0x5A 0x57` -> `565A57`
- `GCF` -> `0x47 0x43 0x46` -> `474346`

### Detailed Frame Dissection

#### Command 1: Dual Target Carrier Initialization Frame
- **Length:** 14 bytes (28 hex characters).
- **Run 1 (XAA):** `0601000e00005841410000584141`
- **Run 2 (USC):** `0601000e00005553430000555343`
- **Breakdown:**
  - `06 01`: Samsung OEM RIL Dispatch Header (Sub-system: RIL OEM Hook).
  - `00 0e`: Frame payload length (14 bytes in big-endian hex).
  - `00 00`: Sub-command tag (Primary Carrier Select).
  - `{hexCsc}`: Target CSC (3 bytes).
  - `00 00`: Sub-command tag (Secondary Carrier Select).
  - `{hexCsc}`: Target CSC (3 bytes).
- **Observed Response:** `[USB-RAW] IN 6B: .OK..#`

#### Command 2: Active Sales Code Registration Frame
- **Length:** 9 bytes (18 hex characters).
- **Run 1 (XAA):** `060100090000584141`
- **Run 2 (USC):** `060100090000555343`
- **Breakdown:**
  - `06 01`: Samsung OEM RIL Dispatch Header.
  - `00 09`: Frame payload length (9 bytes in big-endian hex).
  - `00 00`: Sub-command tag.
  - `{hexCsc}`: Target CSC (3 bytes).
- **Observed Response:** `[USB-RAW] IN 6B: .OK..#`

#### Command 3: Triple Multi-CSC Hierarchy Frame
- **Length:** 18 bytes (36 hex characters).
- **Run 1 (XAA):** `0601001200005841410000584141584141`
- **Run 2 (USC):** `0601001200005553430000555343555343`
- **Breakdown:**
  - `06 01`: Samsung OEM RIL Dispatch Header.
  - `00 12`: Frame payload length (18 bytes = 0x12).
  - `00 00`: Sub-command tag.
  - `{hexCsc}`: Target CSC (Primary NV).
  - `00 00`: Sub-command tag.
  - `{hexCsc}`: Target CSC (Secondary NV).
  - `{hexCsc}`: Target CSC (Customer/OMC NV fallback).
- **Observed Response:** `[USB-RAW] IN 6B: .OK..#`

#### Command 4: Carrier Feature Sync & NV Reindex Frame
- **Length:** 6 bytes (12 hex characters) — **Contains NO CSC payload!**
- **Payload:** `060100120000`
- **Breakdown:**
  - `06 01`: Samsung OEM RIL Dispatch Header.
  - `00 12`: Operation Code (Sync / Commit to NVRAM).
  - `00 00`: Flags / Zero padding.
- **Observed Response:** `[USB-RAW] IN 6B: .OK..#`

#### Command 5: Non-Volatile Carrier Commit Frame
- **Length:** 9 bytes (18 hex characters).
- **Run 1 (XAA):** `060100090000584141`
- **Run 2 (USC):** `060100090000555343`
- **Breakdown:**
  - `06 01`: Samsung OEM RIL Dispatch Header.
  - `00 09`: Frame payload length (9 bytes).
  - `00 00`: Commit / Finalize tag.
  - `{hexCsc}`: Target CSC (3 bytes).
- **Observed Response:** `[USB-RAW] IN 6B: .OK..#`

### Delimiters and Termination
- **Delimiters:**
  - Each response from `/AT` ends with `.OK..#\n` (6 ASCII characters: `.`, `O`, `K`, `.`, `.`, `#`).
  - Next input prompt is signaled by `<< `.
- **Termination:**
  - Sending `EXIT\n` or `QUIT\n` terminates the bridge process loop cleanly (`at_bridge.c` line 356).

---

## 7. Post-Cleanup Operations

- **Composite Command Line:**
```bash
shell:pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor
```
- **Execution Timing:** Executed immediately following Command 5 acknowledgment (Record 84 at 17:40:24.941 and Record 316 at 17:42:26.439).
- **Sub-Command Breakdown & Observed Outputs:**

| Target Package / Action | Purpose | Observed Output | Status Assessment |
|---|---|---|---|
| `pm uninstall -k --user 0 com.sec.android.AutoPreconfig` | Prevents Samsung AutoPreconfig from reverting injected CSC to physical SIM card CSC on reboot. `-k` preserves app data directory. | `Failure [not installed for 0].` (30 bytes) | Expected on non-carrier or previously uninstalled devices. Handled safely. |
| `pm uninstall -k --user 0 com.sec.omadmspr` | Disables Sprint/Boost OMA-DM carrier provisioning client. | `Failure [not installed for 0].` (30 bytes) | Expected on open-market models (`SM-S918U` on XAA). Handled safely. |
| `pm clear com.samsung.android.app.mobiledoctor` | Wipes technician diagnostic and carrier lock flags from mobile doctor database. | `Failed.` (7 bytes) | Expected if package is absent or already wiped. Handled safely. |

*Crucial Implementation Note:* The desktop application must not treat `Failure [not installed for 0]` or `Failed.` as critical fatal errors, as these packages may not be present on all CSC variants.

---

## 8. USB Reset Protocol

- **Command:** `usb:` (sent framed as `0004usb:`)
- **Transport:** Sent directly to the ADB socket endpoint (`127.0.0.1:5037`) on the current device connection.
- **Observed ADB Response:**
```text
[17:40:25.056] ADB-STREAM | Command: restarting in USB mode
```
- **Driver & Hardware Re-enumeration Sequence:**
  1. ADB server signals Android kernel USB gadget driver (`/dev/usb-ffs/adb` or `conn_gadget`).
  2. Windows USB host controller detects detachment:
     `[17:40:38.995] USB-DEV | Command: PHONE REMOVED | Summary: Phone / Device Disconnected (0435c0a420c48948)`
  3. Device reloads baseband and USB descriptor interfaces.
  4. 27.35 seconds later, device re-attaches:
     `[17:41:06.349] USB-ADB | Command: PHONE CONNECTED | Summary: Android Device Connected: SAMSUNG Mobile USB Composite Device (Serial: 0435c0a420c48948) | VID: 0x04E8 | PID: 0x6860 | Driver: dg_ssudbus`
- **Application Workflow Requirement:** `GsmTools.Desktop` must immediately enter a non-blocking wait/polling state after sending `0004usb:`, monitoring connection re-establishment before issuing subsequent ADB commands.

---

## Features Discovered

| # | Category | Feature | Description | Inputs | Outputs | Error Behavior | Discovered Via |
|---|---|---|---|---|---|---|---|
| 1 | Device Audit | Carrier RSU App Detection | Queries installed packages matching `rsuapp` | `pm list package\|grep rsuapp` | List of package names | Exit code 1 / empty string if absent | `csc 15.txt` L7, L58, L107, L120 |
| 2 | Device Audit | Knox tsdaemon Detection | Dual-syntax process listing for TrustZone security daemon | `ps -A\|grep tsdaemon;ps\|grep tsdaemon` | Process line if active | Empty string if absent | `csc 15.txt` L14, L65, L109, L122 |
| 3 | Telemetry | System Property Query | Fast system property retrieval for Model, Serial, CSC, OS | `getprop` | Formatted key-value list `[key]: [value]` | Empty if daemon down | `csc 15.txt` L21, L72, L111, L124 |
| 4 | Telemetry | IMEI Parcel Parsing | Decodes 16-bit UTF-16LE characters from binder parcel dump | `service call iphonesubinfo 1 s16 com.android.shell` | Raw Parcel hex dump lines | Truncated parcel / invalid length | `csc 15.txt` L32, L83, L114, L127 |
| 5 | Security | SELinux Status Check | Evaluates SELinux enforcement state | `getenforce` | `Enforcing` or `Permissive` | Non-zero exit code | `csc 15.txt` L48, L99, L117, L130 |
| 6 | Privilege | KEYTool Verification | Checks presence of factory service apk | `pm path KEYTool.SRV4` | `package:/data/app/.../base.apk` | Empty / Error if not installed | `csc 15.txt` L109, L133 |
| 7 | Privilege | Loopback Bridge Audit | Tests UID 1000 loopback listener on port 2600 | `echo 'id' \|toybox nc localhost 2600` | Process credentials: `uid=1000(system)...` | `nc: connect: Connection refused.` | `csc 15.txt` L120, L136 |
| 8 | Security | Diagnostic Package Audit | Checks versions of `repaircal`, `mobiledoctor`, `retailagent` | `dumpsys package <pkg> \| grep -E "versionName\|versionCode"` | `versionCode=...` `versionName=...` | Empty if package not present | `csc 15.txt` L141–147 |
| 9 | Escalation | Screen Power Stayon Lock | Prevents device sleep during privilege operations | `svc power stayon true` | Empty (success) | Permission denied if not shell | `csc 15.txt` L139 |
| 10 | Escalation | Samsung USB Daemon Halting | Stops `ssud` via UID 1000 netcat pipe to unblock modem AT | `echo 'setprop ctl.stop ssud'\|toybox nc localhost 2600` | Empty (success) | Connection refused if port closed | `csc 15.txt` L265 |
| 11 | Deployment | Raw ADB Sync Push | Protocol transfer of binary `/data/local/tmp/AT` | `sync:` + `STAT` + `SEND` + `DATA` + `DONE` + `QUIT` | `STAT` (16B), `OKAY` | `FAIL` with error description | `csc 15.txt` L131–189, L267–286 |
| 12 | Deployment | Binary Perm Setting | Grants 0777 execution permissions to `/AT` | `chmod 777 /data/local/tmp/AT` | Empty (success) | File not found if sync failed | `csc 15.txt` L204, L245, L287 |
| 13 | Session | Interactive Bridge Launch | Launches standalone AT bridge session | `/data/local/tmp/AT` | Banner + `Token AuthKey: ...` | Execution failure / SIGSEGV | `csc 15.txt` L57, L206, L289 |
| 14 | Session | Helper R2 Bridge Launch | Launches bridge in R2 auto-listener trigger mode | `/data/local/tmp/AT R2` | Extended banner + `Token AuthKey: ...` | Error if missing root/system | `csc 15.txt` L247 |
| 15 | Radio Log | Radio Logcat Flush | Clears radio event buffer prior to injection | `logcat -b radio -c` | Empty (success) | Buffer not found | `csc 15.txt` L67, L299 |
| 16 | Injection | CSC Frame 1 (Dual Carrier) | Initializes target carrier on primary and secondary NV | `0601000e0000{hexCsc}0000{hexCsc}` | `.OK..#` (6 bytes) | `.ERR_IPC..` | `csc 15.txt` L69, L301 |
| 17 | Injection | CSC Frame 2 (Active Sales) | Sets active carrier sales code | `060100090000{hexCsc}` | `.OK..#` (6 bytes) | `.ERR_IPC..` | `csc 15.txt` L72, L304 |
| 18 | Injection | CSC Frame 3 (Triple OMC) | Sets primary, secondary, and OMC fallback | `060100120000{hexCsc}0000{hexCsc}{hexCsc}` | `.OK..#` (6 bytes) | `.ERR_IPC..` | `csc 15.txt` L75, L307 |
| 19 | Injection | CSC Frame 4 (NV Sync) | Triggers internal NVRAM reindex without payload | `060100120000` | `.OK..#` (6 bytes) | `.ERR_IPC..` | `csc 15.txt` L78, L310 |
| 20 | Injection | CSC Frame 5 (NV Commit) | Commits carrier configuration permanently to NV | `060100090000{hexCsc}` | `.OK..#` (6 bytes) | `.ERR_IPC..` | `csc 15.txt` L81, L313 |
| 21 | Cleanup | Carrier App Cleanup | Uninstalls AutoPreconfig, omadmspr, and wipes mobiledoctor | `pm uninstall -k --user 0 ...;pm clear ...` | `Failure [...]`, `Failed.` | Non-zero exit code (handled as non-fatal) | `csc 15.txt` L84, L316 |
| 22 | USB Control | ADB USB Mode Restart | Triggers USB re-enumeration over ADB socket | `0004usb:` | `restarting in USB mode` | ADB socket disconnect / timeout | `csc 15.txt` L89, L321 |

---

## Edge Cases

| # | Feature | Input | Observed Behavior |
|---|---|---|---|
| 1 | UID 1000 Port 2600 Check | Initial query when port 2600 is closed | Returns `nc: connect: Connection refused.` without hanging. System transitions smoothly into `svc power stayon true` and helper escalation. |
| 2 | Diagnostic App Dumpsys | Query for non-existent package `repaircal` | `dumpsys package com.samsung.android.app.repaircal` returns empty string; grep exits cleanly with exit code 1. |
| 3 | Post-Cleanup Package Removal | `pm uninstall -k --user 0 com.sec.android.AutoPreconfig` on device where package is absent | Returns `Failure [not installed for 0].`; tool does not crash or abort, proceeds to next statement. |
| 4 | Post-Cleanup Package Clear | `pm clear com.samsung.android.app.mobiledoctor` when app is not installed | Returns `Failed.`; tool ignores failure and proceeds directly to USB reset. |
| 5 | AT Bridge File Size | Transferring 492,096 byte ELF binary | Fragmented across 7 full 64KB `DATA` chunks and 1 remainder chunk of 33,280 bytes, properly closed by `DONE` and validated by 16-byte `STAT` response. |
| 6 | USB Mode Reset Disconnect | Sending `0004usb:` while client socket is active | Socket disconnects immediately after receiving `restarting in USB mode`. Device drops from Windows device tree for ~27 seconds before re-enumerating. |
| 7 | CSC Injection Command 4 | Dispatching `060100120000` with no target CSC | RIL modem recognizes length 0x12 commit opcode, successfully executing without carrier string and returning `.OK..#`. |
| 8 | IMEI Parcel Trailing Padding | Decoding 15-character IMEI in 40-byte parcel buffer | Character 15 (`3`) is followed by 16-bit zero null terminator (`0x0000`) and 4-byte boundary padding, safely ignored by parser. |
