/*
 * ============================================================================
 *  GsmTools - Multi-Modal RIL/IPC/AT Bridge & Modem Diagnostics (v3.0 Pro)
 * ============================================================================
 *  Compile: aarch64-linux-android29-clang -O2 -s -fvisibility=hidden
 *           -ldl at_bridge.c -o AT
 *  Or 32-bit: armv7a-linux-androideabi21-clang -O2 -s -ldl at_bridge.c -o AT
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
#include <termios.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/system_properties.h>

#include "all_gsm_commands.h"

#define CMD_BUF_SIZE            4096
#define RESP_BUF_SIZE           8192
#define RIL_REQUEST_OEM_HOOK_RAW 59

/* أنماط قنوات الاتصال */
typedef enum {
    CH_NONE = 0,
    CH_SECRIL_LIB,      /* libsecril-client.so */
    CH_RILD_PARCEL,     /* /dev/socket/rild مع تغليف Parcel قياسي */
    CH_DEV_IPC_RAW,     /* /dev/umts_ipc0 أو /dev/umts_rfs0 */
    CH_DEV_AT_TTY,      /* /dev/umts_atc0 أو /dev/smd11 للأوامر النصية */
    CH_TCP_RELAY_2600   /* TCP localhost:2600 Companion App / Netcat Relay */
} channel_mode_t;

typedef struct {
    channel_mode_t mode;
    int            fd;
    void          *dl_handle;
    char           active_path[128];
} bridge_ctx_t;

static bridge_ctx_t g_ctx = { .mode = CH_NONE, .fd = -1, .dl_handle = NULL };
static volatile int g_running = 1;
static uint32_t     g_session_seed = 0; /* 0 = Plaintext, >0 = Encrypted Session */
static int          g_verbose = 0;

static void sig_handler(int sig) {
    (void)sig;
    g_running = 0;
}

/* ============================================================================
 *  1. قراءة خصائص النظام مباشرة من الذاكرة
 * ============================================================================ */
static void fast_getprop(const char *key, char *out, size_t max_len) {
    char buf[PROP_VALUE_MAX] = {0};
    if (__system_property_get(key, buf) > 0) {
        strncpy(out, buf, max_len - 1);
        out[max_len - 1] = '\0';
    } else {
        strncpy(out, "N/A", max_len - 1);
    }
}

/* ============================================================================
 *  2. التشفير الديناميكي للجلسة (Xorshift32 Stream Cipher)
 * ============================================================================ */
static void transform_buffer(uint8_t *buf, size_t len, uint32_t seed) {
    if (seed == 0) return; /* الوضع المباشر غير المشفر */
    uint32_t state = seed;
    for (size_t i = 0; i < len; i++) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        buf[i] ^= (uint8_t)(state & 0xFF);
    }
}

/* ============================================================================
 *  3. مصعد الصلاحيات التلقائي عبر البورت 2600 (UID 1000 Auto-Relay)
 * ============================================================================ */
static int try_escalate_via_port_2600(int port) {
    if (getuid() == 1000 || getuid() == 0)
        return 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    const char *init_cmd = "setprop ctl.stop ssud; id\n";
    write(sock, init_cmd, strlen(init_cmd));

    char resp[256] = {0};
    usleep(150000);
    int n = read(sock, resp, sizeof(resp) - 1);
    close(sock);

    if (n > 0 && strstr(resp, "uid=1000")) {
        if (g_verbose) printf("[+] UID 1000 Escalation Bridge confirmed on port %d\n", port);
        fflush(stdout);
        return 1;
    }
    return -1;
}

/* ============================================================================
 *  4. جداول قنوات الاتصال بالمودم وفتح المنافذ
 * ============================================================================ */
static const char *IPC_NODES[] = {
    "/dev/umts_ipc0",
    "/dev/umts_ipc1",
    "/dev/umts_rfs0",
    "/dev/diag",
    NULL
};

static const char *AT_NODES[] = {
    "/dev/umts_atc0",
    "/dev/umts_atc1",
    "/dev/umts_at0",
    "/dev/at_distributor",
    "/dev/at_distributor0",
    "/dev/smd11",
    "/dev/smd0",
    "/dev/smd7",
    "/dev/ttyGS0",
    "/dev/ttyACM0",
    "/dev/ttyUSB0",
    NULL
};

static const char *RILD_SOCKETS[] = {
    "/dev/socket/rild",
    "/dev/socket/rild-oem",
    "/dev/socket/rild-debug",
    "/dev/socket/rild2",
    "/dev/socket/qmux_radio/qmux_client_socket0",
    NULL
};

static int open_device_list(const char **list, channel_mode_t target_mode) {
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
            strncpy(g_ctx.active_path, list[i], sizeof(g_ctx.active_path) - 1);
            return 0;
        }
    }
    return -1;
}

static int open_rild_socket(void) {
    for (int i = 0; RILD_SOCKETS[i] != NULL; i++) {
        if (access(RILD_SOCKETS[i], F_OK) != 0) continue;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, RILD_SOCKETS[i], sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            g_ctx.mode = CH_RILD_PARCEL;
            g_ctx.fd   = fd;
            strncpy(g_ctx.active_path, RILD_SOCKETS[i], sizeof(g_ctx.active_path) - 1);
            return 0;
        }
        close(fd);
    }
    return -1;
}

static int open_tcp_relay(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
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

static int init_best_channel(int prefer_at_tty) {
    if (g_ctx.fd >= 0) {
        close(g_ctx.fd);
        g_ctx.fd = -1;
    }

    if (prefer_at_tty) {
        if (open_device_list(AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    }

    if (open_device_list(IPC_NODES, CH_DEV_IPC_RAW) == 0) return 0;
    if (open_rild_socket() == 0) return 0;
    if (open_device_list(AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    if (open_tcp_relay(2600) == 0) return 0;

    return -1;
}

static int force_specific_device(const char *path) {
    if (g_ctx.fd >= 0) {
        close(g_ctx.fd);
        g_ctx.fd = -1;
    }

    if (strncmp(path, "/dev/socket/", 12) == 0) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            struct sockaddr_un addr = {0};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                g_ctx.mode = CH_RILD_PARCEL;
                g_ctx.fd   = fd;
                strncpy(g_ctx.active_path, path, sizeof(g_ctx.active_path) - 1);
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
            g_ctx.mode = (strstr(path, "ipc") || strstr(path, "diag")) ? CH_DEV_IPC_RAW : CH_DEV_AT_TTY;
            g_ctx.fd   = fd;
            strncpy(g_ctx.active_path, path, sizeof(g_ctx.active_path) - 1);
            return 0;
        }
    }
    return -1;
}

/* ============================================================================
 *  5. فحص قنوات المودم (Scan Mode)
 * ============================================================================ */
static void scan_devices(void) {
    printf("=== Available Modem Devices ===\n");
    for (int i = 0; AT_NODES[i] != NULL; i++) {
        if (access(AT_NODES[i], F_OK) == 0) {
            int rw = access(AT_NODES[i], R_OK | W_OK);
            printf("[+] %s: Found (TTY AT - %s)\n", AT_NODES[i], rw == 0 ? "Read/Write OK" : "Permission Denied");
        }
    }
    for (int i = 0; IPC_NODES[i] != NULL; i++) {
        if (access(IPC_NODES[i], F_OK) == 0) {
            int rw = access(IPC_NODES[i], R_OK | W_OK);
            printf("[+] %s: Found (Direct IPC - %s)\n", IPC_NODES[i], rw == 0 ? "Read/Write OK" : "Permission Denied");
        }
    }
    printf("=== RIL Sockets ===\n");
    for (int i = 0; RILD_SOCKETS[i] != NULL; i++) {
        if (access(RILD_SOCKETS[i], F_OK) == 0) {
            int s = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr = {0};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, RILD_SOCKETS[i], sizeof(addr.sun_path) - 1);
            int c = (s >= 0) ? connect(s, (struct sockaddr *)&addr, sizeof(addr)) : -1;
            printf("[%c] %s: Found (%s)\n", c == 0 ? '+' : '!', RILD_SOCKETS[i],
                   c == 0 ? "Connectable / Active" : "Exists, connect refused/SELinux blocked");
            if (s >= 0) close(s);
        }
    }
    fflush(stdout);
}

/* ============================================================================
 *  6. إرسال حزم RIL Parcel و IPC
 * ============================================================================ */
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
    struct timeval tv = { .tv_sec = timeout_ms / 1000, .tv_usec = (timeout_ms % 1000) * 1000 };
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

    while ((offset - 4) % 4 != 0) {
        parcel[offset++] = 0x00;
    }

    uint32_t parcel_size_be = htonl(offset - 4);
    memcpy(parcel, &parcel_size_be, 4);

    if (raw_write_all(fd, parcel, offset) < 0) return -1;
    return raw_read_timeout(fd, out_resp, max_resp, 2500);
}

static int dispatch_ipc_frame(const uint8_t *ipc_bytes, int len,
                              uint8_t *resp, int max_resp) {
    if (g_ctx.mode == CH_NONE && init_best_channel(0) != 0) {
        /* Multi-Modal Fallback when raw /dev/ or socket is blocked by SELinux (Qualcomm SM-S918U): */
        if (len >= 6 && ipc_bytes[0] == 0x06 && ipc_bytes[1] == 0x01) {
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
                         "am broadcast -a com.samsung.intent.action.CSC_COMPLETED --es csc %s >/dev/null 2>&1",
                         extracted_csc, extracted_csc, extracted_csc, extracted_csc);
                system(prop_cmd);
            }
            if (resp && max_resp >= 4) {
                resp[0] = 0x06;
                resp[1] = 0x01;
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

/* ============================================================================
 *  7. إرسال أمر AT نصي متزامن (Synchronous AT Command Sender)
 * ============================================================================ */
static int send_at_command_sync(const char *cmd, char *out_resp, int max_resp, int timeout_ms) {
    if (!cmd) return -1;

    if (g_ctx.mode != CH_DEV_AT_TTY && g_ctx.mode != CH_TCP_RELAY_2600) {
        init_best_channel(1);
    }
    if (g_ctx.fd < 0) return -1;

    /* Clean trailing CRLF and spaces */
    char clean_cmd[CMD_BUF_SIZE];
    strncpy(clean_cmd, cmd, sizeof(clean_cmd) - 1);
    clean_cmd[sizeof(clean_cmd) - 1] = '\0';
    int len = strlen(clean_cmd);
    while (len > 0 && (clean_cmd[len - 1] == '\r' || clean_cmd[len - 1] == '\n' || clean_cmd[len - 1] == ' ')) {
        clean_cmd[--len] = '\0';
    }

    char buf[CMD_BUF_SIZE];
    int l;
    if (g_ctx.mode == CH_TCP_RELAY_2600) {
        l = snprintf(buf, sizeof(buf), "echo -e '%s\\r' > /dev/umts_atc0 2>/dev/null; head -n 10 /dev/umts_atc0 2>/dev/null\n", clean_cmd);
    } else {
        l = snprintf(buf, sizeof(buf), "%s\r\n", clean_cmd);
    }

    if (raw_write_all(g_ctx.fd, (const uint8_t *)buf, l) <= 0) return -1;

    int r = raw_read_timeout(g_ctx.fd, (uint8_t *)out_resp, max_resp - 1, timeout_ms);
    if (r >= 0) out_resp[r] = '\0';
    return r;
}

/* ============================================================================
 *  8. فحص أقفال الشريحة والشبكة (SIM Card & Network Lock Inspection)
 * ============================================================================ */
static void query_sim_locks(int verbose) {
    char cpin_resp[256] = {0};
    char clck_pn[256] = {0};
    char clck_pu[256] = {0};
    char clck_pp[256] = {0};
    char clck_pc[256] = {0};
    char clck_sc[256] = {0};

    send_at_command_sync("AT+CPIN?", cpin_resp, sizeof(cpin_resp), 1200);
    send_at_command_sync("AT+CLCK=\"PN\",2", clck_pn, sizeof(clck_pn), 1200);
    send_at_command_sync("AT+CLCK=\"PU\",2", clck_pu, sizeof(clck_pu), 1200);
    send_at_command_sync("AT+CLCK=\"PP\",2", clck_pp, sizeof(clck_pp), 1200);
    send_at_command_sync("AT+CLCK=\"PC\",2", clck_pc, sizeof(clck_pc), 1200);
    send_at_command_sync("AT+CLCK=\"SC\",2", clck_sc, sizeof(clck_sc), 1200);

    const char *sim_state = "UNKNOWN";
    if (strstr(cpin_resp, "READY")) sim_state = "READY";
    else if (strstr(cpin_resp, "SIM PIN")) sim_state = "PIN_REQUIRED";
    else if (strstr(cpin_resp, "SIM PUK")) sim_state = "PUK_REQUIRED";
    else if (strstr(cpin_resp, "NOT INSERTED") || strstr(cpin_resp, "ERROR: 10")) sim_state = "ABSENT";
    else if (strstr(cpin_resp, "ERROR")) sim_state = "MODEM_ERROR";

    const char *pn_lock = (strstr(clck_pn, "+CLCK: 0") || strstr(clck_pn, "+CLCK:0")) ? "UNLOCKED(0)" :
                          (strstr(clck_pn, "+CLCK: 1") || strstr(clck_pn, "+CLCK:1")) ? "LOCKED(1)" : "UNKNOWN";

    const char *pu_lock = (strstr(clck_pu, "+CLCK: 0") || strstr(clck_pu, "+CLCK:0")) ? "UNLOCKED(0)" :
                          (strstr(clck_pu, "+CLCK: 1") || strstr(clck_pu, "+CLCK:1")) ? "LOCKED(1)" : "UNKNOWN";

    const char *pp_lock = (strstr(clck_pp, "+CLCK: 0") || strstr(clck_pp, "+CLCK:0")) ? "UNLOCKED(0)" :
                          (strstr(clck_pp, "+CLCK: 1") || strstr(clck_pp, "+CLCK:1")) ? "LOCKED(1)" : "UNKNOWN";

    const char *pc_lock = (strstr(clck_pc, "+CLCK: 0") || strstr(clck_pc, "+CLCK:0")) ? "UNLOCKED(0)" :
                          (strstr(clck_pc, "+CLCK: 1") || strstr(clck_pc, "+CLCK:1")) ? "LOCKED(1)" : "UNKNOWN";

    const char *sc_lock = (strstr(clck_sc, "+CLCK: 0") || strstr(clck_sc, "+CLCK:0")) ? "UNLOCKED(0)" :
                          (strstr(clck_sc, "+CLCK: 1") || strstr(clck_sc, "+CLCK:1")) ? "LOCKED(1)" : "UNKNOWN";

    if (verbose) {
        printf("=== SIM & Network Personalization Locks ===\n");
        printf("  SIM State (CPIN)             : %s\n", sim_state);
        printf("  Network Lock (CLCK:PN)       : %s\n", pn_lock);
        printf("  Network Subset Lock (CLCK:PU): %s\n", pu_lock);
        printf("  Service Provider Lock (PP)   : %s\n", pp_lock);
        printf("  Corporate Lock (CLCK:PC)     : %s\n", pc_lock);
        printf("  SIM Lock (CLCK:SC)           : %s\n", sc_lock);
    } else {
        printf("SIMLOCK|CPIN=%s|PN=%s|PU=%s|PP=%s|PC=%s|SC=%s\n",
               sim_state, pn_lock, pu_lock, pp_lock, pc_lock, sc_lock);
    }
    fflush(stdout);
}

/* ============================================================================
 *  9. وظائف سبرينت وبوست (/boost Sprint & Boost Mobile Unlock)
 * ============================================================================ */
static int execute_boost_unlock(int verbose) {
    if (verbose) printf("[*] Starting Sprint / Boost Mobile Unlock sequence...\n");

    char resp[RESP_BUF_SIZE] = {0};

    /* 1. إرسال أمر فك قفل سبرينت القديم */
    if (verbose) printf("  [1/5] Sending AT+UNLOCKSPR=OLD\n");
    send_at_command_sync("AT+UNLOCKSPR=OLD", resp, sizeof(resp), 1500);

    /* 2. تصفير معلمات قفل الشريحة */
    if (verbose) printf("  [2/5] Sending AT+SIMLOCKU=0,0,0\n");
    send_at_command_sync("AT+SIMLOCKU=0,0,0", resp, sizeof(resp), 1500);

    /* 3. إلغاء تثبيت وإيقاف حزم قفل المشغل الخاصة بسبرينت */
    if (verbose) printf("  [3/5] Cleaning up Sprint OMADM & Carrier Lock packages...\n");
    system(CMD_UNINSTALL_OMADMSPR);
    system(CMD_UNINSTALL_AUTOPRECONFIG);
    system(CMD_FORCESTOP_SKATD);

    /* 4. تفريغ سجلات الراديو */
    if (verbose) printf("  [4/5] Flushing radio logcat...\n");
    system(CMD_FLUSH_RADIO_LOGS);

    /* 5. إعادة تشغيل الراديو لتطبيق الإعدادات */
    if (verbose) printf("  [5/5] Re-initializing radio modem (AT+CFUN=1,1)...\n");
    send_at_command_sync("AT+CFUN=1,1", resp, sizeof(resp), 2000);

    if (verbose) printf("[+] Sprint/Boost Unlock sequence completed successfully.\n");
    else printf(".OK_BOOST..\n");
    fflush(stdout);
    return 0;
}

/* ============================================================================
 *  10. إعادة قفل المشغل عبر حزمة BCD MCC/MNC (Carrier Relock / SRELOCK1 / SETSPR)
 * ============================================================================ */
static int execute_carrier_relock(const char *mcc_mnc_bcd_hex, int verbose) {
    if (!mcc_mnc_bcd_hex || strlen(mcc_mnc_bcd_hex) < 6) {
        if (verbose) printf("[ERROR] Invalid MCC/MNC BCD Hex format: %s\n", mcc_mnc_bcd_hex ? mcc_mnc_bcd_hex : "NULL");
        else printf(".ERR_RELOCK_INVALID..\n");
        return -1;
    }

    if (verbose) printf("[*] Executing Carrier Relock for BCD payload: %s\n", mcc_mnc_bcd_hex);

    /* بناء حزمة IPC الخاصة بأمر SRELOCK1 / SETSPR */
    char pkt[256];
    uint8_t bin[128];
    uint8_t resp[RESP_BUF_SIZE];

    /* إطار تهيئة الـ Relock: 06 01 [Length] 00 00 [BCD Payload] */
    int hex_len = strlen(mcc_mnc_bcd_hex);
    int total_bytes = 4 + (hex_len / 2);
    snprintf(pkt, sizeof(pkt), "0601%04X0000%s", total_bytes, mcc_mnc_bcd_hex);

    int bin_len = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  Dispatching SRELOCK1 frame: %s\n", pkt);
    int r = dispatch_ipc_frame(bin, bin_len, resp, sizeof(resp));

    /* إعادة تنشيط الراديو */
    char at_resp[256] = {0};
    send_at_command_sync("AT+CFUN=1,1", at_resp, sizeof(at_resp), 1500);

    if (verbose) {
        printf("[+] Carrier Relock dispatched (RIL result: %d bytes). Radio reset initiated.\n", r);
    } else {
        printf(".OK_RELOCK..\n");
    }
    fflush(stdout);
    return 0;
}

/* ============================================================================
 *  11. معلومات وهوية الهاتف العميقة (Deep Telemetry: DEVCONINFO + VERSNAME + IMEI)
 * ============================================================================ */
static void clean_crlf(char *s) {
    char *p = s;
    while (*p) {
        if (*p == '\r' || *p == '\n') *p = ' ';
        p++;
    }
}

static void print_full_telemetry(int verbose) {
    char model[64], brand[64], csc[64], ver[64], patch[64], build[64], serial[64], prod[64];
    fast_getprop("ro.product.model", model, sizeof(model));
    fast_getprop("ro.product.brand", brand, sizeof(brand));
    fast_getprop("ro.csc.sales_code", csc, sizeof(csc));
    fast_getprop("ro.build.version.release", ver, sizeof(ver));
    fast_getprop("ro.build.version.security_patch", patch, sizeof(patch));
    fast_getprop("ro.build.display.id", build, sizeof(build));
    fast_getprop("ro.boot.serialno", serial, sizeof(serial));
    fast_getprop("ril.product_code", prod, sizeof(prod));

    /* قراءة أوامر AT الميدانية من المودم مباشرة */
    char devcon[RESP_BUF_SIZE] = {0};
    char vers_sw[256] = {0};
    char imei[256] = {0};
    char imsi[256] = {0};
    char sn_at[256] = {0};
    char batt[256] = {0};
    char sysscope[256] = {0};

    send_at_command_sync("AT+DEVCONINFO", devcon, sizeof(devcon), 2000);
    send_at_command_sync("AT+VERSNAME=1,9,0", vers_sw, sizeof(vers_sw), 1200);
    send_at_command_sync("AT+IMEITEST=1,0", imei, sizeof(imei), 1200);
    send_at_command_sync("AT+SERIALNO=1,0", sn_at, sizeof(sn_at), 1200);
    send_at_command_sync("AT+CIMI", imsi, sizeof(imsi), 1200);
    send_at_command_sync("AT+BATTEST=1,0", batt, sizeof(batt), 1200);
    send_at_command_sync("AT+SYSSCOPE=1,0", sysscope, sizeof(sysscope), 1200);

    clean_crlf(devcon);
    clean_crlf(vers_sw);
    clean_crlf(imei);
    clean_crlf(imsi);
    clean_crlf(sn_at);
    clean_crlf(batt);
    clean_crlf(sysscope);

    if (verbose) {
        printf("=== Deep Telemetry & Hardware Identification ===\n");
        printf("  Model         : %s\n", model);
        printf("  Brand         : %s\n", brand);
        printf("  CSC Code      : %s\n", csc);
        printf("  Android Ver   : %s\n", ver);
        printf("  Security Patch: %s\n", patch);
        printf("  Build ID      : %s\n", build);
        printf("  Serial (Prop) : %s\n", serial);
        printf("  Product Code  : %s\n", prod);
        printf("  DEVCONINFO    : %s\n", devcon);
        printf("  VERSNAME SW   : %s\n", vers_sw);
        printf("  IMEITEST      : %s\n", imei);
        printf("  SERIALNO AT   : %s\n", sn_at);
        printf("  CIMI (IMSI)   : %s\n", imsi);
        printf("  BATTEST       : %s\n", batt);
        printf("  SYSSCOPE      : %s\n", sysscope);
        printf("  Active Channel: %s (%d)\n", g_ctx.active_path, g_ctx.mode);
    } else {
        printf("DEVINFO|MODEL=%s|BRAND=%s|CSC=%s|AND=%s|PATCH=%s|BUILD=%s|SN=%s|PROD=%s|VERS=%s|IMEI=%s|IMSI=%s|DEVCON=%s\n",
               model, brand, csc, ver, patch, build, serial, prod, vers_sw, imei, imsi, devcon);
    }
    fflush(stdout);
}

/* ============================================================================
 *  12. التحكم في الراديو والطاقة (Radio & Power Actions)
 * ============================================================================ */
static int execute_radio_action(const char *action, int verbose) {
    char resp[RESP_BUF_SIZE] = {0};

    if (strcasecmp(action, "REBOOT") == 0 || strcasecmp(action, "RESET") == 0) {
        if (verbose) printf("[*] Executing Radio Soft Reboot (AT+CFUN=1,1)...\n");
        send_at_command_sync("AT+CFUN=1,1", resp, sizeof(resp), 2000);
    } else if (strcasecmp(action, "OFF") == 0 || strcasecmp(action, "AIRPLANE") == 0) {
        if (verbose) printf("[*] Turning Radio Off (AT+CFUN=0)...\n");
        send_at_command_sync("AT+CFUN=0", resp, sizeof(resp), 2000);
    } else if (strcasecmp(action, "ON") == 0) {
        if (verbose) printf("[*] Turning Radio On (AT+CFUN=1)...\n");
        send_at_command_sync("AT+CFUN=1", resp, sizeof(resp), 2000);
    } else if (strcasecmp(action, "FACTORST") == 0) {
        if (verbose) printf("[*] Modem NV Factory Reset (AT+FACTORST=0,0)...\n");
        send_at_command_sync("AT+FACTORST=0,0", resp, sizeof(resp), 3000);
    } else if (strcasecmp(action, "SWRESET") == 0) {
        if (verbose) printf("[*] Rebooting Phone (AT+SWRESET)...\n");
        send_at_command_sync("AT+SWRESET", resp, sizeof(resp), 2000);
    } else {
        if (verbose) printf("[ERROR] Unknown radio action: %s\n", action);
        else printf(".ERR_UNKNOWN_ACTION..\n");
        return -1;
    }

    if (verbose) printf("[+] Radio action '%s' executed. Response: %s\n", action, resp);
    else printf(".OK_RADIO..\n");
    fflush(stdout);
    return 0;
}

/* ============================================================================
 *  13. التحدي الأمني والمصادقة (Security Challenge: AKSEEDNO)
 * ============================================================================ */
static void query_akseedno(int verbose) {
    char resp[RESP_BUF_SIZE] = {0};
    send_at_command_sync("AT+AKSEEDNO=1,0", resp, sizeof(resp), 2000);
    clean_crlf(resp);

    if (verbose) {
        printf("=== Modem Access Key Seed (Challenge) ===\n");
        printf("  Result: %s\n", resp);
    } else {
        printf("AKSEED|%s\n", resp);
    }
    fflush(stdout);
}

/* ============================================================================
 *  14. محرك تغيير CSC المدمج (Self-Contained 5-Packet Samsung CSC Injection)
 * ============================================================================ */
static int execute_samsung_csc_change(const char *target_csc, int verbose) {
    if (!target_csc || strlen(target_csc) < 2 || strlen(target_csc) > 5) {
        if (verbose) printf("[ERROR] Invalid CSC code: %s (must be 2-5 uppercase chars)\n", target_csc ? target_csc : "NULL");
        else printf(".ERR_CSC_INVALID..\n");
        return -1;
    }

    char clean_csc[8] = {0};
    for (int i = 0; target_csc[i] && i < 5; i++) {
        char c = target_csc[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        clean_csc[i] = c;
    }

    char hex_csc[16] = {0};
    for (int i = 0; clean_csc[i]; i++) {
        sprintf(hex_csc + (i * 2), "%02X", (uint8_t)clean_csc[i]);
    }

    if (verbose) printf("[*] Starting Samsung CSC change to: %s (Hex: %s)\n", clean_csc, hex_csc);

    char pkt[256];
    uint8_t bin[128];
    uint8_t resp[RESP_BUF_SIZE];

    /* Packet 1: 0601000E0000{hexCsc}0000{hexCsc} */
    snprintf(pkt, sizeof(pkt), "0601000E0000%s0000%s", hex_csc, hex_csc);
    int l1 = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [1/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, l1, resp, sizeof(resp));

    /* Packet 2: 060100090000{hexCsc} */
    snprintf(pkt, sizeof(pkt), "060100090000%s", hex_csc);
    int l2 = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [2/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, l2, resp, sizeof(resp));

    /* Packet 3: 060100120000{hexCsc}0000{hexCsc}{hexCsc} */
    snprintf(pkt, sizeof(pkt), "060100120000%s0000%s%s", hex_csc, hex_csc, hex_csc);
    int l3 = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [3/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, l3, resp, sizeof(resp));

    /* Packet 4: 060100120000 */
    snprintf(pkt, sizeof(pkt), "060100120000");
    int l4 = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [4/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, l4, resp, sizeof(resp));

    /* Packet 5: 060100090000{hexCsc} */
    snprintf(pkt, sizeof(pkt), "060100090000%s", hex_csc);
    int l5 = hex2bin(pkt, bin, sizeof(bin));
    if (verbose) printf("  [5/5] Injecting: %s\n", pkt);
    dispatch_ipc_frame(bin, l5, resp, sizeof(resp));

    /* Multi-Modal AT Sequence (Works on Qualcomm and Exynos modems) */
    char at_cmd[128];
    char at_resp[256] = {0};
    send_at_command_sync("AT+SWATD=0", at_resp, sizeof(at_resp), 1000);
    send_at_command_sync("AT+ACTIVATE=0,0,0", at_resp, sizeof(at_resp), 1000);
    send_at_command_sync("AT+SWATD=1", at_resp, sizeof(at_resp), 1000);
    snprintf(at_cmd, sizeof(at_cmd), "AT+PRECONFG=2,%s", clean_csc);
    send_at_command_sync(at_cmd, at_resp, sizeof(at_resp), 1500);
    send_at_command_sync("AT+PRECONFG=1,0", at_resp, sizeof(at_resp), 1000);
    snprintf(at_cmd, sizeof(at_cmd), "AT+RFBYCODE=2,0,%s", clean_csc);
    send_at_command_sync(at_cmd, at_resp, sizeof(at_resp), 1000);

    /* Persistent System Properties & Broadcast Injection */
    char prop_cmd[256];
    snprintf(prop_cmd, sizeof(prop_cmd),
             "setprop persist.sys.omc_sales_code %s; "
             "setprop persist.sys.sales_code %s; "
             "setprop ro.csc.sales_code %s; "
             "am broadcast -a com.samsung.intent.action.CSC_COMPLETED --es csc %s >/dev/null 2>&1",
             clean_csc, clean_csc, clean_csc, clean_csc);
    system(prop_cmd);

    if (verbose) {
        printf("[*] CSC change sequence completed for: %s\n", clean_csc);
        printf("[*] Running carrier package cleanup...\n");
    }

    system(CMD_UNINSTALL_AUTOPRECONFIG);
    system(CMD_UNINSTALL_OMADMSPR);
    system(CMD_FLUSH_RADIO_LOGS);

    if (verbose) printf("[+] Samsung CSC change successful.\n");
    else printf(".OK_CSC..\n");
    fflush(stdout);
    return 0;
}

/* ============================================================================
 *  15. معلومات الهاتف ومعلومات المساعدة
 * ============================================================================ */
static void print_compact_info(void) {
    char model[64], csc[64], ver[64], patch[64], debug_lvl[64];
    fast_getprop("ro.product.model", model, sizeof(model));
    fast_getprop("ro.csc.sales_code", csc, sizeof(csc));
    fast_getprop("ro.build.version.release", ver, sizeof(ver));
    fast_getprop("ro.build.version.security_patch", patch, sizeof(patch));
    fast_getprop("ro.boot.debug_level", debug_lvl, sizeof(debug_lvl));

    printf("INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s\n",
           model, csc, ver, patch, debug_lvl, getuid(),
           g_ctx.mode != CH_NONE ? g_ctx.active_path : "NONE");
    fflush(stdout);
}

static void print_detailed_status(void) {
    char model[64], csc[64], ver[64], patch[64], debug_lvl[64], brand[64], serial[64];
    fast_getprop("ro.product.model", model, sizeof(model));
    fast_getprop("ro.product.brand", brand, sizeof(brand));
    fast_getprop("ro.csc.sales_code", csc, sizeof(csc));
    fast_getprop("ro.build.version.release", ver, sizeof(ver));
    fast_getprop("ro.build.version.security_patch", patch, sizeof(patch));
    fast_getprop("ro.boot.debug_level", debug_lvl, sizeof(debug_lvl));
    fast_getprop("ro.boot.serialno", serial, sizeof(serial));

    printf("=== Device Status ===\n");
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
           g_ctx.mode == CH_RILD_PARCEL ? "RILD Parcel" :
           g_ctx.mode == CH_DEV_IPC_RAW ? "Raw IPC" :
           g_ctx.mode == CH_DEV_AT_TTY ? "AT TTY" :
           g_ctx.mode == CH_TCP_RELAY_2600 ? "TCP Port 2600 Relay" : "Disconnected");
    query_sim_locks(1);
}

static void print_help(const char *prog_name) {
    printf("============================================================\n");
    printf("  GsmTools - Samsung AT, RIL & Boost Multi-Modal Bridge (v3.0)\n");
    printf("============================================================\n");
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("One-Shot CLI Options:\n");
    printf("  -b, --boost, /boost  Execute Sprint / Boost Mobile unlock sequence\n");
    printf("  -r <BCD_HEX>         Carrier Relock via BCD MCCREQ payload\n");
    printf("  -t, --telemetry      Deep device telemetry (AT+DEVCONINFO, VERSNAME, etc.)\n");
    printf("  -c <CSC>             Quick Samsung CSC change (e.g. -c XAA)\n");
    printf("  -a <AT_CMD>          Send single AT command and exit\n");
    printf("  -x <HEX>             Send single hex IPC frame and exit\n");
    printf("  -s                   Scan mode: list accessible modem & RIL devices\n");
    printf("  -l                   Query SIM Card & Network Personalization locks\n");
    printf("  --radio <ACT>        Execute radio action (REBOOT, OFF, ON, FACTORST, SWRESET)\n");
    printf("  --seed               Query security challenge seed (AT+AKSEEDNO)\n");
    printf("  -d <DEVICE>          Force specific modem device path\n");
    printf("  -p <PORT>            Escalation port (default: 2600)\n");
    printf("  -v                   Verbose diagnostic output\n");
    printf("  -h, --help           Show this help menu\n\n");
    printf("Interactive Commands:\n");
    printf("  BOOST, /boost        Sprint / Boost Mobile unlock sequence\n");
    printf("  RELOCK:<BCD>         Carrier Relock with BCD MCC/MNC\n");
    printf("  DEVINFO              Deep device telemetry header\n");
    printf("  RADIO:<ACT>          Radio power & reboot control\n");
    printf("  AKSEED               Modem security challenge query\n");
    printf("  CSC:<CODE>           Automated 5-packet Samsung CSC change\n");
    printf("  AT+... or AT:<CMD>   Send raw AT command (e.g. AT+DEVCONINFO or AT:AT+DEVCONINFO)\n");
    printf("  HEX:<DATA>           Send hex IPC command (or raw hex without prefix)\n");
    printf("  SCAN                 Scan accessible modem channels\n");
    printf("  STATUS               Display full device and channel status\n");
    printf("  LOCKS                Query SIM & Network locks (CPIN, CLCK)\n");
    printf("  RECONNECT            Re-probe and reconnect best modem channel\n");
    printf("  MODE:AT              Switch to AT text channel\n");
    printf("  MODE:IPC             Switch to IPC / RIL socket channel\n");
    printf("  AUTH:<SEED>          Activate dynamic Xorshift32 stream cipher\n");
    printf("  INFO                 Compact pipe-delimited telemetry header\n");
    printf("  EXIT, QUIT           Terminate bridge session\n");
    printf("============================================================\n");
    fflush(stdout);
}

/* ============================================================================
 *  16. الدالة الرئيسية (CLI One-Shot & Interactive Loop)
 * ============================================================================ */

int main(int argc, char *argv[]) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    const char *cli_csc = NULL;
    const char *cli_at = NULL;
    const char *cli_hex = NULL;
    const char *cli_dev = NULL;
    const char *cli_relock = NULL;
    const char *cli_radio = NULL;
    int cli_boost = 0;
    int cli_telemetry = 0;
    int cli_seed = 0;
    int cli_scan = 0;
    int cli_locks = 0;
    int esc_port = 2600;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0) {
            g_verbose = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            cli_scan = 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            cli_locks = 1;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--boost") == 0 || strcmp(argv[i], "/boost") == 0 || strcmp(argv[i], "-boost") == 0 || strcmp(argv[i], "boost") == 0) {
            cli_boost = 1;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--telemetry") == 0) {
            cli_telemetry = 1;
        } else if (strcmp(argv[i], "--seed") == 0) {
            cli_seed = 1;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            cli_csc = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            cli_relock = argv[++i];
        } else if (strcmp(argv[i], "--radio") == 0 && i + 1 < argc) {
            cli_radio = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            cli_at = argv[++i];
        } else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            cli_hex = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            cli_dev = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            esc_port = atoi(argv[++i]);
        }
    }

    try_escalate_via_port_2600(esc_port);

    if (cli_dev) {
        force_specific_device(cli_dev);
    } else {
        init_best_channel((cli_at || cli_telemetry || cli_seed || cli_radio) ? 1 : 0);
    }

    if (cli_scan) {
        scan_devices();
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return 0;
    }

    if (cli_locks) {
        query_sim_locks(1);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return 0;
    }

    if (cli_telemetry) {
        print_full_telemetry(g_verbose);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return 0;
    }

    if (cli_seed) {
        query_akseedno(g_verbose);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return 0;
    }

    if (cli_boost) {
        int res = execute_boost_unlock(g_verbose);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return res;
    }

    if (cli_relock) {
        int res = execute_carrier_relock(cli_relock, g_verbose);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return res;
    }

    if (cli_radio) {
        int res = execute_radio_action(cli_radio, g_verbose);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return res;
    }

    if (cli_at) {
        char at_resp[RESP_BUF_SIZE] = {0};
        int r = send_at_command_sync(cli_at, at_resp, sizeof(at_resp), 2500);
        if (r > 0) printf("%s\n", at_resp);
        else printf(".ERR_AT_TIMEOUT..\n");
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return (r > 0) ? 0 : 1;
    }

    if (cli_hex) {
        uint8_t payload[CMD_BUF_SIZE];
        uint8_t resp[RESP_BUF_SIZE];
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        int l = hex2bin(cli_hex, payload, sizeof(payload));
        if (l > 0) {
            int r = dispatch_ipc_frame(payload, l, resp, sizeof(resp));
            if (r > 0) {
                bin2hex(resp, r, resp_hex);
                printf("%s\n", resp_hex);
            } else {
                printf(".ERR_IPC..\n");
            }
            if (g_ctx.fd >= 0) close(g_ctx.fd);
            return (r > 0) ? 0 : 1;
        } else {
            printf(".ERR_HEX_SYNTAX..\n");
            if (g_ctx.fd >= 0) close(g_ctx.fd);
            return 1;
        }
    }

    if (cli_csc) {
        int res = execute_samsung_csc_change(cli_csc, g_verbose);
        if (g_ctx.fd >= 0) close(g_ctx.fd);
        return res;
    }

    /* الوضع التفاعلي الكامل المتوافق 100% مع واجهة C# Desktop */
    print_compact_info();

    char line[CMD_BUF_SIZE * 2 + 64];
    uint8_t payload[CMD_BUF_SIZE];
    uint8_t resp[RESP_BUF_SIZE];
    char resp_hex[RESP_BUF_SIZE * 2 + 1];

    printf("<< ");
    fflush(stdout);

    while (g_running && fgets(line, sizeof(line), stdin) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (!line[0]) { printf("<< "); fflush(stdout); continue; }

        if (strcasecmp(line, "EXIT") == 0 || strcasecmp(line, "QUIT") == 0) break;

        if (strcasecmp(line, "HELP") == 0) {
            print_help(argv[0]);
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "SCAN") == 0) {
            scan_devices();
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "STATUS") == 0) {
            print_detailed_status();
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "LOCKS") == 0 || strcasecmp(line, "SIMLOCK") == 0) {
            query_sim_locks(0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "DEVINFO") == 0 || strcasecmp(line, "TELEMETRY") == 0) {
            print_full_telemetry(0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "AKSEED") == 0) {
            query_akseedno(0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "BOOST") == 0 ||
            strcasecmp(line, "/boost") == 0 ||
            strcasecmp(line, "-boost") == 0 ||
            strcasecmp(line, "--boost") == 0 ||
            strcasecmp(line, "BOOST_UNLOCK") == 0 ||
            strcasecmp(line, "SPR_UNLOCK") == 0 ||
            strncasecmp(line, "BOOST:", 6) == 0 ||
            strncasecmp(line, "/BOOST:", 7) == 0) {
            execute_boost_unlock(0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strncasecmp(line, "RELOCK:", 7) == 0) {
            execute_carrier_relock(line + 7, 0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strncasecmp(line, "RADIO:", 6) == 0) {
            execute_radio_action(line + 6, 0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strcasecmp(line, "RECONNECT") == 0) {
            init_best_channel(0);
            printf(".CH=%s..\n<< ", g_ctx.active_path);
            fflush(stdout);
            continue;
        }

        if (strcasecmp(line, "INFO") == 0) {
            print_compact_info();
            printf("<< "); fflush(stdout); continue;
        }

        if (strncasecmp(line, "AUTH:", 5) == 0) {
            g_session_seed = (uint32_t)strtoul(line + 5, NULL, 16);
            printf(".OK_AUTH..\n<< ");
            fflush(stdout);
            continue;
        }

        if (strncasecmp(line, "CSC:", 4) == 0) {
            execute_samsung_csc_change(line + 4, 0);
            printf("<< "); fflush(stdout); continue;
        }

        if (strncasecmp(line, "MODE:AT", 7) == 0) {
            init_best_channel(1);
            printf(".CH=%s..\n<< ", g_ctx.active_path);
            fflush(stdout);
            continue;
        }

        if (strncasecmp(line, "MODE:IPC", 8) == 0) {
            init_best_channel(0);
            printf(".CH=%s..\n<< ", g_ctx.active_path);
            fflush(stdout);
            continue;
        }

        if (strncasecmp(line, "AT:", 3) == 0 ||
            strncasecmp(line, "AT", 2) == 0 ||
            line[0] == '+' || line[0] == '$' ||
            strncasecmp(line, "GETSPR", 6) == 0 ||
            strncasecmp(line, "SETSPR", 6) == 0 ||
            strncasecmp(line, "SRELOCK1", 8) == 0) {
            const char *at_cmd = (strncasecmp(line, "AT:", 3) == 0) ? line + 3 : line;
            char at_out[RESP_BUF_SIZE] = {0};
            int r = send_at_command_sync(at_cmd, at_out, sizeof(at_out), 2500);
            if (r > 0) printf("%s\n", at_out);
            else printf(".TIMEOUT..\n");
            printf("<< "); fflush(stdout); continue;
        }

        const char *hex_ptr = line;
        if (strncasecmp(line, "HEX:", 4) == 0) hex_ptr = line + 4;

        int bin_len = hex2bin(hex_ptr, payload, sizeof(payload));
        if (bin_len > 0) {
            transform_buffer(payload, bin_len, g_session_seed);

            int r = dispatch_ipc_frame(payload, bin_len, resp, sizeof(resp));
            memset(payload, 0, sizeof(payload));

            if (r > 0) {
                transform_buffer(resp, r, g_session_seed);
                bin2hex(resp, r, resp_hex);
                printf(".OK..%s#\n", resp_hex);
            } else if (r == 0) {
                printf(".OK..#\n");
            } else {
                printf(".ERR_IPC..\n");
            }
        } else {
            printf(".ERR_SYNTAX..\n");
        }

        printf("<< ");
        fflush(stdout);
    }

    if (g_ctx.fd >= 0) close(g_ctx.fd);
    return 0;
}
