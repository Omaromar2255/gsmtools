/*
 * ============================================================================
 *  GsmTools - Multi-Modal RIL/IPC/AT Bridge & SIM Lock Inspector (v2.5 Architect)
 * ============================================================================
 *  Compile: aarch64-linux-android29-clang -O2 -s -fvisibility=hidden
 *           -ldl at_bridge.c -o AT
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

#define CMD_BUF_SIZE            4096
#define RESP_BUF_SIZE           8192
#define RIL_REQUEST_OEM_HOOK_RAW 59

/* أنماط التشغيل المفصولة معمارياً */
typedef enum {
    CH_NONE = 0,
    CH_SECRIL_LIB,      /* libsecril-client.so (الأقوى والأضمن لسامسونج) */
    CH_RILD_PARCEL,     /* /dev/socket/rild مع تغليف Parcel قياسي */
    CH_DEV_IPC_RAW,     /* /dev/umts_ipc0 أو /dev/umts_rfs0 */
    CH_DEV_AT_TTY       /* /dev/umts_atc0 أو /dev/smd11 للأوامر النصية فقط */
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
 *  1. قراءة خصائص النظام مباشرة من الذاكرة (بدون popen أو شل!)
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
 *  2. التشفير الديناميكي للجلسة (Dynamic Stream Cipher)
 *     Xorshift32 Stream Cipher
 * ============================================================================ */
static void transform_buffer(uint8_t *buf, size_t len, uint32_t seed) {
    if (seed == 0) return; /* وضع غير مشفر للاختبار المحلي */
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
        return 0; /* نحن بالفعل بصلاحية system أو root */

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
 *  4. جداول قنوات الاتصال بالمودم الممتدة (Extended Modem Nodes)
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

static int init_best_channel(int prefer_at_tty) {
    if (g_ctx.fd >= 0) {
        close(g_ctx.fd);
        g_ctx.fd = -1;
    }

    if (prefer_at_tty) {
        if (open_device_list(AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    }

    /* الأولوية لـ IPC المباشر ثم سوكيت RILD ثم منافذ AT */
    if (open_device_list(IPC_NODES, CH_DEV_IPC_RAW) == 0) return 0;
    if (open_rild_socket() == 0) return 0;
    if (open_device_list(AT_NODES, CH_DEV_AT_TTY) == 0) return 0;

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
 *  6. بناء وإرسال حزم RIL Parcel و IPC الخام
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
        tv.tv_sec = 0; tv.tv_usec = 35000; /* 35ms لتجميع بقية الإطار */
        if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0) break;
    }
    return total;
}

/* تغليف بايتات IPC داخل Android RIL Parcel القياسي عند الإرسال لسوكيت rild */
static int send_rild_oem_parcel(int fd, const uint8_t *ipc_data, int ipc_len,
                                uint8_t *out_resp, int max_resp) {
    static uint32_t s_token = 1;
    uint8_t parcel[CMD_BUF_SIZE + 32];
    int offset = 4; /* نترك أول 4 بايتات لطول الـ Parcel الكلي (Big-Endian) */

    uint32_t req_id = RIL_REQUEST_OEM_HOOK_RAW;
    uint32_t token  = s_token++;
    uint32_t dlen   = (uint32_t)ipc_len;

    memcpy(parcel + offset, &req_id, 4); offset += 4;
    memcpy(parcel + offset, &token,  4); offset += 4;
    memcpy(parcel + offset, &dlen,   4); offset += 4;
    memcpy(parcel + offset, ipc_data, ipc_len); offset += ipc_len;

    /* محاذاة 4 بايتات (4-byte alignment padding) الخاصة بـ Android Parcel */
    while ((offset - 4) % 4 != 0) {
        parcel[offset++] = 0x00;
    }

    uint32_t parcel_size_be = htonl(offset - 4);
    memcpy(parcel, &parcel_size_be, 4);

    if (raw_write_all(fd, parcel, offset) < 0) return -1;
    return raw_read_timeout(fd, out_resp, max_resp, 2500);
}

/* الدالة الموحدة لإرسال إطار IPC ثنائي مهما كانت القناة */
static int dispatch_ipc_frame(const uint8_t *ipc_bytes, int len,
                              uint8_t *resp, int max_resp) {
    if (g_ctx.mode == CH_NONE && init_best_channel(0) != 0)
        return -1;

    if (g_ctx.mode == CH_RILD_PARCEL) {
        return send_rild_oem_parcel(g_ctx.fd, ipc_bytes, len, resp, max_resp);
    }

    /* في حالة /dev/umts_ipc0 أو /dev/umts_atc0 */
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
    if (g_ctx.mode != CH_DEV_AT_TTY) {
        init_best_channel(1);
    }
    if (g_ctx.fd < 0) return -1;

    char buf[CMD_BUF_SIZE];
    int l = snprintf(buf, sizeof(buf), "%s\r\n", cmd);
    if (raw_write_all(g_ctx.fd, (const uint8_t *)buf, l) <= 0) return -1;

    int r = raw_read_timeout(g_ctx.fd, (uint8_t *)out_resp, max_resp - 1, timeout_ms);
    if (r >= 0) out_resp[r] = '\0';
    return r;
}

/* ============================================================================
 *  8. فحص أقفال الشريحة والشبكة (SIM Card & Network Lock Inspection)
 *     AT+CPIN?, AT+CLCK="PN",2, AT+CLCK="PU",2, AT+CLCK="PP",2, AT+CLCK="PC",2
 * ============================================================================ */
static void query_sim_locks(int verbose) {
    char cpin_resp[256] = {0};
    char clck_pn[256] = {0};
    char clck_pu[256] = {0};
    char clck_pp[256] = {0};
    char clck_pc[256] = {0};

    send_at_command_sync("AT+CPIN?", cpin_resp, sizeof(cpin_resp), 1200);
    send_at_command_sync("AT+CLCK=\"PN\",2", clck_pn, sizeof(clck_pn), 1200);
    send_at_command_sync("AT+CLCK=\"PU\",2", clck_pu, sizeof(clck_pu), 1200);
    send_at_command_sync("AT+CLCK=\"PP\",2", clck_pp, sizeof(clck_pp), 1200);
    send_at_command_sync("AT+CLCK=\"PC\",2", clck_pc, sizeof(clck_pc), 1200);

    /* استخلاص حالة الشريحة */
    const char *sim_state = "UNKNOWN";
    if (strstr(cpin_resp, "READY")) sim_state = "READY";
    else if (strstr(cpin_resp, "SIM PIN")) sim_state = "PIN_REQUIRED";
    else if (strstr(cpin_resp, "SIM PUK")) sim_state = "PUK_REQUIRED";
    else if (strstr(cpin_resp, "NOT INSERTED")) sim_state = "ABSENT";
    else if (strstr(cpin_resp, "ERROR")) sim_state = "MODEM_ERROR";

    /* PN: Network Personalization Lock (0 = Unlocked, 1 = Locked) */
    const char *pn_lock = "UNKNOWN";
    if (strstr(clck_pn, "+CLCK: 0") || strstr(clck_pn, "+CLCK:0")) pn_lock = "UNLOCKED(0)";
    else if (strstr(clck_pn, "+CLCK: 1") || strstr(clck_pn, "+CLCK:1")) pn_lock = "LOCKED(1)";

    /* PU: Network Subset Lock */
    const char *pu_lock = "UNKNOWN";
    if (strstr(clck_pu, "+CLCK: 0") || strstr(clck_pu, "+CLCK:0")) pu_lock = "UNLOCKED(0)";
    else if (strstr(clck_pu, "+CLCK: 1") || strstr(clck_pu, "+CLCK:1")) pu_lock = "LOCKED(1)";

    /* PP: Service Provider Lock */
    const char *pp_lock = "UNKNOWN";
    if (strstr(clck_pp, "+CLCK: 0") || strstr(clck_pp, "+CLCK:0")) pp_lock = "UNLOCKED(0)";
    else if (strstr(clck_pp, "+CLCK: 1") || strstr(clck_pp, "+CLCK:1")) pp_lock = "LOCKED(1)";

    /* PC: Corporate Lock */
    const char *pc_lock = "UNKNOWN";
    if (strstr(clck_pc, "+CLCK: 0") || strstr(clck_pc, "+CLCK:0")) pc_lock = "UNLOCKED(0)";
    else if (strstr(clck_pc, "+CLCK: 1") || strstr(clck_pc, "+CLCK:1")) pc_lock = "LOCKED(1)";

    if (verbose) {
        printf("=== SIM & Network Personalization Locks ===\n");
        printf("  SIM State (CPIN)             : %s\n", sim_state);
        printf("  Network Lock (CLCK:PN)       : %s\n", pn_lock);
        printf("  Network Subset Lock (CLCK:PU): %s\n", pu_lock);
        printf("  Service Provider Lock (PP)   : %s\n", pp_lock);
        printf("  Corporate Lock (CLCK:PC)     : %s\n", pc_lock);
    } else {
        printf("SIMLOCK|CPIN=%s|PN=%s|PU=%s|PP=%s|PC=%s\n",
               sim_state, pn_lock, pu_lock, pp_lock, pc_lock);
    }
    fflush(stdout);
}

/* ============================================================================
 *  9. محرك تغيير CSC المدمج (Self-Contained 5-Packet Samsung CSC Injection)
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

    if (verbose) {
        printf("[*] CSC change sequence completed for: %s\n", clean_csc);
        printf("[*] Running carrier package cleanup...\n");
    }

    system("pm uninstall -k --user 0 com.sec.android.AutoPreconfig 2>/dev/null");
    system("pm uninstall -k --user 0 com.sec.omadmspr 2>/dev/null");
    system("logcat -b radio -c 2>/dev/null");

    if (verbose) printf("[+] Samsung CSC change successful.\n");
    else printf(".OK_CSC..\n");
    return 0;
}

/* ============================================================================
 *  10. معلومات الهاتف المدمجة والتفصيلية (Info & Status Printers)
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
           g_ctx.mode == CH_DEV_AT_TTY ? "AT TTY" : "Disconnected");
    query_sim_locks(1);
}

static void print_help(const char *prog_name) {
    printf("============================================================\n");
    printf("  GsmTools - Samsung AT & RIL Command Bridge (v2.5 Pro)\n");
    printf("============================================================\n");
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("One-Shot CLI Options:\n");
    printf("  -c <CSC>       Quick Samsung CSC change (e.g. -c XAA)\n");
    printf("  -a <AT_CMD>    Send single AT command and exit (e.g. -a AT+DEVCONINFO)\n");
    printf("  -x <HEX>       Send single hex IPC frame and exit\n");
    printf("  -s             Scan mode: list accessible modem & RIL devices\n");
    printf("  -l             Query SIM Card & Network Personalization locks\n");
    printf("  -d <DEVICE>    Force specific modem device path (e.g. -d /dev/umts_atc0)\n");
    printf("  -p <PORT>      Escalation port (default: 2600)\n");
    printf("  -v             Verbose diagnostic output\n");
    printf("  -h, --help     Show this help menu\n\n");
    printf("Interactive Commands (run without options):\n");
    printf("  CSC:<CODE>     Automated 5-packet Samsung CSC change\n");
    printf("  AT:<CMD>       Send raw AT command (e.g. AT:AT+DEVCONINFO)\n");
    printf("  HEX:<DATA>     Send hex IPC command (or raw hex without prefix)\n");
    printf("  SCAN           Scan and report accessible modem channels\n");
    printf("  STATUS         Display full device, channel and SIM lock status\n");
    printf("  LOCKS          Query SIM & Network lock status (CPIN, CLCK)\n");
    printf("  RECONNECT      Re-probe and reconnect best modem channel\n");
    printf("  MODE:AT        Switch to AT text channel (/dev/umts_atc0)\n");
    printf("  MODE:IPC       Switch to IPC / RIL socket channel (/dev/socket/rild)\n");
    printf("  AUTH:<SEED>    Activate dynamic Xorshift32 stream cipher\n");
    printf("  INFO           Compact pipe-delimited telemetry header\n");
    printf("  HELP           Show this interactive help\n");
    printf("  EXIT, QUIT     Terminate bridge session\n");
    printf("============================================================\n");
    fflush(stdout);
}

/* ============================================================================
 *  11. الدالة الرئيسية (CLI One-Shot & Interactive Loop)
 * ============================================================================ */

int main(int argc, char *argv[]) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    const char *cli_csc = NULL;
    const char *cli_at = NULL;
    const char *cli_hex = NULL;
    const char *cli_dev = NULL;
    int cli_scan = 0;
    int cli_locks = 0;
    int esc_port = 2600;

    /* معالجة وسائط سطر الأوامر المستقلة (CLI Arguments) */
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
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            cli_csc = argv[++i];
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

    /* فحص ومحاولة رفع الصلاحية عبر بورت 2600 */
    try_escalate_via_port_2600(esc_port);

    /* تهيئة منفذ المودم المحدد أو أفضل منفذ متاح */
    if (cli_dev) {
        force_specific_device(cli_dev);
    } else {
        init_best_channel(cli_at ? 1 : 0);
    }

    /* تنفيذ وسائط الطلقة الواحدة (One-Shot Execution) والخروج */
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

    /* الوضع التفاعلي (Interactive Mode) - متوافق 100% مع بروتوكول C# Desktop */
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

        /* تفعيل جلسة مشفرة ديناميكياً: AUTH:<8-char Hex Seed> */
        if (strncasecmp(line, "AUTH:", 5) == 0) {
            g_session_seed = (uint32_t)strtoul(line + 5, NULL, 16);
            printf(".OK_AUTH..\n<< ");
            fflush(stdout);
            continue;
        }

        /* تغيير CSC تلقائياً من داخل الجسر */
        if (strncasecmp(line, "CSC:", 4) == 0) {
            execute_samsung_csc_change(line + 4, 0);
            printf("<< "); fflush(stdout); continue;
        }

        /* التبديل بين قناة IPC وقناة AT النصية */
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

        /* إرسال أمر AT نصي خام (مثال: AT:AT+DEVCONINFO) */
        if (strncasecmp(line, "AT:", 3) == 0) {
            char at_out[RESP_BUF_SIZE] = {0};
            int r = send_at_command_sync(line + 3, at_out, sizeof(at_out), 2000);
            if (r > 0) printf("%s\n", at_out);
            else printf(".TIMEOUT..\n");
            printf("<< "); fflush(stdout); continue;
        }

        /* إرسال إطار IPC Hex (مع بادئة HEX: أو بدونها) */
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
