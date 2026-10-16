/*
 * ============================================================================
 *  GsmTools - Samsung AT RIL Command Bridge
 * ============================================================================
 *  ملف تنفيذ أوامر AT عبر ADB Shell لأجهزة سامسونج الحديثة
 *  يعمل كجسر بين الكمبيوتر والمودم عبر بروتوكول Samsung IPC
 *
 *  Architecture: ARM64 (aarch64-linux-android)
 *  Target: Samsung Galaxy devices (Qualcomm & Exynos)
 *  Android: 10+ (API 29+)
 *
 *  Build: ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

/* ============================================================================
 *  ثوابت وتعريفات
 * ============================================================================ */

#define VERSION         "1.0"
#define TOOL_NAME       "GsmTools AT Bridge"

/* مسارات أجهزة المودم على سامسونج */
/* Samsung Modem Device Paths - tried in priority order */
static const char *MODEM_DEVICES[] = {
    /* Samsung AT Command Channels */
    "/dev/umts_atc0",
    "/dev/umts_atc1",
    "/dev/umts_at0",

    /* Samsung IPC Channels */
    "/dev/umts_ipc0",
    "/dev/umts_ipc1",

    /* Qualcomm SMD (Shared Memory Driver) - for Snapdragon devices */
    "/dev/smd11",           /* AT command channel */
    "/dev/smd0",            /* Primary channel */
    "/dev/smd7",            /* Data channel */

    /* Samsung AT Distributor */
    "/dev/at_distributor",
    "/dev/at_distributor0",

    /* Generic modem paths */
    "/dev/ttyGS0",
    "/dev/ttyACM0",
    "/dev/ttyUSB0",

    /* Qualcomm DIAG */
    "/dev/diag",

    NULL  /* sentinel */
};

/* مسارات سوكيت RIL */
/* RIL Socket Paths */
static const char *RIL_SOCKETS[] = {
    "/dev/socket/rild",
    "/dev/socket/rild-debug",
    "/dev/socket/rild2",
    "/dev/socket/qmux_radio/qmux_client_socket0",
    NULL
};

/* أحجام البوفر */
#define CMD_BUF_SIZE    4096
#define RESP_BUF_SIZE   8192
#define PROP_BUF_SIZE   256
#define HEX_BUF_SIZE    (CMD_BUF_SIZE * 2 + 1)

/* تايم أوت للقراءة من المودم (مللي ثانية) */
#define MODEM_READ_TIMEOUT_MS   3000
#define MODEM_WRITE_TIMEOUT_MS  1000

/* ============================================================================
 *  هياكل البيانات
 * ============================================================================ */

/* أنواع قنوات الاتصال مع المودم */
typedef enum {
    CHANNEL_NONE = 0,
    CHANNEL_DEVICE,         /* /dev/umts_atc0 etc. */
    CHANNEL_RIL_SOCKET,     /* /dev/socket/rild */
    CHANNEL_TCP_PROXY,      /* TCP localhost proxy (companion app) */
} channel_type_t;

/* معلومات قناة المودم */
typedef struct {
    channel_type_t  type;
    int             fd;
    char            path[256];
    int             tcp_port;
} modem_channel_t;

/* معلومات الجهاز */
typedef struct {
    char product_name[PROP_BUF_SIZE];
    char product_model[PROP_BUF_SIZE];
    char device_carrier[PROP_BUF_SIZE];
    char build_number[PROP_BUF_SIZE];
    char android_ver[PROP_BUF_SIZE];
    char security_patch[PROP_BUF_SIZE];
    char serial_number[PROP_BUF_SIZE];
    char baseband_ver[PROP_BUF_SIZE];
    char product_code[PROP_BUF_SIZE];
    char csc_sales_code[PROP_BUF_SIZE];
} device_info_t;

/* حالة البرنامج */
static modem_channel_t  g_modem  = { .type = CHANNEL_NONE, .fd = -1 };
static device_info_t    g_device = { 0 };
static int              g_verbose = 0;
static volatile int     g_running = 1;

/* ============================================================================
 *  دوال مساعدة (Utility Functions)
 * ============================================================================ */

/* معالج إشارة الإيقاف */
static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

/* قراءة خاصية نظام أندرويد */
/* Read Android system property using __system_property_get or getprop */
static int get_system_property(const char *name, char *value, int maxlen) {
    char cmd[512];
    FILE *fp;

    value[0] = '\0';

    /* الطريقة 1: استخدام getprop مباشرة */
    snprintf(cmd, sizeof(cmd), "getprop %s", name);
    fp = popen(cmd, "r");
    if (fp) {
        if (fgets(value, maxlen, fp) != NULL) {
            /* إزالة السطر الجديد */
            size_t len = strlen(value);
            while (len > 0 && (value[len-1] == '\n' || value[len-1] == '\r'))
                value[--len] = '\0';
        }
        pclose(fp);
    }

    return (value[0] != '\0') ? 0 : -1;
}

/* تحويل hex string إلى bytes */
static int hex_to_bytes(const char *hex, unsigned char *out, int max_out) {
    int len = 0;
    while (*hex && *(hex+1) && len < max_out) {
        unsigned int byte;
        char pair[3] = { hex[0], hex[1], '\0' };
        if (sscanf(pair, "%02x", &byte) != 1)
            break;
        out[len++] = (unsigned char)byte;
        hex += 2;
    }
    return len;
}

/* تحويل bytes إلى hex string */
static void bytes_to_hex(const unsigned char *data, int len, char *out) {
    for (int i = 0; i < len; i++) {
        sprintf(out + (i * 2), "%02X", data[i]);
    }
    out[len * 2] = '\0';
}

/* طباعة hex dump للتشخيص */
static void hex_dump(const char *label, const unsigned char *data, int len) {
    if (!g_verbose) return;
    fprintf(stderr, "[DEBUG] %s (%d bytes):\n", label, len);
    for (int i = 0; i < len; i++) {
        fprintf(stderr, "%02X ", data[i]);
        if ((i + 1) % 16 == 0) fprintf(stderr, "\n");
    }
    if (len % 16 != 0) fprintf(stderr, "\n");
}

/* الحصول على الوقت الحالي كنص */
static void get_timestamp(char *buf, int maxlen) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buf, maxlen, "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

/* ============================================================================
 *  قراءة معلومات الجهاز (Device Info)
 * ============================================================================ */

static void read_device_info(void) {
    get_system_property("ro.product.brand",
                        g_device.product_name, PROP_BUF_SIZE);
    get_system_property("ro.product.model",
                        g_device.product_model, PROP_BUF_SIZE);
    get_system_property("ro.build.display.id",
                        g_device.build_number, PROP_BUF_SIZE);
    get_system_property("ro.build.version.release",
                        g_device.android_ver, PROP_BUF_SIZE);
    get_system_property("ro.build.version.security_patch",
                        g_device.security_patch, PROP_BUF_SIZE);
    get_system_property("ro.serialno",
                        g_device.serial_number, PROP_BUF_SIZE);
    get_system_property("gsm.version.baseband",
                        g_device.baseband_ver, PROP_BUF_SIZE);
    get_system_property("ril.product_code",
                        g_device.product_code, PROP_BUF_SIZE);

    /* استخراج كود الشبكة (Carrier) من product_code */
    /* Extract carrier from product code e.g., SM-S918UZKFXAA -> XAA */
    if (g_device.product_code[0]) {
        int len = strlen(g_device.product_code);
        if (len >= 3) {
            strncpy(g_device.csc_sales_code,
                    g_device.product_code + len - 3, 3);
            g_device.csc_sales_code[3] = '\0';
        }
    }

    /* محاولة قراءة الشبكة بطريقة أخرى */
    if (!g_device.csc_sales_code[0]) {
        get_system_property("ro.csc.sales_code",
                            g_device.csc_sales_code, PROP_BUF_SIZE);
    }
    if (!g_device.csc_sales_code[0]) {
        get_system_property("ril.sales_code",
                            g_device.csc_sales_code, PROP_BUF_SIZE);
    }

    /* تحديد اسم الشبكة من CSC code */
    if (!g_device.device_carrier[0]) {
        const char *csc = g_device.csc_sales_code;
        if      (!strcmp(csc, "ATT") || !strcmp(csc, "XAA")) strcpy(g_device.device_carrier, "ATT");
        else if (!strcmp(csc, "TMB") || !strcmp(csc, "XAG")) strcpy(g_device.device_carrier, "T-Mobile");
        else if (!strcmp(csc, "SPR") || !strcmp(csc, "XAS")) strcpy(g_device.device_carrier, "Sprint");
        else if (!strcmp(csc, "VZW") || !strcmp(csc, "XAB")) strcpy(g_device.device_carrier, "Verizon");
        else if (!strcmp(csc, "USC"))                        strcpy(g_device.device_carrier, "US Cellular");
        else if (!strcmp(csc, "CCT"))                        strcpy(g_device.device_carrier, "Comcast");
        else if (csc[0]) strncpy(g_device.device_carrier, csc, PROP_BUF_SIZE - 1);
        else strcpy(g_device.device_carrier, "Unknown");
    }
}

/* عرض بانر البرنامج ومعلومات الجهاز */
static void print_banner(void) {
    printf("\n");
    printf("******** -------------------------- ********\n");
    printf("*******                              *******\n");
    printf("****** ---------- 2026 ------------- ******\n");
    printf("*****                                  *****\n");
    printf("****    %s v%s    ****\n", TOOL_NAME, VERSION);
    printf("***                                      ***\n");
    printf("** ------   Samsung AT RIL Bridge   ------ **\n");
    printf("* ---------------------------------------- *\n");
    printf("\n");
    printf("Product Name  : %s\n", g_device.product_name);
    printf("Product Model : %s\n", g_device.product_model);
    printf("Product Code  : %s\n", g_device.product_code);
    printf("Device Carrier: %s\n", g_device.device_carrier);
    printf("CSC Code      : %s\n", g_device.csc_sales_code);
    printf("Build  Number : %s\n", g_device.build_number);
    printf("Android Ver   : %s\n", g_device.android_ver);
    printf("Security Patch: %s\n", g_device.security_patch);
    printf("Serial Number : %s\n", g_device.serial_number);
    printf("Baseband Ver  : %s\n", g_device.baseband_ver);
    printf("\n");
    fflush(stdout);
}

/* ============================================================================
 *  فتح قناة المودم (Modem Channel Open)
 * ============================================================================ */

/* فتح جهاز المودم المباشر */
static int open_modem_device(modem_channel_t *ch) {
    for (int i = 0; MODEM_DEVICES[i] != NULL; i++) {
        const char *path = MODEM_DEVICES[i];

        /* فحص وجود الملف أولاً */
        if (access(path, F_OK) != 0)
            continue;

        if (g_verbose)
            fprintf(stderr, "[INFO] Trying modem device: %s\n", path);

        int fd = open(path, O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (fd < 0) {
            if (g_verbose)
                fprintf(stderr, "[WARN] Cannot open %s: %s\n",
                        path, strerror(errno));
            continue;
        }

        /* إعداد Terminal Settings إذا كان tty device */
        if (isatty(fd)) {
            struct termios tio;
            if (tcgetattr(fd, &tio) == 0) {
                cfmakeraw(&tio);
                tio.c_cflag |= (CLOCAL | CREAD);
                tio.c_cflag &= ~CRTSCTS;
                tio.c_cc[VMIN]  = 0;
                tio.c_cc[VTIME] = 10;  /* 1 second timeout */
                tcsetattr(fd, TCSANOW, &tio);
                tcflush(fd, TCIOFLUSH);
            }
        }

        ch->type = CHANNEL_DEVICE;
        ch->fd   = fd;
        strncpy(ch->path, path, sizeof(ch->path) - 1);

        fprintf(stderr, "[OK] Modem device opened: %s (fd=%d)\n", path, fd);
        return 0;
    }
    return -1;
}

/* فتح سوكيت RIL */
static int open_ril_socket(modem_channel_t *ch) {
    for (int i = 0; RIL_SOCKETS[i] != NULL; i++) {
        const char *path = RIL_SOCKETS[i];

        if (access(path, F_OK) != 0)
            continue;

        if (g_verbose)
            fprintf(stderr, "[INFO] Trying RIL socket: %s\n", path);

        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            if (g_verbose)
                fprintf(stderr, "[WARN] Cannot connect to %s: %s\n",
                        path, strerror(errno));
            close(fd);
            continue;
        }

        ch->type = CHANNEL_RIL_SOCKET;
        ch->fd   = fd;
        strncpy(ch->path, path, sizeof(ch->path) - 1);

        fprintf(stderr, "[OK] RIL socket connected: %s (fd=%d)\n", path, fd);
        return 0;
    }
    return -1;
}

/* فتح اتصال TCP مع تطبيق مساعد على الجهاز */
static int open_tcp_proxy(modem_channel_t *ch, int port) {
    if (g_verbose)
        fprintf(stderr, "[INFO] Trying TCP proxy on localhost:%d\n", port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* تايم أوت للاتصال */
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (g_verbose)
            fprintf(stderr, "[WARN] Cannot connect to localhost:%d: %s\n",
                    port, strerror(errno));
        close(fd);
        return -1;
    }

    ch->type     = CHANNEL_TCP_PROXY;
    ch->fd       = fd;
    ch->tcp_port = port;
    snprintf(ch->path, sizeof(ch->path), "tcp://127.0.0.1:%d", port);

    fprintf(stderr, "[OK] TCP proxy connected: %s (fd=%d)\n", ch->path, fd);
    return 0;
}

/* فتح أفضل قناة متاحة - يجرب الكل بالترتيب */
static int open_modem_channel(modem_channel_t *ch, int tcp_port) {

    fprintf(stderr, "[*] Scanning modem channels...\n");

    /* المحاولة 1: جهاز المودم المباشر */
    if (open_modem_device(ch) == 0)
        return 0;

    /* المحاولة 2: سوكيت RIL */
    if (open_ril_socket(ch) == 0)
        return 0;

    /* المحاولة 3: TCP proxy (companion app) */
    if (tcp_port > 0) {
        if (open_tcp_proxy(ch, tcp_port) == 0)
            return 0;
    }

    /* محاولة بورتات TCP شائعة */
    int common_ports[] = { 2600, 2601, 5510, 7100, 0 };
    for (int i = 0; common_ports[i] != 0; i++) {
        if (open_tcp_proxy(ch, common_ports[i]) == 0)
            return 0;
    }

    fprintf(stderr, "[ERROR] No modem channel available!\n");
    fprintf(stderr, "[HINT] Possible solutions:\n");
    fprintf(stderr, "  1. Run as root: su -c /data/local/tmp/AT\n");
    fprintf(stderr, "  2. Install companion app with system permissions\n");
    fprintf(stderr, "  3. Set SELinux to permissive: setenforce 0\n");
    return -1;
}

/* إغلاق قناة المودم */
static void close_modem_channel(modem_channel_t *ch) {
    if (ch->fd >= 0) {
        close(ch->fd);
        ch->fd = -1;
    }
    ch->type = CHANNEL_NONE;
}

/* ============================================================================
 *  إرسال واستقبال البيانات من المودم
 * ============================================================================ */

/* إرسال بيانات إلى المودم */
static int modem_write(modem_channel_t *ch,
                       const unsigned char *data, int len) {
    if (ch->fd < 0) return -1;

    hex_dump("SEND", data, len);

    int total = 0;
    while (total < len) {
        int n = write(ch->fd, data + total, len - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000); /* 10ms */
                continue;
            }
            fprintf(stderr, "[ERROR] Modem write error: %s\n",
                    strerror(errno));
            return -1;
        }
        total += n;
    }
    return total;
}

/* قراءة بيانات من المودم مع تايم أوت */
static int modem_read(modem_channel_t *ch,
                      unsigned char *buf, int maxlen, int timeout_ms) {
    if (ch->fd < 0) return -1;

    fd_set rfds;
    struct timeval tv;
    int total = 0;

    /* انتظار البيانات الأولى */
    FD_ZERO(&rfds);
    FD_SET(ch->fd, &rfds);
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(ch->fd + 1, &rfds, NULL, NULL, &tv);
    if (ret <= 0) {
        if (ret == 0 && g_verbose)
            fprintf(stderr, "[WARN] Modem read timeout (%dms)\n", timeout_ms);
        return ret;
    }

    /* قراءة كل البيانات المتاحة */
    while (total < maxlen) {
        int n = read(ch->fd, buf + total, maxlen - total);
        if (n <= 0) break;
        total += n;

        /* فحص سريع إذا فيه بيانات إضافية */
        FD_ZERO(&rfds);
        FD_SET(ch->fd, &rfds);
        tv.tv_sec  = 0;
        tv.tv_usec = 50000; /* 50ms for additional data */
        if (select(ch->fd + 1, &rfds, NULL, NULL, &tv) <= 0)
            break;
    }

    hex_dump("RECV", buf, total);
    return total;
}

/* ============================================================================
 *  بروتوكول Samsung IPC للأوامر
 *  Samsung IPC Protocol - AT Command Interface
 * ============================================================================ */

/*
 * بنية أمر Samsung IPC المستخرجة من تحليل KEY-Tool:
 *
 * Format: [06] [01] [00] [LEN] [00] [00] [PAYLOAD...]
 *
 *   06     = Header / Service ID (Samsung OEM)
 *   01     = Channel / Command Group (AT)
 *   00     = Type (SET)
 *   LEN    = Total packet length
 *   00 00  = Reserved
 *   PAYLOAD = AT command data (e.g., CSC code as ASCII hex)
 *
 * Example: 0601000e00005841410000584141
 *   -> XAA\0\0XAA (CSC change to open/unlocked)
 */

/* إنشاء حزمة IPC لأمر AT */
static int build_ipc_packet(const unsigned char *payload, int payload_len,
                            unsigned char *out, int max_out) {
    /* Header: 06 01 00 */
    int total_len = 6 + payload_len; /* header(6) + payload */

    if (total_len > max_out) {
        fprintf(stderr, "[ERROR] IPC packet too large: %d > %d\n",
                total_len, max_out);
        return -1;
    }

    out[0] = 0x06;                         /* Service ID */
    out[1] = 0x01;                         /* Command Group: AT */
    out[2] = 0x00;                         /* Type: SET */
    out[3] = (unsigned char)total_len;     /* Total Length */
    out[4] = 0x00;                         /* Reserved */
    out[5] = 0x00;                         /* Reserved */

    if (payload_len > 0)
        memcpy(out + 6, payload, payload_len);

    return total_len;
}

/* إرسال أمر AT خام (نص) عبر المودم */
static int send_raw_at_command(modem_channel_t *ch,
                               const char *at_cmd,
                               unsigned char *response, int resp_max) {
    unsigned char cmd_buf[CMD_BUF_SIZE];
    int cmd_len = strlen(at_cmd);

    /* إضافة \r\n في نهاية الأمر */
    snprintf((char *)cmd_buf, sizeof(cmd_buf), "%s\r\n", at_cmd);
    cmd_len = strlen((char *)cmd_buf);

    if (modem_write(ch, cmd_buf, cmd_len) < 0)
        return -1;

    return modem_read(ch, response, resp_max, MODEM_READ_TIMEOUT_MS);
}

/* إرسال أمر IPC مشفر بالـ hex */
static int send_hex_command(modem_channel_t *ch,
                            const char *hex_cmd,
                            unsigned char *response, int resp_max) {
    unsigned char cmd_bytes[CMD_BUF_SIZE];
    int cmd_len;

    /* تحويل الأمر من hex إلى bytes */
    cmd_len = hex_to_bytes(hex_cmd, cmd_bytes, sizeof(cmd_bytes));
    if (cmd_len <= 0) {
        fprintf(stderr, "[ERROR] Invalid hex command\n");
        return -1;
    }

    /* إرسال البايتات مباشرة إلى المودم */
    if (modem_write(ch, cmd_bytes, cmd_len) < 0)
        return -1;

    /* قراءة الرد */
    return modem_read(ch, response, resp_max, MODEM_READ_TIMEOUT_MS);
}

/* ============================================================================
 *  أوامر خاصة (Special Commands)
 * ============================================================================ */

/* فحص حالة الجهاز وعرض معلومات إضافية */
static void cmd_device_status(void) {
    printf("\n=== Device Status ===\n");
    read_device_info();
    print_banner();

    /* فحص SELinux */
    char selinux[64] = "unknown";
    FILE *fp = popen("getenforce 2>/dev/null", "r");
    if (fp) {
        if (fgets(selinux, sizeof(selinux), fp)) {
            selinux[strcspn(selinux, "\r\n")] = '\0';
        }
        pclose(fp);
    }
    printf("SELinux Status: %s\n", selinux);

    /* فحص uid */
    printf("Running as UID: %d\n", getuid());
    printf("Modem Channel : %s\n",
           g_modem.type != CHANNEL_NONE ? g_modem.path : "Not connected");
    printf("\n");
    fflush(stdout);
}

/* عرض الأجهزة المتاحة */
static void cmd_list_devices(void) {
    printf("\n=== Available Modem Devices ===\n");
    for (int i = 0; MODEM_DEVICES[i] != NULL; i++) {
        int exists = (access(MODEM_DEVICES[i], F_OK) == 0);
        int readable = (access(MODEM_DEVICES[i], R_OK) == 0);
        int writable = (access(MODEM_DEVICES[i], W_OK) == 0);
        printf("  %s [%s] %s%s\n",
               MODEM_DEVICES[i],
               exists ? "EXISTS" : "------",
               readable ? "R" : "-",
               writable ? "W" : "-");
    }
    printf("\n=== RIL Sockets ===\n");
    for (int i = 0; RIL_SOCKETS[i] != NULL; i++) {
        int exists = (access(RIL_SOCKETS[i], F_OK) == 0);
        printf("  %s [%s]\n",
               RIL_SOCKETS[i],
               exists ? "EXISTS" : "------");
    }
    printf("\n");
    fflush(stdout);
}

/* عرض المساعدة */
static void cmd_help(void) {
    printf("\n=== GsmTools AT Bridge - Commands ===\n\n");
    printf("  Interactive Commands:\n");
    printf("  -------------------------------------------\n");
    printf("  AT...           Send raw AT command (e.g., AT+CLCK=?)\n");
    printf("  HEX:AABBCC...  Send hex-encoded IPC command\n");
    printf("  IPC:PAYLOAD     Build IPC packet and send\n");
    printf("  CSC:XXX         Change CSC code (e.g., CSC:XAA)\n");
    printf("\n");
    printf("  Utility Commands:\n");
    printf("  -------------------------------------------\n");
    printf("  STATUS          Show device info & connection status\n");
    printf("  SCAN            List available modem devices\n");
    printf("  RECONNECT       Reconnect to modem\n");
    printf("  VERBOSE         Toggle debug output\n");
    printf("  HELP            Show this help\n");
    printf("  EXIT / QUIT     Exit the program\n");
    printf("\n");
    fflush(stdout);
}

/* ============================================================================
 *  بناء أوامر CSC الشائعة
 *  Build Common CSC Change Commands
 * ============================================================================ */

/*
 * أوامر تغيير CSC المستخرجة من تحليل KEY-Tool:
 *
 * الأمر الأساسي يحتوي على كود CSC بترميز ASCII:
 *   XAA = 58 41 41 (AT&T Open)
 *   TMB = 54 4D 42 (T-Mobile)
 *   etc.
 *
 * يتم إرساله في 5 أوامر IPC متتالية
 */

/* إنشاء وإرسال أوامر تغيير CSC */
static int send_csc_change(modem_channel_t *ch, const char *csc_code) {
    unsigned char response[RESP_BUF_SIZE];
    int resp_len;
    char hex_cmd[256];

    if (strlen(csc_code) < 2 || strlen(csc_code) > 5) {
        fprintf(stderr, "[ERROR] Invalid CSC code: %s (must be 2-5 chars)\n",
                csc_code);
        return -1;
    }

    printf("[*] Starting CSC change to: %s\n", csc_code);

    /* تحويل CSC إلى hex ASCII */
    char csc_hex[16] = {0};
    for (int i = 0; csc_code[i]; i++) {
        sprintf(csc_hex + (i*2), "%02X", (unsigned char)csc_code[i]);
    }

    /* الأمر 1: CSC مزدوج مع فاصل */
    snprintf(hex_cmd, sizeof(hex_cmd),
             "0601000e0000%s0000%s", csc_hex, csc_hex);
    printf("[>] CMD 1: %s\n", hex_cmd);
    resp_len = send_hex_command(ch, hex_cmd, response, sizeof(response));
    if (resp_len > 0) {
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        bytes_to_hex(response, resp_len, resp_hex);
        printf("[<] RSP 1: %s\n", resp_hex);
    }

    usleep(100000); /* 100ms delay */

    /* الأمر 2: CSC مفرد */
    snprintf(hex_cmd, sizeof(hex_cmd),
             "060100090000%s", csc_hex);
    printf("[>] CMD 2: %s\n", hex_cmd);
    resp_len = send_hex_command(ch, hex_cmd, response, sizeof(response));
    if (resp_len > 0) {
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        bytes_to_hex(response, resp_len, resp_hex);
        printf("[<] RSP 2: %s\n", resp_hex);
    }

    usleep(100000);

    /* الأمر 3: CSC ثلاثي */
    snprintf(hex_cmd, sizeof(hex_cmd),
             "060100120000%s0000%s%s", csc_hex, csc_hex, csc_hex);
    printf("[>] CMD 3: %s\n", hex_cmd);
    resp_len = send_hex_command(ch, hex_cmd, response, sizeof(response));
    if (resp_len > 0) {
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        bytes_to_hex(response, resp_len, resp_hex);
        printf("[<] RSP 3: %s\n", resp_hex);
    }

    usleep(100000);

    /* الأمر 4: مسح الراديو لوج */
    snprintf(hex_cmd, sizeof(hex_cmd), "060100120000");
    printf("[>] CMD 4: %s\n", hex_cmd);
    resp_len = send_hex_command(ch, hex_cmd, response, sizeof(response));
    if (resp_len > 0) {
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        bytes_to_hex(response, resp_len, resp_hex);
        printf("[<] RSP 4: %s\n", resp_hex);
    }

    usleep(100000);

    /* الأمر 5: CSC مفرد (تأكيد) */
    snprintf(hex_cmd, sizeof(hex_cmd),
             "060100090000%s", csc_hex);
    printf("[>] CMD 5: %s\n", hex_cmd);
    resp_len = send_hex_command(ch, hex_cmd, response, sizeof(response));
    if (resp_len > 0) {
        char resp_hex[RESP_BUF_SIZE * 2 + 1];
        bytes_to_hex(response, resp_len, resp_hex);
        printf("[<] RSP 5: %s\n", resp_hex);
    }

    printf("[*] CSC change sequence completed for: %s\n", csc_code);
    fflush(stdout);
    return 0;
}

/* ============================================================================
 *  تنظيف التطبيقات بعد العملية
 *  Post-Operation Cleanup
 * ============================================================================ */

static void post_operation_cleanup(void) {
    printf("[*] Running post-operation cleanup...\n");

    const char *cleanup_cmds[] = {
        "pm uninstall -k --user 0 com.sec.android.AutoPreconfig",
        "pm uninstall -k --user 0 com.sec.omadmspr",
        "pm clear com.samsung.android.app.mobiledoctor",
        NULL
    };

    for (int i = 0; cleanup_cmds[i]; i++) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s 2>&1", cleanup_cmds[i]);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char result[256];
            if (fgets(result, sizeof(result), fp)) {
                result[strcspn(result, "\r\n")] = '\0';
                printf("  [%d] %s -> %s\n", i+1, cleanup_cmds[i], result);
            }
            pclose(fp);
        }
    }
    printf("[*] Cleanup done\n\n");
    fflush(stdout);
}

/* ============================================================================
 *  حلقة الأوامر التفاعلية (Interactive Command Loop)
 * ============================================================================ */

static void interactive_loop(void) {
    char line[HEX_BUF_SIZE];
    unsigned char response[RESP_BUF_SIZE];
    char resp_hex[RESP_BUF_SIZE * 2 + 1];
    int resp_len;

    printf("Ready. Type HELP for commands.\n\n");
    printf("#> ");
    fflush(stdout);

    while (g_running && fgets(line, sizeof(line), stdin) != NULL) {
        /* إزالة الأسطر الجديدة */
        line[strcspn(line, "\r\n")] = '\0';

        /* تجاهل الأسطر الفارغة */
        if (line[0] == '\0') {
            printf("#> ");
            fflush(stdout);
            continue;
        }

        /* ============ أوامر خاصة ============ */

        if (strcasecmp(line, "EXIT") == 0 || strcasecmp(line, "QUIT") == 0) {
            printf("[*] Exiting...\n");
            break;
        }

        if (strcasecmp(line, "HELP") == 0) {
            cmd_help();
            printf("#> ");
            fflush(stdout);
            continue;
        }

        if (strcasecmp(line, "STATUS") == 0) {
            cmd_device_status();
            printf("#> ");
            fflush(stdout);
            continue;
        }

        if (strcasecmp(line, "SCAN") == 0) {
            cmd_list_devices();
            printf("#> ");
            fflush(stdout);
            continue;
        }

        if (strcasecmp(line, "VERBOSE") == 0) {
            g_verbose = !g_verbose;
            printf("[*] Verbose mode: %s\n", g_verbose ? "ON" : "OFF");
            printf("#> ");
            fflush(stdout);
            continue;
        }

        if (strcasecmp(line, "CLEANUP") == 0) {
            post_operation_cleanup();
            printf("#> ");
            fflush(stdout);
            continue;
        }

        if (strcasecmp(line, "RECONNECT") == 0) {
            printf("[*] Reconnecting...\n");
            close_modem_channel(&g_modem);
            if (open_modem_channel(&g_modem, 0) == 0)
                printf("[OK] Reconnected to: %s\n", g_modem.path);
            else
                printf("[ERROR] Reconnect failed\n");
            printf("#> ");
            fflush(stdout);
            continue;
        }

        /* ============ أوامر CSC ============ */

        if (strncasecmp(line, "CSC:", 4) == 0) {
            if (g_modem.type == CHANNEL_NONE) {
                printf("[ERROR] Not connected to modem\n");
            } else {
                send_csc_change(&g_modem, line + 4);
            }
            printf("#> ");
            fflush(stdout);
            continue;
        }

        /* ============ أوامر HEX (IPC مباشر) ============ */

        if (strncasecmp(line, "HEX:", 4) == 0) {
            if (g_modem.type == CHANNEL_NONE) {
                printf("[ERROR] Not connected to modem\n");
            } else {
                resp_len = send_hex_command(&g_modem, line + 4,
                                            response, sizeof(response));
                if (resp_len > 0) {
                    bytes_to_hex(response, resp_len, resp_hex);
                    printf("[<] %s\n", resp_hex);
                } else if (resp_len == 0) {
                    printf("[<] (no response)\n");
                } else {
                    printf("[ERROR] Send failed\n");
                }
            }
            printf("#> ");
            fflush(stdout);
            continue;
        }

        /* ============ أوامر IPC (بناء الحزمة تلقائياً) ============ */

        if (strncasecmp(line, "IPC:", 4) == 0) {
            if (g_modem.type == CHANNEL_NONE) {
                printf("[ERROR] Not connected to modem\n");
            } else {
                unsigned char payload[CMD_BUF_SIZE];
                unsigned char packet[CMD_BUF_SIZE];
                int payload_len = hex_to_bytes(line + 4, payload,
                                               sizeof(payload));
                int packet_len = build_ipc_packet(payload, payload_len,
                                                   packet, sizeof(packet));
                if (packet_len > 0) {
                    char pkt_hex[CMD_BUF_SIZE * 2 + 1];
                    bytes_to_hex(packet, packet_len, pkt_hex);
                    printf("[>] IPC Packet: %s\n", pkt_hex);

                    if (modem_write(&g_modem, packet, packet_len) >= 0) {
                        resp_len = modem_read(&g_modem, response,
                                              sizeof(response),
                                              MODEM_READ_TIMEOUT_MS);
                        if (resp_len > 0) {
                            bytes_to_hex(response, resp_len, resp_hex);
                            printf("[<] %s\n", resp_hex);
                        } else {
                            printf("[<] (no response)\n");
                        }
                    }
                }
            }
            printf("#> ");
            fflush(stdout);
            continue;
        }

        /* ============ أوامر AT خام ============ */

        if (strncasecmp(line, "AT", 2) == 0) {
            if (g_modem.type == CHANNEL_NONE) {
                printf("[ERROR] Not connected to modem\n");
            } else {
                resp_len = send_raw_at_command(&g_modem, line,
                                               response, sizeof(response));
                if (resp_len > 0) {
                    /* عرض النص كما هو */
                    response[resp_len] = '\0';
                    printf("[<] %s\n", (char *)response);
                } else if (resp_len == 0) {
                    printf("[<] (no response)\n");
                } else {
                    printf("[ERROR] Send failed\n");
                }
            }
            printf("#> ");
            fflush(stdout);
            continue;
        }

        /* ============ إدخال hex خام (بدون بادئة) ============ */
        /* إذا كان الإدخال يبدو كـ hex (أحرف 0-9, A-F فقط) */
        {
            int is_hex = 1;
            int hex_len = strlen(line);
            if (hex_len >= 4 && hex_len % 2 == 0) {
                for (int i = 0; i < hex_len; i++) {
                    char c = line[i];
                    if (!((c >= '0' && c <= '9') ||
                          (c >= 'a' && c <= 'f') ||
                          (c >= 'A' && c <= 'F'))) {
                        is_hex = 0;
                        break;
                    }
                }
            } else {
                is_hex = 0;
            }

            if (is_hex && g_modem.type != CHANNEL_NONE) {
                /* إرسال كأمر hex مباشر */
                resp_len = send_hex_command(&g_modem, line,
                                            response, sizeof(response));
                if (resp_len > 0) {
                    bytes_to_hex(response, resp_len, resp_hex);
                    printf("[<] %s\n", resp_hex);
                } else if (resp_len == 0) {
                    printf("[<] (no response)\n");
                } else {
                    printf("[ERROR] Send failed\n");
                }
            } else if (is_hex && g_modem.type == CHANNEL_NONE) {
                printf("[ERROR] Not connected to modem\n");
            } else {
                printf("[?] Unknown command: %s (type HELP)\n", line);
            }
        }

        printf("#> ");
        fflush(stdout);
    }
}

/* ============================================================================
 *  نقطة الدخول الرئيسية (Main Entry Point)
 * ============================================================================ */

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  -p PORT     TCP proxy port (default: auto-detect)\n");
    printf("  -d DEVICE   Force specific modem device path\n");
    printf("  -v          Verbose/debug mode\n");
    printf("  -s          Scan mode - list devices and exit\n");
    printf("  -c CSC      Quick CSC change mode (e.g., -c XAA)\n");
    printf("  -x HEX      Send single hex command and exit\n");
    printf("  -a ATCMD    Send single AT command and exit\n");
    printf("  -h          Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                    Interactive mode\n", prog);
    printf("  %s -c XAA             Change CSC to XAA\n", prog);
    printf("  %s -x 060100090000584141   Send hex IPC command\n", prog);
    printf("  %s -a AT+CLCK=?       Send AT command\n", prog);
    printf("  %s -d /dev/umts_atc0  Force modem device\n", prog);
    printf("  %s -p 2600            Use TCP proxy on port 2600\n", prog);
    printf("\n");
}

int main(int argc, char *argv[]) {
    int   opt;
    int   tcp_port   = 0;
    char *force_dev  = NULL;
    char *quick_csc  = NULL;
    char *quick_hex  = NULL;
    char *quick_at   = NULL;
    int   scan_only  = 0;

    /* معالجة الإشارات */
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    /* تحليل الخيارات */
    while ((opt = getopt(argc, argv, "p:d:c:x:a:vsh")) != -1) {
        switch (opt) {
            case 'p': tcp_port  = atoi(optarg); break;
            case 'd': force_dev = optarg;       break;
            case 'c': quick_csc = optarg;       break;
            case 'x': quick_hex = optarg;       break;
            case 'a': quick_at  = optarg;       break;
            case 'v': g_verbose = 1;            break;
            case 's': scan_only = 1;            break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* قراءة معلومات الجهاز */
    read_device_info();

    /* عرض البانر */
    print_banner();

    /* وضع الفحص فقط */
    if (scan_only) {
        cmd_list_devices();
        return 0;
    }

    /* فتح قناة المودم */
    if (force_dev) {
        /* استخدام جهاز محدد */
        int fd = open(force_dev, O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (fd < 0) {
            fprintf(stderr, "[ERROR] Cannot open %s: %s\n",
                    force_dev, strerror(errno));
            return 1;
        }
        g_modem.type = CHANNEL_DEVICE;
        g_modem.fd   = fd;
        strncpy(g_modem.path, force_dev, sizeof(g_modem.path) - 1);
        fprintf(stderr, "[OK] Forced device: %s (fd=%d)\n", force_dev, fd);
    } else {
        /* البحث التلقائي عن المودم */
        if (open_modem_channel(&g_modem, tcp_port) != 0) {
            fprintf(stderr,
                    "[WARN] No modem channel found. "
                    "Entering interactive mode anyway.\n");
            fprintf(stderr,
                    "[HINT] Use SCAN to list devices, "
                    "RECONNECT to retry.\n\n");
        }
    }

    /* ============ أوضاع التنفيذ السريع ============ */

    /* تغيير CSC سريع */
    if (quick_csc) {
        if (g_modem.type == CHANNEL_NONE) {
            fprintf(stderr, "[ERROR] No modem channel for CSC change\n");
            return 1;
        }
        int ret = send_csc_change(&g_modem, quick_csc);
        if (ret == 0) post_operation_cleanup();
        close_modem_channel(&g_modem);
        return (ret == 0) ? 0 : 1;
    }

    /* إرسال أمر hex واحد */
    if (quick_hex) {
        if (g_modem.type == CHANNEL_NONE) {
            fprintf(stderr, "[ERROR] No modem channel\n");
            return 1;
        }
        unsigned char response[RESP_BUF_SIZE];
        int resp_len = send_hex_command(&g_modem, quick_hex,
                                         response, sizeof(response));
        if (resp_len > 0) {
            char resp_hex[RESP_BUF_SIZE * 2 + 1];
            bytes_to_hex(response, resp_len, resp_hex);
            printf("%s\n", resp_hex);
        }
        close_modem_channel(&g_modem);
        return (resp_len >= 0) ? 0 : 1;
    }

    /* إرسال أمر AT واحد */
    if (quick_at) {
        if (g_modem.type == CHANNEL_NONE) {
            fprintf(stderr, "[ERROR] No modem channel\n");
            return 1;
        }
        unsigned char response[RESP_BUF_SIZE];
        int resp_len = send_raw_at_command(&g_modem, quick_at,
                                            response, sizeof(response));
        if (resp_len > 0) {
            response[resp_len] = '\0';
            printf("%s\n", (char *)response);
        }
        close_modem_channel(&g_modem);
        return (resp_len >= 0) ? 0 : 1;
    }

    /* ============ الوضع التفاعلي ============ */
    interactive_loop();

    /* تنظيف */
    close_modem_channel(&g_modem);

    printf("[*] Goodbye.\n");
    return 0;
}
