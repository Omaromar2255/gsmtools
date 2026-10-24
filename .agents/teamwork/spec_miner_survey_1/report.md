# Specification Mining Report: AT Bridge Architecture, Cryptography & Protocols

**Document ID**: SPEC-MINER-SURVEY-1-AT-BRIDGE  
**Target Codebase**: `at-bridge/jni/at_bridge.c`, `at-bridge/README.md`, `at-bridge/jni/Android.mk`, `csc 15.txt`  
**Mining Agent**: `spec_miner_survey_1` (Teamwork Preview Spec Miner)  
**Date**: 2026-10-17  

---

## 1. Executive Summary

This deep-dive specification report documents the complete internal architecture, protocol interface, cryptographic primitives, and privilege escalation mechanics of the `AT` bridge binary (`at_bridge.c`) and its interaction with Samsung modem sub-systems, Android RILD, and ADB host clients.

The binary serves as a zero-footprint, statically linked (`-static`, `arm64-v8a`) multi-modal communication bridge. It abstracts direct Samsung IPC character device nodes, Android RIL OEM Parcels over UNIX domain sockets, and AT modem serial ports behind a unified line-oriented interactive shell protocol featuring optional per-packet Xorshift32 stream encryption.

---

## 2. Features Discovered

| # | Category | Feature | Description | Inputs | Outputs | Error Behavior | Discovered Via |
|---|----------|---------|-------------|--------|---------|----------------|----------------|
| 1 | Telemetry | System Property Interrogation | Direct in-memory property retrieval using Android Bionic `__system_property_get` without spawning shell processes. | Property keys: `ro.product.model`, `ro.csc.sales_code`, `ro.build.version.release`, `ro.build.version.security_patch`, `ro.boot.debug_level` | In-memory string buffers | Falls back to `"N/A"` if property get returns $\le 0$. | `at_bridge.c:60-68` |
| 2 | Telemetry | Compact Startup Banner | Formatted telemetry banner output upon process start or via `INFO` command. | None (reads system props and active bridge state) | `INFO\|MODEL=...\|CSC=...\|AND=...\|PATCH=...\|DBG=...\|UID=...\|CH=...\n` | Emits `"NONE"` for `CH` if no channel opened. | `at_bridge.c:319-331` |
| 3 | Protocol | Interactive Prompt | Standard terminal prompt emitted after startup, empty lines, and each command completion. | None | `<< ` (two less-than signs and space, no trailing newline, flushed immediately) | N/A | `at_bridge.c:349-351, 360, 366, 374, 380, 396, 424` |
| 4 | Security | Authentication & Session Seeding | Dynamic activation of Xorshift32 stream cipher for all subsequent IPC packets. | `AUTH:<8-Hex-Seed>\n` (e.g. `AUTH:A1B2C3D4`) | `.OK_AUTH..\n<< ` | Seed `0` disables encryption (plaintext mode). Invalid hex characters fall back to parsed prefix or 0. | `at_bridge.c:363-369` |
| 5 | Security | Xorshift32 Stream Cipher | Symmetric additive stream cipher using Marsaglia triplets [13, 17, 5] applied byte-by-byte to incoming commands and outgoing responses. | Buffer pointer `buf`, length `len`, 32-bit seed `seed` | Buffer transformed in-place via XOR keystream | If `seed == 0`, buffer remains untouched. State re-initialized to seed per packet. | `at_bridge.c:74-84` |
| 6 | Security | Sensitive RAM Scrubbing | Zeroes command payload buffer in memory immediately after hardware dispatch to prevent forensic memory dumps. | `memset(payload, 0, sizeof(payload))` | RAM memory wiped | N/A | `at_bridge.c:408` |
| 7 | Protocol | Plaintext/Raw Hex Mode | Direct unencrypted hex transmission of IPC frames when session seed is 0. | Hex string (e.g. `0601000e00005841410000584141\n`) | `.OK..#\n<< ` or `.OK..<HEX_RESP>#\n<< ` | Returns `.ERR_SYNTAX..\n<< ` on malformed hex; `.ERR_IPC..\n<< ` on transmission failure. | `at_bridge.c:400-420`, `csc 15.txt:468-521` |
| 8 | Protocol | Encrypted Hex Mode | Transmission of ciphertext hex strings when session seed > 0. | Encrypted hex string | `.OK..<HEX_CIPHERTEXT>#\n<< ` or `.OK..#\n<< ` | Same error delimiters as raw hex mode. | `at_bridge.c:403, 412` |
| 9 | Channel | Channel Reconfiguration (AT) | Dynamic runtime switch to prefer AT modem TTY character devices. | `MODE:AT\n` | `.CH=<active_path>..\n<< ` | If no node accessible, `.CH=NONE..\n<< ` | `at_bridge.c:372-377` |
| 10 | Channel | Channel Reconfiguration (IPC) | Dynamic runtime switch to prefer raw IPC character nodes and RILD sockets. | `MODE:IPC\n` | `.CH=<active_path>..\n<< ` | If no node accessible, `.CH=NONE..\n<< ` | `at_bridge.c:378-383` |
| 11 | Command | Raw AT Command Execution | Synchronous submission of plaintext AT commands to modem TTY node. | `AT:<command>\n` (e.g. `AT:AT+DEVCONINFO\n`) | `<raw_response>\n<< ` | `.TIMEOUT..\n<< ` if no response in 2000ms; `.ERR_WRITE..\n<< ` if write fails. | `at_bridge.c:385-397` |
| 12 | Command | Clean Session Teardown | Graceful termination of interactive session and release of hardware file descriptors. | `EXIT\n` or `QUIT\n` (case-insensitive) | Closes channel FD and process exits with code 0 | Signal handlers (`SIGINT`, `SIGTERM`) set `g_running = 0`. | `at_bridge.c:52-55, 356, 428-429` |
| 13 | Channel | Multi-Tier Channel Probing | Prioritized search across IPC device nodes, Android RILD domain sockets, and AT serial ports. | `init_best_channel(prefer_at_tty)` | Updates `g_ctx.mode`, `g_ctx.fd`, `g_ctx.active_path` | Returns -1 if all candidate paths fail `access(R_OK \| W_OK)` or `open()`. | `at_bridge.c:196-212` |
| 14 | Framing | Android RILD OEM Parcel Framing | Encapsulates raw IPC frame into standard Android RIL Parcel (`RIL_REQUEST_OEM_HOOK_RAW` = 59) over `/dev/socket/rild`. | Raw IPC bytes, length | 4-byte BE size + 4-byte LE req_id + 4-byte LE token + 4-byte LE dlen + payload + 4-byte padding | Returns -1 on write error or read timeout (2500ms). | `at_bridge.c:255-280` |
| 15 | Framing | Direct Character Device I/O | Non-blocking raw write and select-based packet reassembly with 35ms burst timeout for character nodes. | Raw bytes, length, timeout | Read payload and total byte count | Handles `EINTR`, `EAGAIN`, `EWOULDBLOCK` with 5ms backoff. | `at_bridge.c:218-252` |
| 16 | Escalation | Port 2600 System Relay Probe | Probes local TCP port 2600 for companion app escalation daemon; confirms `UID=1000` context and stops `ssud`. | Connect to `127.0.0.1:2600`, send `"setprop ctl.stop ssud; id\n"` | `[+] UID 1000 Escalation Bridge confirmed on port 2600\n` | Returns -1 if port closed or response does not contain `"uid=1000"`. | `at_bridge.c:91-126` |
| 17 | Workflow | Samsung CSC Injection Framing | Exact 5-command IPC frame sequence used to inject Carrier Sales Codes into Samsung modem NVM. | 3-character ASCII CSC (e.g. `XAA`) | 5 consecutive `.OK..#` responses | Failure if any frame returns `.ERR_IPC..` or times out. | `csc 15.txt:468-521`, `ORIGINAL_REQUEST.md:53-59` |

---

## 3. Edge Cases Discovered

| # | Feature | Input / Condition | Observed Behavior |
|---|---------|-------------------|-------------------|
| 1 | Authentication | `AUTH:0` or `AUTH:00000000` | Seed is parsed as 0. Stream cipher remains inactive (`seed == 0` bypasses `transform_buffer`). Bridge responds `.OK_AUTH..\n<< `. Operates in Plaintext / Raw Hex Mode. |
| 2 | Authentication | `AUTH:` with trailing invalid hex (e.g. `AUTH:XYZ`) | `strtoul` returns 0. Bridge sets `g_session_seed = 0` and responds with `.OK_AUTH..\n<< `. Effectively disables encryption without returning an error. |
| 3 | Authentication | `AUTH:123456789ABC` (> 8 hex digits) | `strtoul` parses full 64-bit value and casts to `uint32_t`, truncating to lower 32 bits (`(uint32_t)strtoul(...)`). Responds `.OK_AUTH..\n<< `. |
| 4 | Stream Cipher | Seed with MSB set (e.g. `0x80000000`) | Unsigned shift `state >> 17` performs logical right shift (zero-filling MSB). Bitwise operations wrap cleanly modulo $2^{32}$. |
| 5 | Stream Cipher | Empty payload buffer (`len == 0`) | `transform_buffer` loop `for (size_t i = 0; i < len; i++)` immediately terminates. Buffer is untouched. |
| 6 | Hex Parser | Hex string containing spaces or colons (`06 01:00 0e`) | `hex2bin` explicitly skips `' '` and `':'` characters, parsing valid hex pairs seamlessly without syntax errors. |
| 7 | Hex Parser | Odd number of hex characters (e.g. `06010`) | `hex2bin` requires pairs `*hex && *(hex + 1)`. Parses all complete pairs and drops trailing orphaned nibble. |
| 8 | Hex Parser | Non-hex characters in body (e.g. `0601ZZ09`) | `sscanf` fails on `ZZ`; parsing breaks at index. Returns bytes parsed before invalid pair. If 0 bytes parsed, outputs `.ERR_SYNTAX..\n<< `. |
| 9 | Command Framing | Empty line / lone newline (`\n` or `\r\n`) | `line[strcspn(line, "\r\n")] = '\0'`. Empty check `if (!line[0])` matches, re-emits `<< ` prompt without processing any command. |
| 10 | Command Framing | Unknown textual command (e.g. `FOOBAR`) | Does not match known commands (`EXIT`, `INFO`, `AUTH:`, `MODE:`, `AT:`). Falls through to `hex2bin`, which fails (0 bytes). Bridge emits `.ERR_SYNTAX..\n<< `. |
| 11 | Hardware Channel | All modem nodes inaccessible (`CH=NONE`) | When IPC hex frame is received, `dispatch_ipc_frame` calls `init_best_channel(0)`. If still `CH_NONE`, returns `-1`. Bridge emits `.ERR_IPC..\n<< `. |
| 12 | RILD Parcel | IPC frame length not multiple of 4 bytes | `send_rild_oem_parcel` appends zero padding bytes until `(offset - 4) % 4 == 0`. Ensures compatibility with Android Binder/Parcel memory alignment. |
| 13 | IPC Response | Modem sends no response payload (0 bytes) | `dispatch_ipc_frame` returns `r = 0`. Bridge outputs `.OK..#\n<< ` (empty payload delimiter). Client must recognize `.OK..#` as successful zero-byte acknowledgement. |
| 14 | Terminal Disconnect | Shell socket abruptly closes / broken pipe | `signal(SIGPIPE, SIG_IGN)` prevents kernel termination with `EPIPE`. `fgets` returns `NULL`, cleanly breaking loop and closing channel FD. |

---

## 4. Deep-Dive Specification Mining

### 4.1 Startup Banner & Prompt Format

#### Banner Output Format
Upon startup, or whenever the host sends `INFO\n`, `print_compact_info()` writes a single formatted string to `stdout`:
```
INFO|MODEL=<model>|CSC=<csc>|AND=<android_version>|PATCH=<security_patch>|DBG=<debug_level>|UID=<process_uid>|CH=<active_channel_path>
```
Followed immediately by a newline and the interactive prompt:
```
<< 
```

#### Field Specifications

| Field | Source Key / Function | Typical Example | Description |
|---|---|---|---|
| `MODEL` | `ro.product.model` | `SM-S918U` | Commercial hardware marketing model number. |
| `CSC` | `ro.csc.sales_code` | `ATT` | Active carrier sales code from system properties. |
| `AND` | `ro.build.version.release` | `15` | Android OS major version release. |
| `PATCH` | `ro.build.version.security_patch`| `2025-07-01` | Android SPL (Security Patch Level) date string. |
| `DBG` | `ro.boot.debug_level` | `0x4f4c` | Bootloader debug level flag (LOW, MID, HIGH). |
| `UID` | `getuid()` | `2000` or `1000` | POSIX user identifier of running bridge process (`2000` = shell, `1000` = system, `0` = root). |
| `CH` | `g_ctx.active_path` | `/dev/socket/rild` | Active hardware device node or socket path. If no channel could be opened, emits `"NONE"`. |

#### Prompt Mechanics
- Prompt string: `"<< "` (ASCII `0x3C 0x3C 0x20`).
- No trailing `\r` or `\n`.
- Synchronously flushed after every output (`fflush(stdout)`).
- Host clients must buffer incoming characters until `"<< "` is detected at the end of the stream before dispatching the next interactive command.

---

### 4.2 Authentication & Session Protocol

#### Command Syntax
```
AUTH:<SeedHex8>
```
- Case-insensitivity: Prefix checked via `strncasecmp(line, "AUTH:", 5) == 0`.
- Parameter: `<SeedHex8>` is an ASCII hex string representing a 32-bit unsigned integer (e.g. `AUTH:3A7F01BC`).
- Evaluation:
  ```c
  g_session_seed = (uint32_t)strtoul(line + 5, NULL, 16);
  ```
- Response:
  ```
  .OK_AUTH..\n
  << 
  ```

#### Operating Modes
1. **Plaintext / Raw Hex Mode (`Seed = 0`)**:
   - Activated by sending `AUTH:0` or omitting `AUTH:` entirely (initial default is `g_session_seed = 0`).
   - `transform_buffer` returns immediately without altering bytes.
   - Host sends plain ASCII hex representation of IPC frames.
   - Bridge returns plain ASCII hex representation of modem responses.
2. **Encrypted Mode (`Seed > 0`)**:
   - Activated by sending `AUTH:<8-Hex-Seed>` where seed $\neq 0$.
   - All subsequent hex strings on `stdin` are decrypted via Xorshift32 before hardware dispatch.
   - All response bytes from hardware are encrypted via Xorshift32 before hex encoding and output to `stdout`.

---

### 4.3 Xorshift32 Stream Cipher Algorithm & State Mechanics

#### Mathematical Definition
The stream cipher is an additive stream cipher (counter-free XOR stream) based on George Marsaglia's 32-bit linear-feedback shift register (LFSR) algorithm:

$$\begin{aligned}
y_0 &= \text{seed} \\
y_{t}^{(1)} &= y_{t-1} \oplus (y_{t-1} \ll 13) \pmod{2^{32}} \\
y_{t}^{(2)} &= y_{t}^{(1)} \oplus (y_{t}^{(1)} \gg 17) \\
y_t &= y_{t}^{(2)} \oplus (y_{t}^{(2)} \ll 5) \pmod{2^{32}} \\
k_t &= y_t \land \text{0xFF} \\
c_t &= m_t \oplus k_t
\end{aligned}$$

#### C Reference Implementation (`at_bridge.c:74-84`)
```c
static void transform_buffer(uint8_t *buf, size_t len, uint32_t seed) {
    if (seed == 0) return; /* وضع غير مشفر للاختبار المحلي */
    uint32_t state = seed;
    for (size_t i = 0; i < len; i++) {
        /* خوارزمية Xorshift32 لتوليد مفتاح متدفق متغير لكل بايت */
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        buf[i] ^= (uint8_t)(state & 0xFF);
    }
}
```

#### Critical Cryptographic State Rule
**The PRNG state is NOT maintained across packets.**
At the beginning of `transform_buffer`, `state` is re-initialized to `seed`:
`uint32_t state = seed;`
- Incoming packet: Decrypted with `transform_buffer(payload, bin_len, g_session_seed)` (starts with `state = seed`).
- Outgoing response: Encrypted with `transform_buffer(resp, r, g_session_seed)` (starts with `state = seed`).
- Next packet: Decrypted with `transform_buffer(payload, bin_len, g_session_seed)` (starts with `state = seed`).

Because XOR is symmetric and self-inverting:
$$\text{Transform}(\text{Transform}(M, S), S) = M$$
The exact same algorithm is executed for both encryption and decryption.

#### High-Performance C# Implementation (`GsmTools.Desktop/Core`)
```csharp
namespace GsmTools.Desktop.Core
{
    public static class AtBridgeCrypto
    {
        /// <summary>
        /// In-place Xorshift32 additive stream cipher matching at_bridge.c transform_buffer.
        /// </summary>
        /// <param name="buffer">Target byte array containing plaintext or ciphertext.</param>
        /// <param name="offset">Starting byte offset in the buffer.</param>
        /// <param name="length">Number of bytes to transform.</param>
        /// <param name="seed">32-bit session seed. If seed is 0, buffer is unmodified.</param>
        public static void TransformBuffer(byte[] buffer, int offset, int length, uint seed)
        {
            if (seed == 0 || buffer == null || length <= 0)
                return;

            uint state = seed;
            int end = offset + length;
            for (int i = offset; i < end; i++)
            {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                buffer[i] ^= (byte)(state & 0xFF);
            }
        }

        /// <summary>
        /// Transforms a byte array copy and returns the new array.
        /// </summary>
        public static byte[] Transform(byte[] data, uint seed)
        {
            if (seed == 0 || data == null || data.Length == 0)
                return (byte[])data?.Clone() ?? Array.Empty<byte>();

            byte[] copy = (byte[])data.Clone();
            TransformBuffer(copy, 0, copy.Length, seed);
            return copy;
        }
    }
}
```

#### Bitwise Verification Table (First 5 Keystream Bytes for `Seed = 0x12345678`)
- Iteration 1:
  - `state = 0x12345678`
  - `<< 13`: `0x8ACF0000` $\rightarrow$ `^`: `0x98FB5678`
  - `>> 17`: `0x00004C7D` $\rightarrow$ `^`: `0x98FB1A05`
  - `<< 5`:  `0x1F6340A0` $\rightarrow$ `^`: `0x87985AA5`
  - `k_0 = 0xA5`
- Iteration 2:
  - `state = 0x87985AA5`
  - `<< 13`: `0x0B54A000` $\rightarrow$ `^`: `0x8CCCFA05`
  - `>> 17`: `0x00004666` $\rightarrow$ `^`: `0x8CCCBC63`
  - `<< 5`:  `0x99778C60` $\rightarrow$ `^`: `0x15BB3003`
  - `k_1 = 0x03`
- Iteration 3:
  - `state = 0x15BB3003`
  - `<< 13`: `0x66006000` $\rightarrow$ `^`: `0x73BB5003`
  - `>> 17`: `0x000039DD` $\rightarrow$ `^`: `0x73BB69DE`
  - `<< 5`:  `0x776D3BC0` $\rightarrow$ `^`: `0x04D6521E`
  - `k_2 = 0x1E`
- Iteration 4:
  - `state = 0x04D6521E`
  - `<< 13`: `0xCA43C000` $\rightarrow$ `^`: `0xCE95921E`
  - `>> 17`: `0x0000674A` $\rightarrow$ `^`: `0xCE95F554`
  - `<< 5`:  `0xD2BEAA80` $\rightarrow$ `^`: `0x1C2B5FD4`
  - `k_3 = 0xD4`
- Iteration 5:
  - `state = 0x1C2B5FD4`
  - `<< 13`: `0x6BFFA800` $\rightarrow$ `^`: `0x77D4F7D4`
  - `>> 17`: `0x00003BEA` $\rightarrow$ `^`: `0x77D4CC3E`
  - `<< 5`:  `0xFA9987C0` $\rightarrow$ `^`: `0x8D4D4BFE`
  - `k_4 = 0xFE`

---

### 4.4 Command & Packet Framing

#### Interactive Protocol Delimiters

| Delimiter | Context | Meaning |
|---|---|---|
| `<< ` | stdout | Ready prompt. Bridge is waiting for the next input command line. |
| `.OK..#\n` | stdout | IPC frame acknowledged by modem/RILD with 0 response bytes. |
| `.OK..<HEX>#\n` | stdout | IPC frame acknowledged with return payload `<HEX>` (encrypted if seed > 0). |
| `.OK_AUTH..\n` | stdout | Authentication seed registered successfully. |
| `.CH=<path>..\n` | stdout | Hardware channel switched successfully to `<path>`. |
| `.TIMEOUT..\n` | stdout | AT command timed out (no response within 2000ms). |
| `.ERR_WRITE..\n` | stdout | Hardware write failure on character device FD. |
| `.ERR_IPC..\n` | stdout | Hardware dispatch failure on active IPC channel or socket. |
| `.ERR_SYNTAX..\n`| stdout | Malformed hex characters on command line. |

#### Samsung CSC 5-Frame Injection Protocol
The Samsung CSC switch sequence transmits 5 binary IPC frames via `dispatch_ipc_frame`.
For CSC = `XAA` (ASCII hex `584141`):

1. **Frame 1 - Set Primary & Carrier CSC**:
   - Plaintext Hex: `0601000e00005841410000584141`
   - IPC Class: `0x06` (SEC_IPC_CMD_SEC), Sub: `0x01`
   - Frame Length: `0x000E` (14 bytes)
   - Payload: `0000` + `584141` ("XAA") + `0000` + `584141` ("XAA")
2. **Frame 2 - Set Sales Code**:
   - Plaintext Hex: `060100090000584141`
   - Frame Length: `0x0009` (9 bytes)
   - Payload: `0000` + `584141` ("XAA")
3. **Frame 3 - Multi-CSC Registry Update**:
   - Plaintext Hex: `0601001200005841410000584141584141`
   - Frame Length: `0x0012` (18 bytes)
   - Payload: `0000` + `584141` + `0000` + `584141` + `584141`
4. **Frame 4 - NVM Commit / Flush**:
   - Plaintext Hex: `060100120000`
   - Frame Length: `0x0012` (18 bytes header, null parameter)
   - Payload: `0000`
5. **Frame 5 - Verify & Persist CSC**:
   - Plaintext Hex: `060100090000584141`
   - Frame Length: `0x0009` (9 bytes)
   - Payload: `0000` + `584141` ("XAA")
6. **Teardown**:
   - Client sends `EXIT\n`.
   - Process closes file descriptors and exits.

---

### 4.5 Smart Fallback, Hardware Channels & Relay Mechanisms

#### SELinux & UID 1000 Architectural Requirement
1. **The Permission Barrier**:
   When `/data/local/tmp/AT` is launched directly from `adb shell`, it inherits the `shell` user context:
   - UID: `2000` (`shell`)
   - SELinux domain: `u:r:shell:s0`
   In modern Samsung firmware (Android 13, 14, 15 / One UI 5, 6, 7), SELinux policy strictly forbids `u:r:shell:s0` from opening `/dev/socket/rild` (type `rild_socket`) and character nodes `/dev/umts_ipc0` (type `radio_device`).
   Consequently, direct invocation under `UID=2000` results in `access()` returning -1, leaving the bridge with `CH=NONE`.
2. **The Companion Service (`KEYTool.SRV4` / Port 2600)**:
   Forensic traces in `csc 15.txt:127-130` show:
   ```
   uid=1000(system) gid=1000(system) groups=1000(system),1002(bluetooth),... context=u:r:system_app:s0
   ```
   The companion service listens on TCP `127.0.0.1:2600` under `UID=1000` (`system_app` domain), which has full access to Samsung RIL sockets and IPC channels.
3. **The Relay Execution Flow**:
   - Pre-check: Client verifies port 2600 readiness via ADB:
     `echo 'id' | toybox nc localhost 2600`
   - Deactivation of conflicting daemons:
     `echo 'setprop ctl.stop ssud' | toybox nc localhost 2600`
   - Spawning Bridge under UID 1000:
     If the bridge binary `/data/local/tmp/AT` is executed via:
     `toybox nc localhost 2600` (or companion app execution pipe),
     the bridge runs with `UID=1000`.
   - Result:
     When running under `UID=1000`, `init_best_channel` successfully opens `/dev/socket/rild` or `/dev/umts_ipc0`, reporting:
     `INFO|MODEL=SM-S918U|CSC=ATT|AND=15|PATCH=2025-07-01|DBG=0x4f4c|UID=1000|CH=/dev/socket/rild`

#### Channel Priority Cascade

```
                    +------------------------------------+
                    |       init_best_channel()          |
                    +------------------------------------+
                                      |
                     [prefer_at_tty == 1?]
                               /              \
                             YES               NO
                             /                  \
              +----------------------+   +-----------------------+
              | Check AT_NODES       |   | Check IPC_NODES       |
              | (/dev/umts_atc0, ...) |   | (/dev/umts_ipc0, ...) |
              +----------------------+   +-----------------------+
                         |                           |
                     [Opened?]                   [Opened?]
                     /       \                   /       \
                   YES        NO               YES        NO
                   /           \               /           \
               CH_DEV_AT_TTY    |        CH_DEV_IPC_RAW     |
                                |                           v
                                |                +-----------------------+
                                |                | Check RILD_SOCKETS    |
                                |                | (/dev/socket/rild, ...) |
                                |                +-----------------------+
                                |                           |
                                |                       [Opened?]
                                |                       /       \
                                |                     YES        NO
                                |                     /           \
                                |              CH_RILD_PARCEL      |
                                |                                  v
                                +------------------------> Check AT_NODES
                                                                   |
                                                               [Opened?]
                                                               /       \
                                                             YES        NO
                                                             /           \
                                                     CH_DEV_AT_TTY     CH_NONE
```

#### Android RIL Parcel Layout (`CH_RILD_PARCEL`)

```
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +---------------------------------------------------------------+
 |               Total Parcel Length (Big-Endian)                | (bytes 0..3)
 +---------------------------------------------------------------+
 |             req_id = 59 (RIL_REQUEST_OEM_HOOK_RAW)             | (bytes 4..7)
 +---------------------------------------------------------------+
 |                     token (Little-Endian)                     | (bytes 8..11)
 +---------------------------------------------------------------+
 |              ipc_len (Raw IPC Data Length in Bytes)           | (bytes 12..15)
 +---------------------------------------------------------------+
 |                                                               |
 |               Raw IPC Data Payload (ipc_len bytes)            | (bytes 16..16+ipc_len-1)
 |                                                               |
 +---------------------------------------------------------------+
 |         Alignment Padding (0x00 bytes to multiple of 4)        |
 +---------------------------------------------------------------+
```

---

## 5. Architectural Recommendations for `GsmTools.Desktop`

1. **Implement Dual-Mode `PersistentAtBridgeSession`**:
   - Provide an option to operate either in Raw Hex Mode (`Seed = 0`) or Encrypted Mode (`Seed = 0x...`).
   - For Encrypted Mode, generate a random 32-bit seed via `RandomNumberGenerator.GetInt32(1, int.MaxValue)`, send `AUTH:<seed>`, and wrap/unwrap all subsequent payloads.
2. **Buffer Splitting & Prompt Synchronization**:
   - Because ADB interactive streams can fragment output, `PersistentAtBridgeSession` must accumulate incoming text in a line/chunk buffer, checking for `.OK..*#` and the trailing `<< ` prompt before fulfilling task completion.
3. **Escalation Fallback Orchestration**:
   - If direct execution of `/data/local/tmp/AT` returns `UID=2000` and `CH=NONE`, automatically initiate the port 2600 handshake:
     `echo 'id' | toybox nc localhost 2600`
     Verify `uid=1000`, run `echo 'setprop ctl.stop ssud' | toybox nc localhost 2600`, and relay the session through the port 2600 tunnel.
