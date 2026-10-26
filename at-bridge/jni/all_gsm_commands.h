/**
 * ============================================================================
 *  all_gsm_commands.h — GsmTools v4.0 Comprehensive Command Dictionary
 * ============================================================================
 *  SINGLE SOURCE OF TRUTH for every command in the GsmTools system.
 *  Compiled into the AT bridge binary (at_bridge.c) and mirrored by the
 *  Python tools (tools/at_tester.py) and the desktop workflows.
 *
 *  Provenance key (documented in docs/COMMANDS_REFERENCE.md):
 *    [OBS]  observed live against real Samsung modems (L24 session logs)
 *    [STD]  3GPP TS 27.007 standard command
 *    [RE ]  reverse-engineered from the original AT tool (Ghidra dumps)
 *    [EXP]  experimental — encoding not fully confirmed, use with care
 *
 *  Channels:
 *    SECRIL  = libsecril-client.so (send_recv_cmd)          — strongest
 *    IPC     = /dev/umts_ipc0 | umts_ipc1 | umts_rfs0 | /dev/diag
 *    RILD    = /dev/socket/rild{,-oem,-debug,2} | qmux_radio (OEM parcel 59)
 *    ATTY    = /dev/umts_atc0 | atc1 | at0 | at_distributor[0] | smd11 | smd0
 *              | smd7 | ttyGS0 | ttyACM0 | ttyUSB0
 *    RELAY   = TCP 127.0.0.1:2600 (UID-1000 companion bridge)
 * ============================================================================
 */
#ifndef ALL_GSM_COMMANDS_H
#define ALL_GSM_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. SHELL & PRIVILEGE ESCALATION (run via system() / port-2600 relay)
 * ========================================================================= */
#define CMD_FLUSH_RADIO_LOGS \
    "logcat -b radio -c > /dev/null 2>&1"

#define CMD_FLUSH_MAIN_LOGS \
    "logcat -b system,main -c > /dev/null 2>&1"

#define CMD_SET_SCREEN_TIMEOUT \
    "settings put system screen_off_timeout 600000 > /dev/null 2>&1"

#define CMD_SET_ASSISTANT_SECURE \
    "settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1"

#define CMD_SET_ASSISTANT_SYSTEM \
    "settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1"

#define CMD_SET_DOUBLEPRESS_GLOBAL \
    "settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1"

#define CMD_RESTORE_ASSISTANT_GOOGLE \
    "settings put Secure assistant com.google.android.googlequicksearchbox/com.google.android.voiceinteraction.GsaVoiceInteractionService > /dev/null 2>&1"

#define CMD_LAUNCH_SETTINGS \
    "am start -a android.settings.SETTINGS > /dev/null 2>&1"

#define CMD_TRIGGER_ASSIST_KEY \
    "input keyevent 219 > /dev/null 2>&1"          /* KEYCODE_ASSIST */

#define CMD_INJECT_NC_ROOT_SHELL \
    "input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'"

#define CMD_KEYEVENT_ENTER \
    "input keyevent 66 > /dev/null 2>&1"
#define CMD_KEYEVENT_TAB \
    "input keyevent 61 > /dev/null 2>&1"
#define CMD_KEYEVENT_HOME \
    "input keyevent 3 > /dev/null 2>&1"
#define CMD_KEYEVENT_BACK \
    "input keyevent 4 > /dev/null 2>&1"
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
#define CMD_UNINSTALL_CARRIER_CODE_CHANGER \
    "pm uninstall -k --user 0 com.sec.android.CarrierCodeChanger > /dev/null 2>&1"
#define CMD_FORCESTOP_SKATD \
    "am force-stop com.skatd.unlock > /dev/null 2>&1"
#define CMD_DISABLE_WIFI \
    "cmd wifi set-wifi-enabled disabled > /dev/null 2>&1"

/* EFS SIM-Lock-Byte operations (root required) — [RE] */
#define CMD_EFS_SLB_BACKUP \
    "mv /efs/root/data/SLB.bin /efs/root/data/SLB.bin.bak 2>/dev/null"
#define CMD_EFS_SLB_ZERO \
    "echo 00000000 > /efs/root/data/SLB.bin 2>/dev/null"
#define CMD_EFS_SLB_PROTECT \
    "chmod 0400 /efs/root/data/SLB.bin 2>/dev/null"

/* EFS factory-mode toggles (from embedded FotaAgent dump) — [RE] */
#define CMD_EFS_FACTORY_MODE_ON   "echo ON > /efs/FactoryApp/factorymode 2>/dev/null"
#define CMD_EFS_FACTORY_MODE_OFF  "echo OFF > /efs/FactoryApp/factorymode 2>/dev/null"
#define CMD_EFS_KEYSTR_ON         "echo ON > /efs/FactoryApp/keystr 2>/dev/null"
#define CMD_EFS_KEYSTR_OFF        "echo OFF > /efs/FactoryApp/keystr 2>/dev/null"
#define CMD_EFS_HIDDEN_MENU_ON    "echo ON > /efs/carrier/HiddenMenu 2>/dev/null"
#define CMD_EFS_HIDDEN_MENU_OFF   "echo OFF > /efs/carrier/HiddenMenu 2>/dev/null"

/* Samsung Retail-Mode (LDU) activation — [RE] from FotaAgent dump */
#define CMD_RETAIL_ACTIVATE_FMT \
    "am start -n com.samsung.rms.retailagent.global/com.samsung.sea.rm.ui.IntentActivationActivity " \
    "--es source com.sdet.fdmagent --es password %s --es region %s " \
    "--es subsidiary %s --es channel %s --es sub_channel %s > /dev/null 2>&1"


/* =========================================================================
 * 2. AT CORE / HANDSHAKE [STD]
 * ========================================================================= */
#define AT_CMD_TEST                 "AT\r\n"
#define AT_CMD_ECHO_OFF             "ATE0\r\n"
#define AT_CMD_RESET                "ATZ\r\n"
#define AT_CMD_IDENTITY             "ATI\r\n"              /* [OBS 286x] */
#define AT_CMD_CLAC                 "AT+CLAC\r\n"          /* list all commands */


/* =========================================================================
 * 3. TELEMETRY & DEVICE IDENTITY
 * ========================================================================= */
#define AT_CMD_DEVCONINFO           "AT+DEVCONINFO\r\n"     /* [OBS 35x] master identity read */
#define AT_CMD_VERSNAME_SW          "AT+VERSNAME=1,9,0\r\n" /* [OBS] full SW version */
#define AT_CMD_VERSNAME_PDA         "AT+VERSNAME=1,7,0\r\n" /* [OBS] PDA / build id */
#define AT_CMD_VERSNAME_CSC         "AT+VERSNAME=1,8,0\r\n" /* [OBS] CSC version */
#define AT_CMD_VERSNAME_CP          "AT+VERSNAME=1,0,0\r\n" /* [OBS] baseband/CP version */
#define AT_CMD_SERIALNO             "AT+SERIALNO=1,0\r\n"   /* [OBS] serial from NVRAM */
#define AT_CMD_IMEITEST             "AT+IMEITEST=1,0\r\n"   /* [OBS] IMEI from radio */
#define AT_CMD_IMEI_QUERY_LEGACY    "+IMEITEST:1,\r\n"      /* [RE] legacy probe form */
#define AT_CMD_CIMI                 "AT+CIMI\r\n"           /* [OBS 34x] IMSI (MCC+MNC+MSIN) */
#define AT_CMD_BATTEST              "AT+BATTEST=1,0\r\n"    /* [OBS] battery diagnostics */
#define AT_CMD_SYSSCOPE             "AT+SYSSCOPE=1,0\r\n"   /* [OBS] Knox/security status */
#define AT_CMD_MSLCHECK             "AT+MSLCHECK\r\n"       /* [RE] Master Subsidy Lock status */
#define AT_CMD_DUMPENABLE           "AT+DUMPENABLE=1,1\r\n" /* [RE] modem ramdump logging */
#define AT_CMD_KSTRINGB             "AT+KSTRINGB=0,3\r\n"   /* [OBS 17x] keystring injection */
#define AT_CMD_SVCIFPGM_1           "AT+SVCIFPGM=1,1\r\n"   /* [OBS 16x] read mobile info */
#define AT_CMD_SVCIFPGM_4           "AT+SVCIFPGM=1,4\r\n"   /* [RE] SIM restriction check */
#define AT_CMD_CSQ                  "AT+CSQ\r\n"            /* [STD/OBS 38x] signal quality */
#define AT_CMD_COPS_QUERY           "AT+COPS?\r\n"          /* [STD] operator selection */
#define AT_CMD_COPS_LIST            "AT+COPS=?\r\n"         /* [STD] list operators w/ MCC-MNC */
#define AT_CMD_CREG_QUERY           "AT+CREG?\r\n"          /* [STD] registration status */
#define AT_CMD_CBC                  "AT+CBC\r\n"            /* [STD/OBS] battery level */
#define AT_CMD_CGSN                 "AT+CGSN\r\n"           /* [STD/OBS] serial number */


/* =========================================================================
 * 4. SIM STATUS & 3GPP NETWORK PERSONALIZATION LOCKS
 * ========================================================================= */
#define AT_CMD_SIM_STATUS           "AT+CPIN?\r\n"                    /* [STD/OBS 29x] */
#define AT_CMD_QUERY_NETWORK_LOCK   "AT+CLCK=\"PN\",2\r\n"             /* [OBS] net lock query */
#define AT_CMD_QUERY_SUBSET_LOCK    "AT+CLCK=\"PU\",2\r\n"             /* [OBS] net-subset lock */
#define AT_CMD_QUERY_SP_LOCK        "AT+CLCK=\"PP\",2\r\n"             /* [OBS] provider lock */
#define AT_CMD_QUERY_CORP_LOCK      "AT+CLCK=\"PC\",2\r\n"             /* [OBS] corporate lock */
#define AT_CMD_QUERY_SIM_LOCK       "AT+CLCK=\"SC\",2\r\n"             /* [OBS] SIM (PIN) lock */
#define AT_CMD_ENABLE_NET_LOCK_FMT  "AT+CLCK=\"PN\",1,\"%s\"\r\n"      /* [STD] enable net lock */
#define AT_CMD_LOCKINFO             "AT+LOCKINFO=2,0,00,N,N,N,N\r\n"  /* [OBS 15x] lock info dump */
#define AT_CMD_DETALOCK_ZERO        "AT+DETALOCK=2,00000000,00000000,00000000,00000000,00000000,00000000\r\n" /* [OBS] per-lock control */
#define AT_CMD_LVOFLOCK             "AT+LVOFLOCK=1,0\r\n"             /* [OBS] LV lock off */


/* =========================================================================
 * 5. SAMSUNG CSC EXECUTION CHAINS (verified via Alephgsm + field captures)
 *     Canonical sequence: SWATD=0 -> ACTIVATE -> SWATD=1 -> PRECONFG=2,<CSC>
 *     -> (verify PRECONFG=1,0) -> CFUN=1,1
 * ========================================================================= */
#define AT_CMD_PRECONFG_READ        "AT+PRECONFG=1,0\r\n"          /* [OBS] read current CSC */
#define AT_CMD_PRECONFG_WRITE_FMT   "AT+PRECONFG=2,%s\r\n"         /* [OBS] write new CSC */
#define AT_CMD_SWATD_0              "AT+SWATD=0\r\n"               /* [OBS 26x] svc mode open */
#define AT_CMD_SWATD_1              "AT+SWATD=1\r\n"               /* [OBS 37x] svc mode lock */
#define AT_CMD_ACTIVATE             "AT+ACTIVATE=0,0,0\r\n"        /* [OBS 35x] activate channels */
#define AT_CMD_RFBYCODE_FMT         "AT+RFBYCODE=2,0,%s\r\n"       /* [OBS] modern CSC+RF rewrite */
#define AT_CMD_RFBYCODE_21_FMT      "AT+RFBYCODE=2,1,%s\r\n"       /* [OBS] alternate variant */
#define AT_CMD_RFBYCODE_QUERY       "AT+RFBYCODE=1,1,0\r\n"        /* [OBS] query current */


/* =========================================================================
 * 6. SECURITY CHALLENGE & AUTHENTICATION SEEDS
 * ========================================================================= */
#define AT_CMD_AKSEEDNO_REQ         "AT+AKSEEDNO=1,0\r\n"          /* [OBS 4x] request seed */
#define AT_CMD_AKSEEDNO_RESP_FMT    "AT+AKSEEDNO=0,%04x\r\n"       /* [RE] KEY-Tool agent form */
#define AT_CMD_BLOBSIGN             "AT+BLOBSIGN\r\n"              /* [OBS] signed blob upload */
#define AT_CMD_BLOBSIGN_1_1_0       "AT+BLOBSIGN=1,1,0\r\n"        /* [OBS 8x] */
#define AT_CMD_IMEISIGN             "AT+IMEISIGN\r\n"              /* [RE] IMEI cert upload */
#define AT_CMD_IMEISIGN_1_1_0       "AT+IMEISIGN=1,1,0\r\n"        /* [OBS 6x] */
#define AT_CMD_IMEISIGN_2_1_FMT     "AT+IMEISIGN=2,1,%s\r\n"       /* [OBS] with hex blob */
#define AT_CMD_MSLSECUR_1_0         "AT+MSLSECUR=1,0\r\n"          /* [OBS 6x] MSL unlock state */
#define AT_CMD_MSLSECUR_2_FMT       "AT+MSLSECUR=2,%s\r\n"         /* [OBS] MSL write */
#define AT_CMD_MEIDAUTH_FMT         "AT+MEIDAUTH=%s\r\n"           /* [OBS 12x] MEID auth */
#define AT_CMD_FRPUNLCK             "AT+FRPUNLCK=3,0,0\r\n"        /* [OBS 2x] FRP unlock */
#define AT_CMD_SUDDLMOD             "AT+SUDDLMOD=0,0\r\n"          /* [OBS 5x] SUD/DL mode */
#define AT_CMD_ETMOSK_0             "AT+ETMOSK=0\r\n"              /* [OBS 3x] ETM open mask */
#define AT_CMD_ETMOSLB_QUERY        "AT+ETMOSLB=2\r\n"             /* [OBS 3x] ETM sub-lock */
#define AT_CMD_ETMOSLB_9_FMT        "AT+ETMOSLB=9,%s,%s\r\n"       /* [OBS 4x] ETM sub-lock write */
#define AT_CMD_LVLOCK_FMT           "AT+LVLOCK=%s\r\n"             /* [OBS] LV lock write */
#define AT_CMD_Q8HVPR               "AT+Q8HVPR\r\n"                /* [OBS 16x] security probe */
#define AT_CMD_PARALLEL             "AT+PARALLEL=2,0,00000\r\n"    /* [OBS] parallel channel */
#define AT_CMD_REACTIVE             "AT+REACTIVE=1,0,0\r\n"        /* [OBS] reactive state */
#define AT_CMD_SIMLGDATA_QUERY       "AT+SIMLGDATA\r\n"             /* [OBS 8x] LG SIM-lock data */
#define AT_CMD_SIMLGDATA_FMT        "AT+SIMLGDATA=%s\r\n"          /* [OBS] LG SIM-lock write */
#define AT_CMD_POWRESET             "AT+POWRESET=0,0\r\n"          /* [OBS 14x] power reset */


/* =========================================================================
 * 7. SPRINT / BOOST UNLOCK & CARRIER RELOCK (RIL internal dispatcher cmds)
 * ========================================================================= */
#define AT_CMD_UNLOCKSPR_OLD        "AT+UNLOCKSPR=OLD\r\n"         /* [OBS 2x] SPR lock release */
#define AT_CMD_SIMLOCKU_RESET       "AT+SIMLOCKU=0,0,0\r\n"        /* [OBS 2x] SIM-lock reset */

/* Original-tool internal session commands (dispatcher prefix-matched) [RE]:
 *   any input starting with SETSPR (6) / GETSPR (6) / SRELOCK (7) is accepted.
 */
#define RIL_CMD_SETSPR              "SETSPR"     /* write SPR carrier profile (MCC/MNC list) */
#define RIL_CMD_GETSPR              "GETSPR"     /* read  SPR carrier profile */
#define RIL_CMD_SRELOCK             "SRELOCK"    /* carrier relock (practically SRELOCK1) */
#define RIL_CMD_SRELOCK1            "SRELOCK1"


/* =========================================================================
 * 8. RADIO & POWER CONTROLS [STD/3GPP TS 27.007]
 * ========================================================================= */
#define AT_CMD_REBOOT_RADIO         "AT+CFUN=1,1\r\n"   /* soft radio reboot */
#define AT_CMD_RADIO_OFF            "AT+CFUN=0\r\n"     /* airplane (radio off) */
#define AT_CMD_RADIO_ON             "AT+CFUN=1\r\n"     /* radio on */
#define AT_CMD_RADIO_NO_RF          "AT+CFUN=4\r\n"     /* disable RF TX/RX only */
#define AT_CMD_MODEM_FACTORY_RESET  "AT+FACTORST=0,0\r\n" /* [OBS 5x] NV factory reset */
#define AT_CMD_SWRESET              "AT+SWRESET\r\n"    /* full phone reboot */
#define AT_CMD_REBOOT               "AT+REBOOT\r\n"     /* alias of SWRESET [RE] */
#define AT_CMD_NBOOT                "AT+NBOOT\r\n"      /* [OBS 4x] normal boot */


/* =========================================================================
 * 9. QUALCOMM BASEBAND COMMANDS [STD-QC]
 * ========================================================================= */
#define AT_CMD_QCRMCALL             "AT$QCRMCALL=1,1\r\n"
#define AT_CMD_QCPWRDN              "AT$QCPWRDN\r\n"
#define AT_CMD_QENG_SERVING         "AT+QENG=\"servingcell\"\r\n"
#define AT_CMD_QCN                  "AT+QCN\r\n"
#define AT_CMD_SPDIAG               "AT+SPDIAG=\r\n"    /* [OBS 36x] diag mode */


/* =========================================================================
 * 10. MODEM CHANNELS / DEVICE NODES (probe order = list order)
 * ========================================================================= */
static const char * const ALL_MODEM_CHANNELS[] = {
    /* Samsung RIL sockets (parcel channel) */
    "/dev/socket/rild",
    "/dev/socket/rild-debug",
    "/dev/socket/rild-oem",
    "/dev/socket/rild2",
    "/dev/socket/qmux_radio/qmux_client_socket0",
    /* Samsung Shannon / Exynos IPC nodes */
    "/dev/umts_ipc0",
    "/dev/umts_ipc1",
    "/dev/umts_rfs0",
    "/dev/umts_boot0",
    "/dev/diag",
    /* AT text channels */
    "/dev/umts_atc0",
    "/dev/umts_atc1",
    "/dev/umts_at0",
    "/dev/at_distributor",
    "/dev/at_distributor0",
    /* Qualcomm SMD channels */
    "/dev/smd11",
    "/dev/smd0",
    "/dev/smd7",
    /* Virtual serial */
    "/dev/ttyGS0",
    "/dev/ttyACM0",
    "/dev/ttyUSB0",
    0
};

static const char * const ALL_RILD_SOCKETS[] = {
    "/dev/socket/rild",
    "/dev/socket/rild-oem",
    "/dev/socket/rild-debug",
    "/dev/socket/rild2",
    "/dev/socket/qmux_radio/qmux_client_socket0",
    0
};

static const char * const ALL_IPC_NODES[] = {
    "/dev/umts_ipc0",
    "/dev/umts_ipc1",
    "/dev/umts_rfs0",
    "/dev/diag",
    0
};

static const char * const ALL_AT_NODES[] = {
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
    0
};

/* libsecril-client.so — strongest Samsung channel [RE] */
#define SECRIL_LIBRARY_PATH1  "libsecril-client.so"
#define SECRIL_LIBRARY_PATH2  "/system/lib64/libsecril-client.so"
#define SECRIL_LIBRARY_PATH3  "/vendor/lib64/libsecril-client.so"
#define SECRIL_SEND_RECV_SYM  "send_recv_cmd"


/* =========================================================================
 * 11. SYSTEM PROPERTIES
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
    "gsm.sim.operator.alpha",
    "gsm.operator.numeric",
    "gsm.operator.alpha",
    "gsm.operator.iso-country",
    "gsm.network.type",
    "gsm.version.baseband",
    "ril.serialnumber",
    "ril.approved_code",
    "ril.product_code",
    "persist.sys.usb.config",
    "persist.sys.omc_sales_code",
    "persist.sys.sales_code",
    "persist.sys.prev_salescode",
    "ro.boot.carrierid",
    "knox.kg.state",
    "sys.usb.config",
    0
};


/* =========================================================================
 * 12. TARGET PACKAGES & ACTIVITIES
 * ========================================================================= */
#define PKG_AUDIO_DEBUG         "com.sec.android.app.audiocoredebug"
#define ACT_AUDIO_DEBUG_SHELL   "com.sec.android.app.audiocoredebug.AudioAdbShellActivity"
#define PKG_HIDDEN_MENU         "com.sec.hiddenmenu"
#define ACT_IOT_HIDDEN_MENU     "com.sec.hiddenmenu.IOTHiddenMenu"
#define PKG_FACTORY_TEST        "com.samsung.android.FactoryTestLauncher"
#define PKG_RETAIL_MODE         "com.samsung.rms.retailagent.global"
#define PKG_RETAIL_MODE_LITE    "com.samsung.rms.retailagent.global_lite"
#define PKG_SKT_UNLOCK          "com.skatd.unlock"
#define PKG_FOTA_AGENT          "com.sdet.fotaagent"
#define PKG_OMADM_SPR           "com.sec.omadmspr"
#define PKG_AUTOPRECONFIG       "com.sec.android.AutoPreconfig"
#define PKG_CARRIER_CODE_CHANGER "com.sec.android.CarrierCodeChanger"
#define PKG_KEYTOOL_SRV4        "KEYTool.SRV4"
#define PKG_REPAIRCAL           "com.samsung.android.app.repaircal"
#define PKG_MOBILEDOCTOR        "com.samsung.android.app.mobiledoctor"


/* =========================================================================
 * 13. BRIDGE INTERACTIVE PROTOCOL (stable C#<->bridge contract — DO NOT BREAK)
 * ========================================================================= */
#define PROTO_PREFIX_CSC        "CSC:"
#define PROTO_PREFIX_AT         "AT:"
#define PROTO_PREFIX_HEX        "HEX:"
#define PROTO_PREFIX_BOOST      "BOOST:"
#define PROTO_PREFIX_RELOCK     "RELOCK:"
#define PROTO_PREFIX_RADIO      "RADIO:"
#define PROTO_PREFIX_AUTH       "AUTH:"
#define PROTO_PREFIX_MODE       "MODE:"

#define PROTO_CMD_BOOST_UNLOCK  "BOOST"
#define PROTO_CMD_SCAN          "SCAN"
#define PROTO_CMD_STATUS        "STATUS"
#define PROTO_CMD_LOCKS         "LOCKS"
#define PROTO_CMD_SIMLOCK       "SIMLOCK"
#define PROTO_CMD_DEVINFO       "DEVINFO"
#define PROTO_CMD_TELEMETRY     "TELEMETRY"
#define PROTO_CMD_AKSEED        "AKSEED"
#define PROTO_CMD_RECONNECT     "RECONNECT"
#define PROTO_CMD_HELP          "HELP"
#define PROTO_CMD_INFO          "INFO"
#define PROTO_CMD_VERSION       "VERSION"
#define PROTO_CMD_PROPS         "PROPS"
#define PROTO_CMD_EXIT          "EXIT"
#define PROTO_CMD_QUIT          "QUIT"

/* machine-readable markers */
#define MARK_OK_HEX_FMT         ".OK..%s#\n"
#define MARK_OK_EMPTY           ".OK..#\n"
#define MARK_OK_CSC             ".OK_CSC..\n"
#define MARK_OK_AUTH            ".OK_AUTH..\n"
#define MARK_OK_BOOST           ".OK_BOOST..\n"
#define MARK_OK_RELOCK          ".OK_RELOCK..\n"
#define MARK_OK_RADIO           ".OK_RADIO..\n"
#define MARK_ERR_IPC            ".ERR_IPC..\n"
#define MARK_ERR_SYNTAX         ".ERR_SYNTAX..\n"
#define MARK_ERR_HEX_SYNTAX     ".ERR_HEX_SYNTAX..\n"
#define MARK_ERR_CSC_INVALID    ".ERR_CSC_INVALID..\n"
#define MARK_ERR_RELOCK_INVALID ".ERR_RELOCK_INVALID..\n"
#define MARK_ERR_UNKNOWN_ACTION ".ERR_UNKNOWN_ACTION..\n"
#define MARK_ERR_AT_TIMEOUT     ".ERR_AT_TIMEOUT..\n"
#define MARK_TIMEOUT            ".TIMEOUT..\n"
#define MARK_CH_FMT             ".CH=%s..\n"
#define PROMPT                  "<< "

#define TELEMETRY_FORMAT_STRING \
    "INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s\n"
#define SIMLOCK_FORMAT_STRING \
    "SIMLOCK|CPIN=%s|PN=%s|PU=%s|PP=%s|PC=%s|SC=%s\n"
#define DEVINFO_FORMAT_STRING \
    "DEVINFO|MODEL=%s|BRAND=%s|CSC=%s|AND=%s|PATCH=%s|BUILD=%s|SN=%s|" \
    "PROD=%s|VERS=%s|IMEI=%s|IMSI=%s|DEVCON=%s\n"
#define AKSEED_FORMAT_STRING    "AKSEED|%s\n"
#define VERSION_FORMAT_STRING   "VERSION|NAME=GsmTools AT Bridge|VER=4.0|CH=%s|MODE=%d\n"


/* =========================================================================
 * 14. SAMSUNG IPC FRAME TEMPLATES (the 5-packet CSC engine + relock)
 *     Format: [Main=0x06][Sub=0x01][Type 2B BE][payload]
 *     The type field doubles as length in captured traces — kept verbatim
 *     from real captures; do NOT reinterpret as little-endian.
 * ========================================================================= */
#define IPC_CSC_PKT1_FMT  "0601000E0000%s0000%s"          /* 0601000E0000<CSC>0000<CSC>  */
#define IPC_CSC_PKT2_FMT  "060100090000%s"                /* 060100090000<CSC>           */
#define IPC_CSC_PKT3_FMT  "060100120000%s0000%s%s"        /* 060100120000<CSC>0000<CSC><CSC> */
#define IPC_CSC_PKT4_STR  "060100120000"                  /* 060100120000                */
#define IPC_CSC_PKT5_FMT  "060100090000%s"                /* 060100090000<CSC>           */
#define IPC_RELOCK_FMT    "0601%04X0000%s"                /* 0601<LEN:4HEX>0000<BCD>     */
#define IPC_TYPE_CSC_MAIN 0x06
#define IPC_TYPE_CSC_SUB  0x01

/* rild OEM hook parcel: [4B BE total][4B req=59][4B token++][4B dlen][ipc][pad4] */
#define RIL_REQUEST_OEM_HOOK_RAW 59
#define RELAY_PORT_DEFAULT        2600
#define RELAY_ESCALATE_CMD        "setprop ctl.stop ssud; id\n"


#ifdef __cplusplus
}
#endif

#endif /* ALL_GSM_COMMANDS_H */
