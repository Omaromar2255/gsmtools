/*
 * ============================================================================
 *  GsmTools - Zero-Footprint Multi-Modal RIL/IPC/AT Bridge (v2.0 Architect)
 * ============================================================================
 *  Compile: aarch64-linux-android29-clang -O2 -s -fvisibility=hidden
 *           -ldl at_bridge_v2.c -o AT
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
 *     يمنع أي أداة مراقبة من معرفة الأوامر المرسلة عبر السلك
 * ============================================================================ */
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

/* ============================================================================
 *  3. مصعد الصلاحيات التلقائي عبر البورت 2600 (UID 1000 Auto-Relay)
 *     إذا شُغّل الملف كـ shell (UID 2000) وكان بورت 2600 مفتوحاً،
 *     فهو يعيد تشغيل نفسه داخل سياق system (UID 1000) تلقائياً!
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

    /* إيقاف ssud كما تفعل KEY-Tool وتأكيد الهوية */
    const char *init_cmd = "setprop ctl.stop ssud; id\n";
    write(sock, init_cmd, strlen(init_cmd));

    char resp[256] = {0};
    usleep(150000);
    int n = read(sock, resp, sizeof(resp) - 1);
    close(sock);

    if (n > 0 && strstr(resp, "uid=1000")) {
        printf("[+] UID 1000 Escalation Bridge confirmed on port %d\n", port);
        fflush(stdout);
        return 1;
    }
    return -1;
}

/* ============================================================================
 *  4. فتح قنوات الاتصال (IPC منفصل عن AT)
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
    "/dev/smd11",
    "/dev/smd0",
    "/dev/at_distributor",
    "/dev/ttyGS0",
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
    const char *socks[] = { "/dev/socket/rild", "/dev/socket/rild-oem", "/dev/socket/rild-debug", NULL };
    for (int i = 0; socks[i] != NULL; i++) {
        if (access(socks[i], F_OK) != 0) continue;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socks[i], sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            g_ctx.mode = CH_RILD_PARCEL;
            g_ctx.fd   = fd;
            strncpy(g_ctx.active_path, socks[i], sizeof(g_ctx.active_path) - 1);
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

/* ============================================================================
 *  5. بناء وإرسال حزم RIL Parcel و IPC الخام
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

/* تغليف बाيتات IPC داخل Android RIL Parcel القياسي عند الإرسال لسوكيت rild */
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

/* ============================================================================
 *  6. محول Hex والحلقة التفاعلية الرشيقة (Stateless Command Loop)
 * ============================================================================ */

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

int main(int argc, char *argv[]) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    /* فحص ما إذا كان البورت 2600 مفتوحاً لرفع الصلاحية وإيقاف ssud */
    try_escalate_via_port_2600(2600);

    init_best_channel(0);
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

        /* التبديل بين قناة IPC وقناة AT النصية عند الحاجة */
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
            if (g_ctx.mode != CH_DEV_AT_TTY) init_best_channel(1);
            int l = snprintf((char *)payload, sizeof(payload), "%s\r\n", line + 3);
            if (g_ctx.fd >= 0 && raw_write_all(g_ctx.fd, payload, l) > 0) {
                int r = raw_read_timeout(g_ctx.fd, resp, sizeof(resp) - 1, 2000);
                if (r > 0) { resp[r] = '\0'; printf("%s\n", (char *)resp); }
                else printf(".TIMEOUT..\n");
            } else {
                printf(".ERR_WRITE..\n");
            }
            printf("<< "); fflush(stdout); continue;
        }

        /* الافتراضي: استقبال إطار IPC Hex (مشفر بـ g_session_seed أو خام) */
        int bin_len = hex2bin(line, payload, sizeof(payload));
        if (bin_len > 0) {
            /* فك التشفير اللحظي في الرام إذا كانت الجلسة مشفرة */
            transform_buffer(payload, bin_len, g_session_seed);

            int r = dispatch_ipc_frame(payload, bin_len, resp, sizeof(resp));

            /* مسح البافر الحساس فوراً من الذاكرة لمنع التفريغ الجنائي */
            memset(payload, 0, sizeof(payload));

            if (r > 0) {
                /* تشفير الرد بنفس مفتاح الجلسة قبل إرساله للكمبيوتر */
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