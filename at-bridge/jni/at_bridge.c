/*
 * ============================================================================
 *  GsmTools — Samsung AT / RIL / IPC Multi-Modal Bridge  (v4.0)
 * ============================================================================
 *  Complete rewrite of the on-device bridge binary.
 *
 *  Channels (priority order):
 *    1. CH_SECRIL_LIB     libsecril-client.so  (send_recv_cmd)   [strongest]
 *    2. CH_DEV_IPC_RAW     /dev/umts_ipc0 | ipc1 | rfs0 | diag
 *    3. CH_RILD_PARCEL     /dev/socket/rild* (OEM hook parcel 59)
 *    4. CH_DEV_AT_TTY      /dev/umts_atc0 | atc1 | at0 | at_distributor | smd* | tty*
 *    5. CH_TCP_RELAY_2600  TCP 127.0.0.1:2600 (UID-1000 companion relay)
 *
 *  Session protocol (stable contract, consumed by GsmTools.Desktop):
 *    INFO|MODEL=..|CSC=..|AND=..|PATCH=..|DBG=..|UID=..|CH=..
 *    prompt " << "  — machine markers ".OK..%s#" ".ERR_*.." ".CH=%s.."
 *    SIMLOCK|CPIN=..|PN=..|PU=..|PP=..|PC=..|SC=..
 *    DEVINFO|MODEL=..|..|DEVCON=..
 *    AKSEED|<response>
 *
 *  Build (Android):
 *    aarch64-linux-android29-clang  -O2 -s -fvisibility=hidden -ldl at_bridge.c -o AT
 *    armv7a-linux-androideabi21-clang -O2 -s -ldl at_bridge.c -o AT_32
 *  Build (host Linux, for protocol testing / mock runs):
 *    gcc -DGT_HOST_BUILD -O2 at_bridge.c -o AT_host -lpthread
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <termios.h>

#ifdef __ANDROID__
#  include <dlfcn.h>
#  include <sys/system_properties.h>
#  define gt_system_property_get __system_property_get
#  define GT_HAVE_SECRIL 1
#else
#  include "gt_host_compat.h"   /* host-build shims (properties / dlopen) */
#  define GT_HAVE_SECRIL 0
#endif

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "all_gsm_commands.h"

#define GT_VERSION     "4.0"
#define CMD_BUF_SIZE   4096
#define RESP_BUF_SIZE  8192

/* =========================================================================
 *  0. GLOBAL STATE
 * ========================================================================= */
typedef enum {
    CH_NONE = 0,
    CH_SECRIL_LIB,      /* libsecril-client.so send_recv_cmd          */
    CH_DEV_IPC_RAW,     /* /dev/umts_ipc0 | umts_rfs0 | /dev/diag     */
    CH_RILD_PARCEL,     /* /dev/socket/rild + OEM parcel framing     */
    CH_DEV_AT_TTY,      /* /dev/umts_atc0 & friends (text AT)         */
    CH_TCP_RELAY_2600   /* TCP localhost:2600 companion bridge       */
} channel_mode_t;

typedef struct {
    channel_mode_t mode;
    int            fd;
    void          *dl_handle;
    char           active_path[128];
} bridge_ctx_t;

static bridge_ctx_t g_ctx = { .mode = CH_NONE, .fd = -1, .dl_handle = NULL };
static volatile int  g_running = 1;
static uint32_t       g_session_seed = 0;   /* 0 = plaintext, >0 = Xorshift32 */
static int            g_verbose = 0;

/* libsecril send_recv_cmd symbol (Android only, resolved by dlsym) */
typedef int (*secril_send_recv_fn)(void *client, int cmd_id,
                                   const void *data_in, size_t in_len,
                                   void *data_out, size_t *out_len);
static secril_send_recv_fn g_secril_fn = NULL;

static void sig_handler(int sig) {
    (void)sig;
    g_running = 0;
}

/* =========================================================================
 *  1. SYSTEM PROPERTY READ (direct, no subprocess)
 * ========================================================================= */
static void fast_getprop(const char *key, char *out, size_t max_len) {
    char buf[PROP_VALUE_MAX] = {0};
    if (gt_system_property_get(key, buf) > 0) {
        strncpy(out, buf, max_len - 1);
        out[max_len - 1] = '\0';
    } else {
        strncpy(out, "N/A", max_len - 1);
        out[max_len - 1] = '\0';
    }
}

/* =========================================================================
 *  2. SESSION CIPHER — Marsaglia Xorshift32 [13,17,5] (involutory stream)
 *     Identical to C# Xorshift32StreamCipher and tools/ parity tests.
 *     Covers HEX IPC payloads + responses only (AT text stays readable).
 * ========================================================================= */
static void transform_buffer(uint8_t *buf, size_t len, uint32_t seed) {
    if (seed == 0) return;
    uint32_t state = seed;
    for (size_t i = 0; i < len; i++) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        buf[i] ^= (uint8_t)(state & 0xFF);
    }
}

/* =========================================================================
 *  3. PORT-2600 UID-1000 ESCALATION BRIDGE
 * ========================================================================= */
static int relay_roundtrip(int port, const char *cmd, char *resp, size_t resp_max,
                           int timeout_ms) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv = { .tv_sec = timeout_ms / 1000,
                           .tv_usec = (timeout_ms % 1000) * 1000 };
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    if (write(sock, cmd, strlen(cmd)) < 0) { close(sock); return -1; }

    /* poll until data or timeout — retry small reads instead of one blind read */
    int total = 0;
    while (total < (int)resp_max - 1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        struct timeval tv2 = { .tv_sec = 0, .tv_usec = 200000 };
        if (select(sock + 1, &rfds, NULL, NULL, &tv2) <= 0) break;
        int n = read(sock, resp + total, resp_max - 1 - total);
        if (n <= 0) break;
        total += n;
        if (strstr(resp, "uid=") || strstr(resp, "OK")) break;  /* got a verdict */
    }
    resp[total] = '\0';
    close(sock);
    return total;
}

static int try_escalate_via_port_2600(int port) {
    if (getuid() == 1000 || getuid() == 0)
        return 0;

    char resp[512] = {0};
    /* the canonical probe: stops ssud then asks for identity */
    if (relay_roundtrip(port, RELAY_ESCALATE_CMD, resp, sizeof(resp), 1200) > 0) {
        if (strstr(resp, "uid=1000") || strstr(resp, "uid=0")) {
            if (g_verbose)
                printf("[+] UID-1000 escalation bridge confirmed on port %d\n", port);
            return 1;
        }
    }
    return -1;
}

/* =========================================================================
 *  4. CHANNEL OPENERS
 * ========================================================================= */
static int open_device_list(const char * const *list, channel_mode_t target_mode) {
    for (int i = 0; list[i] != NULL; i++) {
        if (access(list[i], R_OK | W_OK) != 0) continue;
        int fd = open(list[i], O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (fd >= 0) {
            if (isatty(fd)) {
                struct termios tio;
                if (tcgetattr(fd, &tio) == 0) {
                    cfmakeraw(&tio);
                    tio.c_cflag |= (CLOCAL | CREAD);
                    tio.c_cflag &= ~CRTSCTS;
                    tcsetattr(fd, TCSANOW, &tio);
                    tcflush(fd, TCIOFLUSH);
                }
            }
            g_ctx.mode = target_mode;
            g_ctx.fd   = fd;
            snprintf(g_ctx.active_path, sizeof(g_ctx.active_path), "%s", list[i]);
            return 0;
        }
    }
    return -1;
}

static int open_rild_socket(void) {
    for (int i = 0; ALL_RILD_SOCKETS[i] != NULL; i++) {
        if (access(ALL_RILD_SOCKETS[i], F_OK) != 0) continue;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, ALL_RILD_SOCKETS[i], sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            g_ctx.mode = CH_RILD_PARCEL;
            g_ctx.fd   = fd;
            snprintf(g_ctx.active_path, sizeof(g_ctx.active_path), "%s", ALL_RILD_SOCKETS[i]);
            return 0;
        }
        close(fd);
    }
    return -1;
}

/* ---- CH_SECRIL_LIB: dlopen libsecril-client.so and use send_recv_cmd ---- */
static int open_secril_library(void) {
#if GT_HAVE_SECRIL
    static const char *paths[] = {
        SECRIL_LIBRARY_PATH1, SECRIL_LIBRARY_PATH2, SECRIL_LIBRARY_PATH3, NULL
    };
    for (int i = 0; paths[i]; i++) {
        void *h = dlopen(paths[i], RTLD_NOW);
        if (!h) continue;
        secril_send_recv_fn fn = (secril_send_recv_fn)dlsym(h, SECRIL_SEND_RECV_SYM);
        if (!fn) { dlclose(h); continue; }
        g_ctx.dl_handle = h;
        g_secril_fn     = fn;
        g_ctx.mode      = CH_SECRIL_LIB;
        g_ctx.fd        = -1;
        snprintf(g_ctx.active_path, sizeof(g_ctx.active_path), "%s", paths[i]);
        if (g_verbose) printf("[+] libsecril channel open via %s\n", paths[i]);
        return 0;
    }
#endif
    return -1;
}

static int open_tcp_relay(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv = { .tv_sec = 1, .tv_usec = 500000 };
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        g_ctx.mode = CH_TCP_RELAY_2600;
        g_ctx.fd   = s;
        snprintf(g_ctx.active_path, sizeof(g_ctx.active_path), "tcp:127.0.0.1:%d", port);
        return 0;
    }
    close(s);
    return -1;
}

/* =========================================================================
 *  5. CHANNEL SELECTION
 * ========================================================================= */
static int init_best_channel(int prefer_at_tty) {
    if (g_ctx.fd >= 0) { close(g_ctx.fd); g_ctx.fd = -1; }
    g_ctx.dl_handle = NULL;
    g_secril_fn     = NULL;
    g_ctx.mode      = CH_NONE;

    if (prefer_at_tty) {
        if (open_device_list(ALL_AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    }

    if (open_secril_library() == 0)                      return 0;  /* strongest first */
    if (open_device_list(ALL_IPC_NODES, CH_DEV_IPC_RAW) == 0) return 0;
    if (open_rild_socket() == 0)                         return 0;
    if (open_device_list(ALL_AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    if (open_tcp_relay(RELAY_PORT_DEFAULT) == 0)         return 0;

    return -1;
}

static int force_specific_device(const char *path) {
    if (g_ctx.fd >= 0) { close(g_ctx.fd); g_ctx.fd = -1; }
    g_ctx.mode = CH_NONE;

    if (strncmp(path, "/dev/socket/", 12) == 0) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            struct sockaddr_un addr = {0};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                g_ctx.mode = CH_RILD_PARCEL;
                g_ctx.fd   = fd;
                snprintf(g_ctx.active_path, sizeof(g_ctx.active_path), "%s", path);
                return 0;
            }
            close(fd);
        }
    } else {
        int fd = open(path, O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (fd >= 0) {
            if (isatty(fd)) {
                struct termios tio;
                if (tcgetattr(fd, &tio) == 0) {
                    cfmakeraw(&tio);
                    tio.c_cflag |= (CLOCAL | CREAD);
                    tio.c_cflag &= ~CRTSCTS;
                    tcsetattr(fd, TCSANOW, &tio);
                    tcflush(fd, TCIOFLUSH);
                }
            }
            g_ctx.mode = (strstr(path, "ipc") || strstr(path, "diag"))
                             ? CH_DEV_IPC_RAW : CH_DEV_AT_TTY;
            g_ctx.fd   = fd;
            snprintf(g_ctx.active_path, sizeof(g_ctx.active_path), "%s", path);
            return 0;
        }
    }
    return -1;
}

/* =========================================================================
 *  6. CHANNEL SCAN (SCAN command)
 * ========================================================================= */
static void scan_devices(void) {
    printf("=== Available Modem Devices ===\n");
    for (int i = 0; ALL_AT_NODES[i] != NULL; i++) {
        if (access(ALL_AT_NODES[i], F_OK) == 0) {
            int rw = access(ALL_AT_NODES[i], R_OK | W_OK);
            printf("[+] %s: Found (TTY AT - %s)\n", ALL_AT_NODES[i],
                   rw == 0 ? "Read/Write OK" : "Permission Denied");
        }
    }
    for (int i = 0; ALL_IPC_NODES[i] != NULL; i++) {
        if (access(ALL_IPC_NODES[i], F_OK) == 0) {
            int rw = access(ALL_IPC_NODES[i], R_OK | W_OK);
            printf("[+] %s: Found (Direct IPC - %s)\n", ALL_IPC_NODES[i],
                   rw == 0 ? "Read/Write OK" : "Permission Denied");
        }
    }
    printf("=== RIL Sockets ===\n");
    for (int i = 0; ALL_RILD_SOCKETS[i] != NULL; i++) {
        if (access(ALL_RILD_SOCKETS[i], F_OK) == 0) {
            int s = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr = {0};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, ALL_RILD_SOCKETS[i], sizeof(addr.sun_path) - 1);
            int c = (s >= 0) ? connect(s, (struct sockaddr *)&addr, sizeof(addr)) : -1;
            printf("[%c] %s: Found (%s)\n", c == 0 ? '+' : '!', ALL_RILD_SOCKETS[i],
                   c == 0 ? "Connectable / Active" : "Exists, connect refused/SELinux blocked");
            if (s >= 0) close(s);
        }
    }
#if GT_HAVE_SECRIL
    printf("=== libsecril ===\n");
    if (g_secril_fn != NULL) {
        printf("[+] libsecril-client.so: loaded (%s)\n", g_ctx.active_path);
    } else {
        void *h = dlopen(SECRIL_LIBRARY_PATH1, RTLD_NOW);
        printf("[%c] libsecril-client.so: %s\n", h ? '+' : '!',
               h ? "loadable" : "not loadable on this build");
        if (h) dlclose(h);
    }
#endif
    fflush(stdout);
}

/* =========================================================================
 *  7. RAW IO HELPERS
 * ========================================================================= */
static int raw_write_all(int fd, const uint8_t *buf, int len) {
    int total = 0;
    while (total < len) {
        int n = write(fd, buf + total, len - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) { usleep(5000); continue; }
            return -1;
        }
        total += n;
    }
    return total;
}

static int raw_read_timeout(int fd, uint8_t *buf, int max_len, int timeout_ms) {
    fd_set rfds;
    struct timeval tv = { .tv_sec = timeout_ms / 1000,
                           .tv_usec = (timeout_ms % 1000) * 1000 };
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0) return 0;

    int total = 0;
    while (total < max_len) {
        int n = read(fd, buf + total, max_len - total);
        if (n <= 0) break;
        total += n;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec = 0; tv.tv_usec = 35000;
        if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0) break;
    }
    return total;
}

/* =========================================================================
 *  8. RILD OEM-HOOK PARCEL  ([4B BE size][4B req=59][4B token][4B dlen][ipc][pad4])
 * ========================================================================= */
static int send_rild_oem_parcel(int fd, const uint8_t *ipc_data, int ipc_len,
                                uint8_t *out_resp, int max_resp) {
    static uint32_t s_token = 1;
    uint8_t parcel[CMD_BUF_SIZE + 32];
    int offset = 4;

    uint32_t req_id = RIL_REQUEST_OEM_HOOK_RAW;
    uint32_t token  = s_token++;
    uint32_t dlen   = (uint32_t)ipc_len;

    memcpy(parcel + offset, &req_id, 4); offset += 4;
    memcpy(parcel + offset, &token,  4); offset += 4;
    memcpy(parcel + offset, &dlen,   4); offset += 4;
    memcpy(parcel + offset, ipc_data, ipc_len); offset += ipc_len;

    while ((offset - 4) % 4 != 0) parcel[offset++] = 0x00;

    uint32_t parcel_size_be = htonl(offset - 4);
    memcpy(parcel, &parcel_size_be, 4);

    if (raw_write_all(fd, parcel, offset) < 0) return -1;
    return raw_read_timeout(fd, out_resp, max_resp, 2500);
}

/* =========================================================================
 *  9. IPC FRAME DISPATCH (with libsecril + Qualcomm CSC fallback)
 * ========================================================================= */
static int dispatch_ipc_frame(const uint8_t *ipc_bytes, int len,
                              uint8_t *resp, int max_resp) {
    /* strongest: libsecril send_recv_cmd */
    if (g_ctx.mode == CH_SECRIL_LIB && g_secril_fn != NULL) {
        size_t out_len = 0;
        uint8_t out_buf[RESP_BUF_SIZE];
        int rc = g_secril_fn(NULL, RIL_REQUEST_OEM_HOOK_RAW,
                              ipc_bytes, (size_t)len,
                              out_buf, &out_len);
        if (rc == 0 && out_len > 0 && out_len < (size_t)max_resp) {
            memcpy(resp, out_buf, out_len);
            return (int)out_len;
        }
        /* fall through to fd-based channels */
    }

    if (g_ctx.fd < 0 && init_best_channel(0) != 0) {
        /* Multi-modal fallback — raw /dev + sockets blocked (Qualcomm SM-S918U):
         * extract CSC letters from a 06 01 frame and apply via setprop chain. */
        if (len >= 6 && ipc_bytes[0] == IPC_TYPE_CSC_MAIN && ipc_bytes[1] == IPC_TYPE_CSC_SUB) {
            char extracted_csc[8] = {0};
            int csc_idx = 0;
            for (int i = 6; i < len && csc_idx < 5; i++) {
                if (ipc_bytes[i] >= 'A' && ipc_bytes[i] <= 'Z') {
                    extracted_csc[csc_idx++] = (char)ipc_bytes[i];
                }
            }
            if (csc_idx >= 2) {
                char prop_cmd[256];
                snprintf(prop_cmd, sizeof(prop_cmd),
                         "setprop persist.sys.omc_sales_code %s; "
                         "setprop persist.sys.sales_code %s; "
                         "setprop ro.csc.sales_code %s; "
                         "am broadcast -a com.samsung.intent.action.CSC_COMPLETED "
                         "--es csc %s >/dev/null 2>&1",
                         extracted_csc, extracted_csc, extracted_csc, extracted_csc);
                system(prop_cmd);
            }
            if (resp && max_resp >= 4) {
                resp[0] = IPC_TYPE_CSC_MAIN;
                resp[1] = IPC_TYPE_CSC_SUB;
                resp[2] = 0x00;
                resp[3] = 0x00;
                return 4;
            }
            return 0;
        }
        return -1;
    }

    if (g_ctx.mode == CH_RILD_PARCEL) {
        return send_rild_oem_parcel(g_ctx.fd, ipc_bytes, len, resp, max_resp);
    }

    if (raw_write_all(g_ctx.fd, ipc_bytes, len) < 0) return -1;
    return raw_read_timeout(g_ctx.fd, resp, max_resp, 2500);
}

/* =========================================================================
 * 10. HEX <-> BIN
 * ========================================================================= */
static int hex2bin(const char *hex, uint8_t *out, int max_out) {
    int len = 0;
    while (*hex && *(hex + 1) && len < max_out) {
        if (*hex == ' ' || *hex == ':') { hex++; continue; }
        unsigned int b;
        char pair[3] = { hex[0], hex[1], 0 };
        if (sscanf(pair, "%02x", &b) != 1) break;
        out[len++] = (uint8_t)b;
        hex += 2;
    }
    return len;
}

static void bin2hex(const uint8_t *in, int len, char *out) {
    for (int i = 0; i < len; i++) sprintf(out + (i * 2), "%02X", in[i]);
    out[len * 2] = '\0';
}

/* =========================================================================
 * 11. SYNCHRONOUS AT COMMAND SENDER (multi-node aware, relay-safe)
 * ========================================================================= */
static int send_at_command_sync(const char *cmd, char *out_resp, int max_resp,
                                int timeout_ms) {
    if (!cmd) return -1;

    if (g_ctx.mode != CH_DEV_AT_TTY && g_ctx.mode != CH_TCP_RELAY_2600) {
        init_best_channel(1);
    }
    if (g_ctx.fd < 0) return -1;

    /* strip trailing CRLF / spaces */
    char clean_cmd[CMD_BUF_SIZE];
    strncpy(clean_cmd, cmd, sizeof(clean_cmd) - 1);
    clean_cmd[sizeof(clean_cmd) - 1] = '\0';
    int len = (int)strlen(clean_cmd);
    while (len > 0 && (clean_cmd[len-1] == '\r' || clean_cmd[len-1] == '\n' ||
                       clean_cmd[len-1] == ' ')) {
        clean_cmd[--len] = '\0';
    }

    char buf[CMD_BUF_SIZE];
    int l;
    if (g_ctx.mode == CH_TCP_RELAY_2600) {
        /* through the UID-1000 netcat relay: wrap in a shell one-liner that
         * tries every known AT node in order (first writable wins) */
        l = snprintf(buf, sizeof(buf),
            "for n in /dev/umts_atc0 /dev/umts_atc1 /dev/umts_at0 "
            "/dev/at_distributor /dev/at_distributor0 /dev/smd11 /dev/ttyGS0; do "
            "if [ -w \"$n\" ]; then "
            "echo -e '%s\\r' > \"$n\" 2>/dev/null; "
            "head -n 10 \"$n\" 2>/dev/null; break; fi; done\n",
            clean_cmd);
    } else {
        l = snprintf(buf, sizeof(buf), "%s\r\n", clean_cmd);
    }

    if (raw_write_all(g_ctx.fd, (const uint8_t *)buf, l) <= 0) return -1;

    int r = raw_read_timeout(g_ctx.fd, (uint8_t *)out_resp, max_resp - 1, timeout_ms);
    if (r >= 0) out_resp[r] = '\0';
    return r;
}

/* =========================================================================
 *  12. SIM & NETWORK LOCK INSPECTION (LOCKS command)
 * ========================================================================= */
static const char *clck_state(const char *raw) {
    if (strstr(raw, "+CLCK: 0") || strstr(raw, "+CLCK:0")) return "UNLOCKED(0)";
    if (strstr(raw, "+CLCK: 1") || strstr(raw, "+CLCK:1")) return "LOCKED(1)";
    return "UNKNOWN";
}

static void query_sim_locks(int verbose) {
    char cpin_resp[256] = {0};
    char clck_pn[256] = {0}, clck_pu[256] = {0}, clck_pp[256] = {0};
    char clck_pc[256] = {0}, clck_sc[256] = {0};

    send_at_command_sync(AT_CMD_SIM_STATUS, cpin_resp, sizeof(cpin_resp), 1200);
    send_at_command_sync(AT_CMD_QUERY_NETWORK_LOCK, clck_pn, sizeof(clck_pn), 1200);
    send_at_command_sync(AT_CMD_QUERY_SUBSET_LOCK,  clck_pu, sizeof(clck_pu), 1200);
    send_at_command_sync(AT_CMD_QUERY_SP_LOCK,      clck_pp, sizeof(clck_pp), 1200);
    send_at_command_sync(AT_CMD_QUERY_CORP_LOCK,    clck_pc, sizeof(clck_pc), 1200);
    send_at_command_sync(AT_CMD_QUERY_SIM_LOCK,     clck_sc, sizeof(clck_sc), 1200);

    const char *sim_state = "UNKNOWN";
    if (strstr(cpin_resp, "READY")) sim_state = "READY";
    else if (strstr(cpin_resp, "SIM PIN")) sim_state = "PIN_REQUIRED";
    else if (strstr(cpin_resp, "SIM PUK")) sim_state = "PUK_REQUIRED";
    else if (strstr(cpin_resp, "NOT INSERTED") || strstr(cpin_resp, "ERROR: 10")) sim_state = "ABSENT";
    else if (strstr(cpin_resp, "ERROR")) sim_state = "MODEM_ERROR";

    if (verbose) {
        printf("=== SIM & Network Personalization Locks ===\n");
        printf("  SIM State (CPIN)             : %s\n", sim_state);
        printf("  Network Lock (CLCK:PN)       : %s\n", clck_state(clck_pn));
        printf("  Network Subset Lock (CLCK:PU): %s\n", clck_state(clck_pu));
        printf("  Service Provider Lock (PP)   : %s\n", clck_state(clck_pp));
        printf("  Corporate Lock (CLCK:PC)     : %s\n", clck_state(clck_pc));
        printf("  SIM Lock (CLCK:SC)           : %s\n", clck_state(clck_sc));
    }
    printf(SIMLOCK_FORMAT_STRING,
           sim_state, clck_state(clck_pn), clck_state(clck_pu),
           clck_state(clck_pp), clck_state(clck_pc), clck_state(clck_sc));
    fflush(stdout);
}

/* =========================================================================
 *  13. BOOST / SPRINT UNLOCK  (5 steps — always ends with .OK_BOOST.. marker)
 * ========================================================================= */
static int execute_boost_unlock(int verbose) {
    char resp[RESP_BUF_SIZE] = {0};

    if (verbose) printf("[*] Starting Sprint / Boost Mobile unlock sequence...\n");

    if (verbose) printf("  [1/5] Sending " AT_CMD_UNLOCKSPR_OLD);
    send_at_command_sync(AT_CMD_UNLOCKSPR_OLD, resp, sizeof(resp), 1500);

    if (verbose) printf("  [2/5] Sending " AT_CMD_SIMLOCKU_RESET);
    send_at_command_sync(AT_CMD_SIMLOCKU_RESET, resp, sizeof(resp), 1500);

    if (verbose) printf("  [3/5] Neutralizing Sprint OMADM & carrier-lock packages...\n");
    system(CMD_UNINSTALL_OMADMSPR);
    system(CMD_UNINSTALL_AUTOPRECONFIG);
    system(CMD_FORCESTOP_SKATD);

    if (verbose) printf("  [4/5] Flushing radio logcat...\n");
    system(CMD_FLUSH_RADIO_LOGS);

    if (verbose) printf("  [5/5] Re-initializing radio modem (AT+CFUN=1,1)...\n");
    send_at_command_sync(AT_CMD_REBOOT_RADIO, resp, sizeof(resp), 2000);

    if (verbose) printf("[+] Sprint/Boost unlock sequence completed.\n");
    printf(MARK_OK_BOOST);
    fflush(stdout);
    return 0;
}

/* =========================================================================
 *  14. CARRIER RELOCK (SRELOCK1 / SETSPR — MCCREQ BCD payload)
 * ========================================================================= */
static int execute_carrier_relock(const char *mcc_mnc_bcd_hex, int verbose) {
    if (!mcc_mnc_bcd_hex || strlen(mcc_mnc_bcd_hex) < 6 ||
        strspn(mcc_mnc_bcd_hex, "0123456789ABCDEFabcdef") != strlen(mcc_mnc_bcd_hex)) {
        if (verbose)
            printf("[ERROR] Invalid MCC/MNC BCD hex payload: %s "
                   "(expected >= 6 hex chars, e.g. 421F0100)\n",
                   mcc_mnc_bcd_hex ? mcc_mnc_bcd_hex : "NULL");
        printf(MARK_ERR_RELOCK_INVALID);
        fflush(stdout);
        return -1;
    }

    if (verbose) printf("[*] Executing carrier relock for BCD payload: %s\n", mcc_mnc_bcd_hex);

    /* SRELOCK1 / SETSPR frame: 06 01 <LEN:4hex> 00 00 <BCD payload> */
    char pkt[512];
    uint8_t bin[256];
    uint8_t resp[RESP_BUF_SIZE];

    int hex_len = (int)strlen(mcc_mnc_bcd_hex);
    int total_bytes = 4 + (hex_len / 2);
    snprintf(pkt, sizeof(pkt), IPC_RELOCK_FMT, (unsigned)total_bytes, mcc_mnc_bcd_hex);

    int bin_len = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  Dispatching SRELOCK1 frame: %s\n", pkt);
    int r = dispatch_ipc_frame(bin, bin_len, resp, sizeof(resp));
    if (verbose && r > 0) {
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        bin2hex(resp, r, resp_hex);
        printf("  RIL response (%d bytes): %s\n", r, resp_hex);
    }

    /* radio refresh to enforce the profile */
    char at_resp[256] = {0};
    send_at_command_sync(AT_CMD_REBOOT_RADIO, at_resp, sizeof(at_resp), 1500);

    if (verbose) printf("[+] Carrier relock dispatched. Radio reset initiated.\n");
    printf(MARK_OK_RELOCK);
    fflush(stdout);
    return 0;
}

/* =========================================================================
 *  15. DEEP TELEMETRY (DEVINFO)
 * ========================================================================= */
static void clean_crlf(char *s) {
    for (char *p = s; *p; p++)
        if (*p == '\r' || *p == '\n') *p = ' ';
}

static void print_full_telemetry(int verbose) {
    char model[64], brand[64], csc[64], ver[64], patch[64], build[64], serial[64], prod[64];
    fast_getprop("ro.product.model",             model,  sizeof(model));
    fast_getprop("ro.product.brand",             brand,  sizeof(brand));
    fast_getprop("ro.csc.sales_code",            csc,    sizeof(csc));
    fast_getprop("ro.build.version.release",     ver,    sizeof(ver));
    fast_getprop("ro.build.version.security_patch", patch, sizeof(patch));
    fast_getprop("ro.build.display.id",          build,  sizeof(build));
    fast_getprop("ro.boot.serialno",             serial, sizeof(serial));
    fast_getprop("ril.product_code",             prod,   sizeof(prod));

    char devcon[RESP_BUF_SIZE] = {0};
    char vers_sw[256] = {0}, imei[256] = {0}, imsi[256] = {0}, sn_at[256] = {0};
    char batt[256] = {0}, sysscope[256] = {0};

    send_at_command_sync(AT_CMD_DEVCONINFO,     devcon,  sizeof(devcon),  2000);
    send_at_command_sync(AT_CMD_VERSNAME_SW,    vers_sw, sizeof(vers_sw), 1200);
    send_at_command_sync(AT_CMD_IMEITEST,       imei,    sizeof(imei),    1200);
    send_at_command_sync(AT_CMD_SERIALNO,       sn_at,   sizeof(sn_at),   1200);
    send_at_command_sync(AT_CMD_CIMI,           imsi,    sizeof(imsi),    1200);
    send_at_command_sync(AT_CMD_BATTEST,        batt,    sizeof(batt),    1200);
    send_at_command_sync(AT_CMD_SYSSCOPE,       sysscope,sizeof(sysscope),1200);

    clean_crlf(devcon);   clean_crlf(vers_sw);  clean_crlf(imei);
    clean_crlf(imsi);     clean_crlf(sn_at);    clean_crlf(batt);
    clean_crlf(sysscope);

    if (verbose) {
        printf("=== Deep Telemetry & Hardware Identification ===\n");
        printf("  Model         : %s\n",  model);
        printf("  Brand         : %s\n",  brand);
        printf("  CSC Code      : %s\n",  csc);
        printf("  Android Ver   : %s\n",  ver);
        printf("  Security Patch: %s\n",  patch);
        printf("  Build ID      : %s\n",  build);
        printf("  Serial (Prop) : %s\n",  serial);
        printf("  Product Code  : %s\n",  prod);
        printf("  DEVCONINFO    : %s\n",  devcon);
        printf("  VERSNAME SW   : %s\n",  vers_sw);
        printf("  IMEITEST      : %s\n",  imei);
        printf("  SERIALNO AT   : %s\n",  sn_at);
        printf("  CIMI (IMSI)   : %s\n",  imsi);
        printf("  BATTEST       : %s\n",  batt);
        printf("  SYSSCOPE      : %s\n",  sysscope);
        printf("  Active Channel: %s (%d)\n", g_ctx.active_path, g_ctx.mode);
    }
    printf(DEVINFO_FORMAT_STRING,
           model, brand, csc, ver, patch, build, serial, prod,
           vers_sw, imei, imsi, devcon);
    fflush(stdout);
}

/* =========================================================================
 *  16. RADIO & POWER ACTIONS
 * ========================================================================= */
static int execute_radio_action(const char *action, int verbose) {
    char resp[RESP_BUF_SIZE] = {0};
    const char *cmd = NULL;

    if (strcasecmp(action, "REBOOT") == 0 || strcasecmp(action, "RESET") == 0) {
        cmd = AT_CMD_REBOOT_RADIO;
        if (verbose) printf("[*] Radio soft reboot (AT+CFUN=1,1)...\n");
    } else if (strcasecmp(action, "OFF") == 0 || strcasecmp(action, "AIRPLANE") == 0) {
        cmd = AT_CMD_RADIO_OFF;
        if (verbose) printf("[*] Radio off / airplane (AT+CFUN=0)...\n");
    } else if (strcasecmp(action, "ON") == 0) {
        cmd = AT_CMD_RADIO_ON;
        if (verbose) printf("[*] Radio on (AT+CFUN=1)...\n");
    } else if (strcasecmp(action, "FACTORST") == 0) {
        cmd = AT_CMD_MODEM_FACTORY_RESET;
        if (verbose) printf("[*] Modem NV factory reset (AT+FACTORST=0,0)...\n");
    } else if (strcasecmp(action, "SWRESET") == 0) {
        cmd = AT_CMD_SWRESET;
        if (verbose) printf("[*] Full phone reboot (AT+SWRESET)...\n");
    } else {
        if (verbose) printf("[ERROR] Unknown radio action: %s\n", action);
        printf(MARK_ERR_UNKNOWN_ACTION);
        fflush(stdout);
        return -1;
    }

    send_at_command_sync(cmd, resp, sizeof(resp), 3000);
    clean_crlf(resp);
    if (verbose) printf("[+] Radio action '%s' executed. Response: %s\n", action, resp);
    printf(MARK_OK_RADIO);
    fflush(stdout);
    return 0;
}

/* =========================================================================
 *  17. SECURITY CHALLENGE (AKSEED)
 * ========================================================================= */
static void query_akseedno(int verbose) {
    char resp[RESP_BUF_SIZE] = {0};
    send_at_command_sync(AT_CMD_AKSEEDNO_REQ, resp, sizeof(resp), 2000);
    clean_crlf(resp);

    if (verbose) {
        printf("=== Modem Access Key Seed (Challenge) ===\n");
        printf("  Result: %s\n", resp);
    }
    printf(AKSEED_FORMAT_STRING, resp);
    fflush(stdout);
}

/* =========================================================================
 *  18. SAMSUNG CSC ENGINE — 5 IPC packets + AT chain + setprop + verify
 * ========================================================================= */
static int execute_samsung_csc_change(const char *target_csc, int verbose) {
    if (!target_csc || strlen(target_csc) < 2 || strlen(target_csc) > 5) {
        if (verbose)
            printf("[ERROR] Invalid CSC code: %s (must be 2-5 chars, e.g. XAA)\n",
                   target_csc ? target_csc : "NULL");
        printf(MARK_ERR_CSC_INVALID);
        fflush(stdout);
        return -1;
    }

    /* normalize to uppercase */
    char clean_csc[8] = {0};
    for (int i = 0; target_csc[i] && i < 5; i++) {
        char c = target_csc[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        clean_csc[i] = c;
    }

    char hex_csc[16] = {0};
    for (int i = 0; clean_csc[i]; i++)
        sprintf(hex_csc + (i * 2), "%02X", (uint8_t)clean_csc[i]);

    if (verbose)
        printf("[*] Starting Samsung CSC change to: %s (hex: %s)\n", clean_csc, hex_csc);

    char pkt[512];
    uint8_t bin[256];
    uint8_t resp[RESP_BUF_SIZE];
    int r;

    /* --- Phase 1: the 5-packet IPC injection ------------------------- */
    snprintf(pkt, sizeof(pkt), IPC_CSC_PKT1_FMT, hex_csc, hex_csc);
    r = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [1/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, r, resp, sizeof(resp));

    snprintf(pkt, sizeof(pkt), IPC_CSC_PKT2_FMT, hex_csc);
    r = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [2/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, r, resp, sizeof(resp));

    snprintf(pkt, sizeof(pkt), IPC_CSC_PKT3_FMT, hex_csc, hex_csc, hex_csc);
    r = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [3/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, r, resp, sizeof(resp));

    snprintf(pkt, sizeof(pkt), "%s", IPC_CSC_PKT4_STR);
    r = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [4/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, r, resp, sizeof(resp));

    snprintf(pkt, sizeof(pkt), IPC_CSC_PKT5_FMT, hex_csc);
    r = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [5/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, r, resp, sizeof(resp));

    /* --- Phase 2: canonical AT chain (SWATD/ACTIVATE/PRECONFG/RFBYCODE) - */
    char at_cmd[128];
    char at_resp[512] = {0};

    send_at_command_sync(AT_CMD_SWATD_0, at_resp, sizeof(at_resp), 1000);
    send_at_command_sync(AT_CMD_ACTIVATE, at_resp, sizeof(at_resp), 1000);
    send_at_command_sync(AT_CMD_SWATD_1, at_resp, sizeof(at_resp), 1000);

    snprintf(at_cmd, sizeof(at_cmd), AT_CMD_PRECONFG_WRITE_FMT, clean_csc);
    send_at_command_sync(at_cmd, at_resp, sizeof(at_resp), 2000);
    if (verbose) { clean_crlf(at_resp);
        printf("  AT+PRECONFG=2,%s -> %s\n", clean_csc, at_resp); }

    /* --- Phase 3: verify by reading back ------------------------------ */
    char verify[512] = {0};
    send_at_command_sync(AT_CMD_PRECONFG_READ, verify, sizeof(verify), 1500);
    clean_crlf(verify);
    int verified = strstr(verify, clean_csc) != NULL;
    if (verbose)
        printf("  Verify AT+PRECONFG=1,0 -> %s  [%s]\n", verify,
               verified ? "MATCH" : "NO MATCH (will rely on setprop)");

    snprintf(at_cmd, sizeof(at_cmd), AT_CMD_RFBYCODE_FMT, clean_csc);
    send_at_command_sync(at_cmd, at_resp, sizeof(at_resp), 1000);

    /* --- Phase 4: persistent system properties + broadcast ------------ */
    char prop_cmd[320];
    snprintf(prop_cmd, sizeof(prop_cmd),
             "setprop persist.sys.omc_sales_code %s; "
             "setprop persist.sys.sales_code %s; "
             "setprop persist.sys.prev_salescode $(getprop ro.csc.sales_code); "
             "setprop ro.csc.sales_code %s; "
             "am broadcast -a com.samsung.intent.action.CSC_COMPLETED "
             "--es csc %s >/dev/null 2>&1",
             clean_csc, clean_csc, clean_csc, clean_csc);
    system(prop_cmd);

    /* --- Phase 5: carrier package cleanup + radio flush ---------------- */
    if (verbose) printf("[*] Running carrier package cleanup...\n");
    system(CMD_UNINSTALL_AUTOPRECONFIG);
    system(CMD_UNINSTALL_OMADMSPR);
    system(CMD_UNINSTALL_CARRIER_CODE_CHANGER);
    system(CMD_FLUSH_RADIO_LOGS);

    if (verbose) printf("[+] Samsung CSC change completed for: %s (verified=%s)\n",
                         clean_csc, verified ? "yes" : "via-fallback");
    printf(MARK_OK_CSC);
    fflush(stdout);
    return 0;
}

/* =========================================================================
 *  19. BANNER / STATUS / PROPS / VERSION
 * ========================================================================= */
static void print_compact_info(void) {
    char model[64], csc[64], ver[64], patch[64], debug_lvl[64];
    fast_getprop("ro.product.model",                 model, sizeof(model));
    fast_getprop("ro.csc.sales_code",                csc,   sizeof(csc));
    fast_getprop("ro.build.version.release",         ver,   sizeof(ver));
    fast_getprop("ro.build.version.security_patch",  patch, sizeof(patch));
    fast_getprop("ro.boot.debug_level",              debug_lvl, sizeof(debug_lvl));

    printf(TELEMETRY_FORMAT_STRING,
           model, csc, ver, patch, debug_lvl, getuid(),
           g_ctx.mode != CH_NONE ? g_ctx.active_path : "NONE");
    fflush(stdout);
}

static void print_version(void) {
    printf(VERSION_FORMAT_STRING, g_ctx.active_path, (int)g_ctx.mode);
    fflush(stdout);
}

static void print_props(void) {
    printf("PROPS|");
    for (int i = 0; ALL_SYSTEM_PROPERTIES[i] != NULL; i++) {
        char val[96] = {0};
        fast_getprop(ALL_SYSTEM_PROPERTIES[i], val, sizeof(val));
        printf("%s=%s%s", ALL_SYSTEM_PROPERTIES[i], val,
               ALL_SYSTEM_PROPERTIES[i + 1] ? "|" : "");
    }
    printf("\n");
    fflush(stdout);
}

static void print_detailed_status(void) {
    char model[64], csc[64], ver[64], patch[64], debug_lvl[64], brand[64], serial[64];
    fast_getprop("ro.product.model",                model, sizeof(model));
    fast_getprop("ro.product.brand",                brand, sizeof(brand));
    fast_getprop("ro.csc.sales_code",               csc,   sizeof(csc));
    fast_getprop("ro.build.version.release",        ver,   sizeof(ver));
    fast_getprop("ro.build.version.security_patch", patch, sizeof(patch));
    fast_getprop("ro.boot.debug_level",             debug_lvl, sizeof(debug_lvl));
    fast_getprop("ro.boot.serialno",                serial, sizeof(serial));

    printf("=== Device Status (GsmTools Bridge v%s) ===\n", GT_VERSION);
    printf("  Product Brand  : %s\n", brand);
    printf("  Product Model  : %s\n", model);
    printf("  Serial Number  : %s\n", serial);
    printf("  Device Carrier : %s\n", csc);
    printf("  Android Version: %s\n", ver);
    printf("  Security Patch : %s\n", patch);
    printf("  Debug Level    : %s\n", debug_lvl);
    printf("  Process UID    : %d\n", getuid());
    printf("  Active Channel : %s (%s)\n",
           g_ctx.mode != CH_NONE ? g_ctx.active_path : "NONE",
           g_ctx.mode == CH_SECRIL_LIB    ? "libsecril-client" :
           g_ctx.mode == CH_RILD_PARCEL   ? "RILD Parcel" :
           g_ctx.mode == CH_DEV_IPC_RAW   ? "Raw IPC" :
           g_ctx.mode == CH_DEV_AT_TTY    ? "AT TTY" :
           g_ctx.mode == CH_TCP_RELAY_2600? "TCP Port 2600 Relay" : "Disconnected");
    query_sim_locks(1);
}

/* =========================================================================
 *  20. HELP
 * ========================================================================= */
static void print_help(const char *prog_name) {
    printf("============================================================\n");
    printf("  GsmTools - Samsung AT / RIL / Boost Bridge (v%s)\n", GT_VERSION);
    printf("============================================================\n");
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("One-shot CLI options:\n");
    printf("  -c <CSC>             Quick Samsung CSC change (e.g. -c XAA)\n");
    printf("  -b, --boost, /boost   Sprint / Boost unlock sequence\n");
    printf("  -r <BCD_HEX>         Carrier relock (e.g. -r 421F0100)\n");
    printf("  -t, --telemetry      Deep telemetry (DEVCONINFO, VERSNAME, ...)\n");
    printf("  -a <AT_CMD>          Send single AT command and exit\n");
    printf("  -x <HEX>             Send single hex IPC frame and exit\n");
    printf("  -s                    Scan accessible modem & RIL channels\n");
    printf("  -l                    Query SIM & network personalization locks\n");
    printf("    --seed              Query security challenge (AT+AKSEEDNO=1,0)\n");
    printf("    --radio <ACT>       REBOOT | OFF | ON | FACTORST | SWRESET\n");
    printf("  -d <DEVICE>          Force specific modem device path\n");
    printf("  -p <PORT>            UID-1000 relay port (default 2600)\n");
    printf("  -v                    Verbose diagnostic output\n");
    printf("  -h, --help           This help\n\n");
    printf("Interactive session commands:\n");
    printf("  CSC:<CODE>           Automated 5-packet Samsung CSC change\n");
    printf("  BOOST | /boost       Sprint / Boost unlock sequence\n");
    printf("  RELOCK:<BCD>         Carrier relock with BCD MCC/MNC payload\n");
    printf("  RADIO:<ACT>          Radio power & reboot control\n");
    printf("  DEVINFO              Deep telemetry header\n");
    printf("  AKSEED               Modem security challenge query\n");
    printf("  AT+... or AT:<CMD>   Raw AT command (GETSPR/SETSPR/SRELOCK1 pass through)\n");
    printf("  HEX:<DATA>           Hex IPC frame (or bare hex)\n");
    printf("  SCAN | STATUS        Channel scan / full device status\n");
    printf("  LOCKS                SIM & network lock query (CPIN + 5xCLCK)\n");
    printf("  RECONNECT            Re-probe best modem channel\n");
    printf("  MODE:AT | MODE:IPC   Switch channel preference\n");
    printf("  AUTH:<SEED_HEX>      Activate Xorshift32 session cipher (HEX payloads)\n");
    printf("  INFO | PROPS | VERSION  Compact telemetry / property dump / version\n");
    printf("  EXIT, QUIT           Terminate session\n");
    printf("============================================================\n");
    fflush(stdout);
}

/* =========================================================================
 *  21. MAIN — CLI one-shot & interactive loop
 * ========================================================================= */
int main(int argc, char *argv[]) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    const char *cli_csc = NULL, *cli_at = NULL, *cli_hex = NULL;
    const char *cli_dev = NULL, *cli_relock = NULL, *cli_radio = NULL;
    int cli_boost = 0, cli_telemetry = 0, cli_seed = 0;
    int cli_scan = 0, cli_locks = 0;
    int esc_port = RELAY_PORT_DEFAULT;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_help(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "-v")) {
            g_verbose = 1;
        } else if (!strcmp(argv[i], "-s")) {
            cli_scan = 1;
        } else if (!strcmp(argv[i], "-l")) {
            cli_locks = 1;
        } else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--boost") ||
                   !strcmp(argv[i], "/boost") || !strcmp(argv[i], "-boost") ||
                   !strcmp(argv[i], "boost")) {
            cli_boost = 1;
        } else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--telemetry")) {
            cli_telemetry = 1;
        } else if (!strcmp(argv[i], "--seed")) {
            cli_seed = 1;
        } else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            cli_csc = argv[++i];
        } else if (!strcmp(argv[i], "-r") && i + 1 < argc) {
            cli_relock = argv[++i];
        } else if (!strcmp(argv[i], "--radio") && i + 1 < argc) {
            cli_radio = argv[++i];
        } else if (!strcmp(argv[i], "-a") && i + 1 < argc) {
            cli_at = argv[++i];
        } else if (!strcmp(argv[i], "-x") && i + 1 < argc) {
            cli_hex = argv[++i];
        } else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            cli_dev = argv[++i];
        } else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            esc_port = atoi(argv[++i]);
        }
    }

    try_escalate_via_port_2600(esc_port);

    if (cli_dev) {
        force_specific_device(cli_dev);
    } else {
        init_best_channel((cli_at || cli_telemetry || cli_seed || cli_radio) ? 1 : 0);
    }

    if (cli_scan)     { scan_devices();          goto done; }
    if (cli_locks)    { query_sim_locks(1);       goto done; }
    if (cli_telemetry){ print_full_telemetry(g_verbose); goto done; }
    if (cli_seed)     { query_akseedno(g_verbose);goto done; }
    if (cli_boost)    { int rc = execute_boost_unlock(g_verbose); close(g_ctx.fd); return rc; }
    if (cli_relock)   { int rc = execute_carrier_relock(cli_relock, g_verbose); close(g_ctx.fd); return rc; }
    if (cli_radio)    { int rc = execute_radio_action(cli_radio, g_verbose); close(g_ctx.fd); return rc; }

    if (cli_at) {
        char at_resp[RESP_BUF_SIZE] = {0};
        int r = send_at_command_sync(cli_at, at_resp, sizeof(at_resp), 2500);
        if (r > 0) printf("%s\n", at_resp);
        else printf(MARK_ERR_AT_TIMEOUT);
        goto done;
    }

    if (cli_hex) {
        uint8_t payload[CMD_BUF_SIZE];
        uint8_t resp[RESP_BUF_SIZE];
        int l = hex2bin(cli_hex, payload, sizeof(payload));
        if (l > 0) {
            int r = dispatch_ipc_frame(payload, l, resp, sizeof(resp));
            if (r > 0) {
                char resp_hex[RESP_BUF_SIZE * 2 + 1];
                bin2hex(resp, r, resp_hex);
                printf("%s\n", resp_hex);
            } else {
                printf(MARK_ERR_IPC);
            }
            close(g_ctx.fd);
            return (r > 0) ? 0 : 1;
        }
        printf(MARK_ERR_HEX_SYNTAX);
        goto done;
    }

    if (cli_csc) {
        int rc = execute_samsung_csc_change(cli_csc, g_verbose);
        close(g_ctx.fd);
        return rc;
    }

    /* ============ interactive session (C# desktop contract) ============ */
    print_compact_info();

    char line[CMD_BUF_SIZE * 2 + 64];
    uint8_t payload[CMD_BUF_SIZE];
    uint8_t resp[RESP_BUF_SIZE];
    char resp_hex[RESP_BUF_SIZE * 2 + 1];

    printf(PROMPT);
    fflush(stdout);

    while (g_running && fgets(line, sizeof(line), stdin) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (!line[0]) { printf(PROMPT); fflush(stdout); continue; }

        if (strcasecmp(line, PROTO_CMD_EXIT) == 0 || strcasecmp(line, PROTO_CMD_QUIT) == 0)
            break;

        if (strcasecmp(line, PROTO_CMD_HELP) == 0) {
            print_help(argv[0]); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_SCAN) == 0) {
            scan_devices(); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_STATUS) == 0) {
            print_detailed_status(); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_LOCKS) == 0 || strcasecmp(line, PROTO_CMD_SIMLOCK) == 0) {
            query_sim_locks(0); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_DEVINFO) == 0 || strcasecmp(line, PROTO_CMD_TELEMETRY) == 0) {
            print_full_telemetry(0); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_AKSEED) == 0) {
            query_akseedno(0); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_VERSION) == 0) {
            print_version(); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_PROPS) == 0) {
            print_props(); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_BOOST_UNLOCK) == 0 ||
            strcasecmp(line, "/boost") == 0 || strcasecmp(line, "-boost") == 0 ||
            strcasecmp(line, "--boost") == 0 ||
            strcasecmp(line, "BOOST_UNLOCK") == 0 ||
            strcasecmp(line, "SPR_UNLOCK") == 0 ||
            strncasecmp(line, PROTO_PREFIX_BOOST, 6) == 0 ||
            strncasecmp(line, "/BOOST:", 7) == 0) {
            execute_boost_unlock(0); printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, PROTO_PREFIX_RELOCK, 7) == 0) {
            execute_carrier_relock(line + 7, 0); printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, PROTO_PREFIX_RADIO, 6) == 0) {
            execute_radio_action(line + 6, 0); printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_RECONNECT) == 0) {
            init_best_channel(0);
            printf(MARK_CH_FMT, g_ctx.active_path);
            printf(PROMPT); fflush(stdout); continue;
        }
        if (strcasecmp(line, PROTO_CMD_INFO) == 0) {
            print_compact_info(); printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, PROTO_PREFIX_AUTH, 5) == 0) {
            g_session_seed = (uint32_t)strtoul(line + 5, NULL, 16);
            printf(MARK_OK_AUTH);
            printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, PROTO_PREFIX_CSC, 4) == 0) {
            execute_samsung_csc_change(line + 4, 0);
            printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, "MODE:AT", 7) == 0) {
            init_best_channel(1);
            printf(MARK_CH_FMT, g_ctx.active_path);
            printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, "MODE:IPC", 8) == 0) {
            init_best_channel(0);
            printf(MARK_CH_FMT, g_ctx.active_path);
            printf(PROMPT); fflush(stdout); continue;
        }
        if (strncasecmp(line, "AT:", 3) == 0 ||
            strncasecmp(line, "AT", 2) == 0 ||
            line[0] == '+' || line[0] == '$' ||
            strncasecmp(line, RIL_CMD_GETSPR, 6) == 0 ||
            strncasecmp(line, RIL_CMD_SETSPR, 6) == 0 ||
            strncasecmp(line, RIL_CMD_SRELOCK, 7) == 0) {
            /* raw AT passthrough (AT: prefix optional).
             * SRELOCK matched on 7-char prefix like the original dispatcher. */
            const char *at_cmd = (strncasecmp(line, "AT:", 3) == 0) ? line + 3 : line;
            char at_out[RESP_BUF_SIZE] = {0};
            int r = send_at_command_sync(at_cmd, at_out, sizeof(at_out), 2500);
            if (r > 0) printf("%s\n", at_out);
            else printf(MARK_TIMEOUT);
            printf(PROMPT); fflush(stdout); continue;
        }

        /* fallback: hex IPC frame (optionally HEX:-prefixed) */
        const char *hex_ptr = line;
        if (strncasecmp(line, PROTO_PREFIX_HEX, 4) == 0) hex_ptr = line + 4;

        int bin_len = hex2bin(hex_ptr, payload, sizeof(payload));
        if (bin_len > 0) {
            transform_buffer(payload, bin_len, g_session_seed);

            int r = dispatch_ipc_frame(payload, bin_len, resp, sizeof(resp));
            memset(payload, 0, sizeof(payload));

            if (r > 0) {
                transform_buffer(resp, r, g_session_seed);
                bin2hex(resp, r, resp_hex);
                printf(MARK_OK_HEX_FMT, resp_hex);
            } else if (r == 0) {
                printf(MARK_OK_EMPTY);
            } else {
                printf(MARK_ERR_IPC);
            }
        } else {
            printf(MARK_ERR_SYNTAX);
        }

        printf(PROMPT);
        fflush(stdout);
    }

done:
    if (g_ctx.fd >= 0) close(g_ctx.fd);
    return 0;
}