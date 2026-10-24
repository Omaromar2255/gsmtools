# مكتبات وملفات RIL — `libril.so` والمشابهة: متى وكيف تُستخدم

**التاريخ:** 24/10/2026  
**النطاق:** كل مكتبة/ملف/سوكيت يستخدمه نظام الأدوات (AT / at-bridge / KEY-Tool / Q-Unlocker / SimUnlockerPro) للاتصال بمودم سامسونج، مع إثبات المصدر لكل عنصر، وتحديد **متى** و**كيف** يُستخدم.

> **الملف المرافق:** `LIBRIL_COMMANDS.txt` (قائمة مسطحة قابلة للـgrep) + `extract_ril_libraries.py` (سكربت الاستخراج) + `AT_Source_Extract\KEYTool_RIL_agent_32bit.elf` (العميل المدمج المستخرج).

---

## 0) الخلاصة التنفيذية

1. **`libril.so` في هذا السياق ليست مكتبة AOSP** — هي **مكتبة RIL خاصة بالأداة** مدمجة داخل APK:
   - `lib/arm64-v8a/libril.so` (351,901 بايت) — داخل نسختي APK في `KEY-Tool.DMP`.
   - `lib/armeabi-v7a/libril.so` — داخل 3 نسخ APK في `SimUnlockerPro.DMP` (موقعة بتوقيع سامسونج `ANDROID.RSAPK`).
   - تُحمَّل عبر `System.loadLibrary("ril")` من `classes.dex` بعد تثبيت الـAPK بـ`pm install -r -g`.
2. **المكتبة الرسمية للاتصال بالمودم هي `libsecril-client.so`** (سامسونج) — رمزها `send_recv_cmd`:
   - مصرّحة في `DT_NEEDED` لعميل الأداة المدمج (ELF 32-بت داخل `KEY-Tool.DMP`).
   - `AT` (HUNTER-UNLOCKER) يبحث عنها برسائل صريحة: `Found 'send_recv_cmd' in %s.`
   - `at-bridge` لديه وضع كامل لها: `CH_SECRIL_LIB /* libsecril-client.so (الأقوى والأضمن لسامسونج) */`.
3. **`AT` لا يستورد `dlopen`/`dlsym` إطلاقاً** (112 PLT + 113 dynsym موثقة — لا وجود لهما). يبني أوامر `echo '<cmd>' | toybox nc 127.0.0.1 2600` عبر `popen` (8 مواقع) و`system` (77 موقعاً) ويقرأ الردود عبر `fgets`/`getline`.
4. **4 قنوات اتصال بالمودم** (موثقة في at-bridge):
   | الأولوية | القناة | المسار |
   |---|---|---|
   | 1 | IPC خام | `/dev/umts_ipc0` → `umts_ipc1` → `umts_rfs0` → `/dev/diag` |
   | 2 | سوكيت rild | `/dev/socket/rild` → `rild-oem` → `rild-debug` → `rild2` → `qmux` |
   | 3 | AT نصي | `/dev/umts_atc0` → `atc1` → `at0` → `at_distributor` → `smd11` → `smd0/7` → TTY |
   | — | مكتبة secril | `libsecril-client.so` عبر `send_recv_cmd` (أقوى قناة) |
5. **التسلسل الكامل** (مرصود): دفع `athun` → `chmod 0777` → `AT+AKSEEDNO` (مصادقة) → `GETSPR`/`SETSPR`/`SRELOCK1` → `rm athun`.

---

## 1) خريطة الملفات والمكتبات (مع الدليل)

### 1.1 مكتبات RIL

| الملف | الدليل | الدور |
|---|---|---|
| `lib/arm64-v8a/libril.so` | KEY-Tool.DMP: ZIP CD @`0x2fdacc3` (csize 343,768 → **usize 351,901**, CRC `0x3b4f66b3`) + CD @`0x3cb9ff1` (csize 338,257 → **usize 346,333**, CRC `0xbf9fa74`) + نسختا local header @`0x2f8517d`/`0x3c65a2d` | **مكتبة RIL خاصة بالأداة** داخل APK arm64 |
| `lib/armeabi-v7a/libril.so` | SimUnlockerPro.DMP @`0x3ab1989`, `0x3b09a9f` (مع `META-INF/ANDROID.RSAPK` + `ANDROID.SFPK` + `MANIFEST.MFPK` — **توقيع سامسونج**)، `0x52b2ebb` | نسخة armeabi-v7a |
| `libsecril-client.so` | KEY-Tool.DMP dynstr @`0x68a2222` (DT_NEEDED للعميل المدمج) + رسائل AT (`send_recv_cmd`) + at-bridge `CH_SECRIL_LIB` | **عميل RIL الرسمي لسامسونج** — يصدّر `send_recv_cmd` |
| `libsec-ril.so` | KEY-Tool.DMP dynstr @`0x68a2236` (DT_NEEDED) | مكتبة RIL الخاصة بمودم سامسونج |
| `libplayservice.so` | KEY-Tool.DMP @`0x16ba75e` — النص: `libplayservice.so: Permission denied` | مكتبة مساعدة لوكيل KEY-Tool — يفشل تحميلها في سياقات غير مسموحة |
| `libandroid_runtime.so` + `libnativehelper.so` + `libbinder.so` + `libutils.so` | KEY-Tool.DMP NEEDED list @`0x68a2222` (نفس العميل) | مكتبات أندرويد JNI/ART — تُحمّل تلقائياً مع العميل |

### 1.2 عقد المودم (Device Nodes)

| المسار | النوع | at-bridge (سطر) |
|---|---|---|
| `/dev/umts_ipc0` | IPC خام | `IPC_NODES[0]` (سطر 130) |
| `/dev/umts_ipc1` | IPC خام | سطر 131 |
| `/dev/umts_rfs0` | IPC خام | سطر 132 |
| `/dev/diag` | Diag | سطر 133 |
| `/dev/umts_atc0` | AT نصي | `AT_NODES[0]` (سطر 138) |
| `/dev/umts_atc1` | AT نصي | سطر 139 |
| `/dev/umts_at0` | AT نصي | سطر 140 |
| `/dev/at_distributor` | AT نصي | سطر 141 |
| `/dev/at_distributor0` | AT نصي | سطر 142 |
| `/dev/smd11` | SMD (Qualcomm) | سطر 143 |
| `/dev/smd0` | SMD | سطر 144 |
| `/dev/smd7` | SMD | سطر 145 |
| `/dev/ttyGS0` | USB TTY | سطر 146 |
| `/dev/ttyACM0` | USB CDC | سطر 147 |
| `/dev/ttyUSB0` | USB | سطر 148 |

### 1.3 سوكيتات rild

| المسار | at-bridge (سطر) |
|---|---|
| `/dev/socket/rild` | `RILD_SOCKETS[0]` (سطر 153) |
| `/dev/socket/rild-oem` | سطر 154 |
| `/dev/socket/rild-debug` | سطر 155 |
| `/dev/socket/rild2` | سطر 156 |
| `/dev/socket/qmux_radio/qmux_client_socket0` | سطر 157 |

### 1.4 وحدات RIL لكل معماريات (الأدوات)

| الوحدة | المصدر | المعمارية |
|---|---|---|
| `HUN_RIL_SAM2025_06_ALL_noapparm64v8a` | Q-Unlocker — `region_results.txt` | arm64-v8a |
| `HUN_RIL_SAM2025_06_ALL_noapparmiv7a` | Q-Unlocker — نفس الملف | armeabi-v7a |
| `Hunter_Ril_Sam_spr_2020armiv7a` | Q-Unlocker — نفس الملف | armeabi-v7a (SPR) |
| `ATRIL` / `ATRIL64` / `NoAppRil` | SimUnlockerPro.DMP @`0x17920fa5` (UTF-16) | أدوات RIL على الهاتف |
| `RIL202310` / `RIL202409` / `JDMRIL2023` | SimUnlockerPro.DMP @`0x2015ecf` | وحدات RIL سنوية (dlopen) |
| `AT_SamRilTool_V3_arm64_KT` / `DAT_SamRilTool_V3_KT` | KEY-Tool resource list | أدوات RIL (KEY-Tool) |
| `lib/arm64-v8a/libril.so` (داخل APK) | KEY-Tool.DMP (§1.1) | arm64-v8a |

### 1.5 ملفات مرافقة (APK/DEX)

| الملف | الدليل |
|---|---|
| `classes.dex` | KEY-Tool.DMP @`0x6c0fbf1` (UTF-16) + `Push classes.dex File Error` |
| `sim_android_1.5.apk` | SimUnlockerPro.DMP @`0x17920fa5` |
| `repaircal.apk` | نفس الموقع |
| `Ekbase.apk` | نفس الموقع |
| `DM.apk` | نفس الموقع |
| `Unlock32` | نفس الموقع |
| `RetailExploit` | نفس الموقع |
| `GPT apk` / `readcode2` | نفس الموقع |
| `MCCMNC.txt` | KEY-Tool.DMP @`0x16c62c5` |
| `KMB64` | KEY-Tool resource list |

---

## 2) `libril.so` — مكتبة الأداة داخل APK

### 2.1 ما هي؟

**ليست** مكتبة AOSP `libril.so` (التي تُستخدم داخل `rild`). هي مكتبة **خاصة بالأداة** مبنية بلغة native (ARM) ومدمجة داخل APK باسم `lib/<abi>/libril.so` — التسمية القياسية لـJNI:

```text
System.loadLibrary("ril")  →  يبحث أندرويد عن lib/<abi>/libril.so داخل APK
```

### 2.2 كيف تُثبَّت وتُحمَّل؟

**التسلسل المرصود (من Q-Unlocker / KEY-Tool):**

```text
1) adb push <apk> /data/local/tmp/...         (دفع الـAPK)
2) adb install -r -g <apk>                     (تثبيت APK مع الأذونات)
3) التطبيق يبدأ → System.loadLibrary("ril")     (تحميل libril.so)
4) classes.dex يستدعي الدوال الأصلية            (JNI)
```

**الدليل المرصود:**
- `adb.exe install -r -g "." ./QTMPX142_...` — من `relock_extracted_results.txt`.
- `Push classes.dex File Error` — من `keytool_exploit_flow.txt`.
- `pm path KEYTool.SRV4` — أمر تشخيصي مرصود (PowerShell session).

### 2.3 متى تُستخدم؟

| الحدث | الاستخدام |
|---|---|
| تثبيت APK جديد | `libril.so` تُنسخ إلى `/data/app/<pkg>/lib/<abi>/` |
| تشغيل التطبيق (KEYTool.SRV4 أو الوكيل) | `System.loadLibrary("ril")` يحمّلها في عملية التطبيق |
| عملية MCC/MNC | `Reading All MCC+MNC Available :` → المكتبة تقرأ SIMLock Blob |
| إعادة القفل | `Writing SIMLock Blob :` + `Writing Shared2 Blob :` → المكتبة تكتب |
| فشل التحميل | `libplayservice.so: Permission denied` (في سياق مغاير) |

### 2.4 نسخ APK داخل KEY-Tool.DMP

| CD offset | csize | usize | CRC |
|---|---:|---:|---|
| `0x2fdacc3` | 343,768 | 351,901 | `0x3b4f66b3` |
| `0x3cb9ff1` | 338,257 | 346,333 | `0x0bf9fa74` |

نسختان مختلفتان (حجمان وCRC مختلفان) — بناءان مختلفان من نفس المكتبة.

---

## 3) `libsecril-client.so` و `libsec-ril.so` — عميل سامسونج الرسمي

### 3.1 `libsecril-client.so`

**الرمز المصدَّر:** `send_recv_cmd` — دالة إرسال/استقبال إطار IPC ثنائي من/إلى المودم.

**الاستخدام في الأدوات:**

| المكوّن | الاستخدام | الدليل |
|---|---|---|
| **AT** (HUNTER-UNLOCKER) | يبحث عن `send_recv_cmd` بعد تحميل المكتبة | رسائل مفكوكة (§6) |
| **at-bridge** | `CH_SECRIL_LIB` — «الأقوى والأضمن لسامسونج» | `at_bridge.c` سطر 35 |
| **عميل KEY-Tool المدمج** | مرتبط بها في `DT_NEEDED` (تحميل مباشر عند بدء العملية) | `KEYTool_RIL_agent_32bit.elf` |
| **KEY-Tool DMP** | نص `libsecril-client.so` في dynstr | @`0x68a2222` |

**متى تُستخدم؟**
- **عند بدء كل عملية RIL**: تحميل المكتبة → `Connect_RIL()` → `send_recv_cmd(frame)` → استقبال الرد → `Disconnect_RIL()`.
- **قبل** `SETSPR`/`SRELOCK1`: لقراءة SPR الحالي أولاً.
- **بعد** أي كتابة: للتحقق من النتيجة (`Result : OK`).

**كيف تُستخدم؟**

```c
// نمط الاستدعاء (مستنتج من at-bridge CH_SECRIL_LIB + رسائل AT):
void *lib = dlopen("libsecril-client.so", RTLD_NOW);
send_recv_cmd_t fn = dlsym(lib, "send_recv_cmd");
// ← AT يطبع: "[i] Found 'send_recv_cmd' in %s."
int code = fn(request, &response, &resp_len);
// ← AT يطبع: "[-] Library %s did not return a valid response (code: %d, len: %d)."
```

> **ملاحظة معمارية:** AT لا يستورد `dlopen`/`dlsym` — يُرجّح أن التحميل الفعلي يتم في **مكوّن مرافق** (العميل المدمج الذي يستوردهما) وأن AT يدير الجلسة ويستقبل النتائج فقط.

### 3.2 `libsec-ril.so`

مكتبة RIL الخاصة بمودم سامسونج (vendor). مرتبطة أيضاً في `DT_NEEDED` لعميل KEY-Tool — تُحمَّل تلقائياً معه (وليست هدفاً مباشراً للأداة).

---

## 4) عميل KEY-Tool RIL المدمج (ELF 32-بت)

### 4.1 البنية

| الخاصية | القيمة |
|---|---|
| **الموقع في DMP** | `KEY-Tool.DMP @ 0x689f91e` |
| **المعيار** | ELF 32-bit ARM (ET_DYN, machine 0x28) |
| **الحجم** | 45,708 بايت (`0xB28C` — PT_LOAD filesz) |
| **المستخرج** | `AT_Source_Extract\KEYTool_RIL_agent_32bit.elf` |

### 4.2 المكتبات المطلوبة (DT_NEEDED)

```text
liblog.so
libandroid.so
libutils.so
libcutils.so
libbinder.so
libnativehelper.so        ← JNI
libc++.so
libandroid_runtime.so     ← ART runtime
libz.so
libsecril-client.so       ← ★ عميل RIL سامسونج
libsec-ril.so             ← ★ مكتبة RIL للمودم
libc.so
libstdc++.so
libm.so
libdl.so
```

### 4.3 الواردات (Imports) الرئيسية

| الدالة | الاستخدام |
|---|---|
| `dlopen` | تحميل مكتبات ديناميكياً (libsecril-client.so أو غيرها) |
| `dlsym` | البحث عن رموز (`send_recv_cmd`) |
| `system` | تنفيذ أوامر shell |
| `execv` | استبدال العملية |
| `setuid` / `getuid` | فحص/تغيير UID |
| `__system_property_get` | قراءة خصائص النظام مباشرة |
| `fgets` / `scanf` | قراءة الإدخال |

### 4.4 الصادرات (Exports) — محرك المصادقة + RIL

| الدالة | الحجم | الوظيفة |
|---|---:|---|
| `main` | 504 بايت | نقطة الدخول |
| `onclick()` | 5,328 بايت | **حلقة الأوامر الرئيسية** (مستقبل أوامر الجلسة) |
| `AES()` | 1,876 بايت | تشفير AES-128 |
| `RSA_verify()` | 4,364 بايت | التحقق من توقيع RSA |
| `Encrypt_Massagekeysha256()` | — | تشفير الرسائل (مفتاح SHA256) |
| `Encrypt_Massagekey()` | — | تشفير الرسائل |
| `Encrypt_Massageadbidsha256()` | — | تشفير ADB ID |
| `sha256()` | 192 بايت | هاش SHA-256 |
| `Get_Build()` | 1,388 بايت | قراءة معلومات البناء |
| `hex2stream()` / `stream2hex()` | — | تحويل hex↔binary |
| `RM_TOOL()` | 156 بايت | إزالة/تنظيف |
| `strsub()` / `concat()` | — | أدوات نصية |
| AES tables: `Rcon`, `Key`, `RoundKey`, `state`, `Nk`, `Nr` | — | جداول AES-128 |

> **الدلالة:** هذا هو **محرك المصادقة** (AES + RSA + SHA256) + **عميل RIL** (يرتبط مباشرة بـ`libsecril-client.so`). يُحمَّل عند بدء عملية الوكيل.

---

## 5) `rild` وسوكيتاته + عقد `/dev` — قنوات at-bridge

### 5.1 أوضاع القنوات الأربعة

```c
typedef enum {
    CH_NONE = 0,
    CH_SECRIL_LIB,      /* libsecril-client.so — الأقوى والأضمن لسامسونج */
    CH_RILD_PARCEL,     /* /dev/socket/rild مع تغليف Parcel قياسي */
    CH_DEV_IPC_RAW,     /* /dev/umts_ipc0 أو /dev/umts_rfs0 */
    CH_DEV_AT_TTY       /* /dev/umts_atc0 أو /dev/smd11 — أوامر نصية فقط */
} channel_mode_t;
```

### 5.2 أولوية الاختيار (`init_best_channel`)

```c
static int init_best_channel(int prefer_at_tty) {
    if (prefer_at_tty) {
        if (open_device_list(AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    }
    /* الأولوية لـ IPC المباشر ثم سوكيت RILD ثم منافذ AT */
    if (open_device_list(IPC_NODES, CH_DEV_IPC_RAW) == 0) return 0;
    if (open_rild_socket() == 0) return 0;
    if (open_device_list(AT_NODES, CH_DEV_AT_TTY) == 0) return 0;
    return -1;
}
```

**القاعدة:** IPC خام أولاً → سوكيت rild ثانياً → منفذ AT نصي أخيراً.

### 5.3 حزمة RIL Parcel (لسوكيت rild)

```c
#define RIL_REQUEST_OEM_HOOK_RAW 59

static int send_rild_oem_parcel(int fd, const uint8_t *ipc_data, int ipc_len, ...) {
    // [4 بايت Big-Endian: الطول الكلي]
    // [4 بايت: req_id = 59]
    // [4 بايت: token (متزايد)]
    // [4 بايت: طول البيانات]
    // [بيانات IPC الخام]
    // [حشو محاذاة 4 بايت]
}
```

### 5.4 مصعد الصلاحيات عبر البورت 2600

```c
static int try_escalate_via_port_2600(int port) {
    if (getuid() == 1000 || getuid() == 0) return 0;
    // الاتصال بـ 127.0.0.1:2600
    // إرسال: "setprop ctl.stop ssud; id\n"
    // قراءة الرد → البحث عن "uid=1000"
}
```

**الشرط المسبق:** مستمع shell على البورت 2600 — يُنشأ بحيلة `audiocoredebug`:
```text
input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'
```

### 5.5 أوامر at-bridge التفاعلية (كاملة)

| الأمر | الوظيفة |
|---|---|
| `CSC:<CODE>` | تغيير CSC تلقائي (5 حزم IPC) |
| `AT:<CMD>` | إرسال أمر AT خام (مثال: `AT:AT+DEVCONINFO`) |
| `HEX:<DATA>` | إرسال إطار IPC hex |
| `SCAN` | فحص وعرض قنوات المودم المتاحة |
| `STATUS` | حالة الجهاز + القناة + أقفال SIM |
| `LOCKS` / `SIMLOCK` | فحص أقفال SIM والشبكة (CPIN + CLCK) |
| `RECONNECT` | إعادة فحص وإعادة اتصال بأفضل قناة |
| `MODE:AT` | التبديل لقناة AT النصية |
| `MODE:IPC` | التبديل لقناة IPC / سوكيت rild |
| `AUTH:<SEED>` | تفعيل تشفير Xorshift32 |
| `INFO` | معلومات مختصرة |
| `HELP` | قائمة المساعدة |
| `EXIT` / `QUIT` | إنهاء الجلسة |

### 5.6 أوامر CLI (سطر الأوامر)

| الوسيط | الوظيفة |
|---|---|
| `-c <CSC>` | تغيير CSC سريع |
| `-a <AT_CMD>` | إرسال أمر AT واحد والخروج |
| `-x <HEX>` | إرسال إطار IPC hex واحد |
| `-s` | وضع الفحص (SCAN) |
| `-l` | فحص أقفال SIM |
| `-d <DEVICE>` | فرز مسار محدد (مثال: `-d /dev/umts_atc0`) |
| `-p <PORT>` | منفذ المصعد (افتراضي 2600) |
| `-v` | مخرجات تشخيصية |
| `-h` / `--help` | المساعدة |

---

## 6) رسائل تحميل المكتبة داخل `AT` + إثبات عدم استيراد `dlopen`

### 6.1 رسائل محمّل المكتبة (مفكوكة من الرام)

| النص | Ghidra | مفتاح XOR | الدلالة |
|---|---|---|---|
| `[-] Failed to load library %s: %s` | `0x2aa030` | `0x18` | فشل تحميل المكتبة |
| `send_recv_cmd` | `0x2aa058` | `0xed` | **اسم الرمز المطلوب** (تصدير libsecril-client.so) |
| `[-] Failed to find symbol 'send_recv_cmd' in %s: %s` | `0x2aa070` | `0x64` | فشل إيجاد الرمز |
| `[i] Found 'send_recv_cmd' in %s.` | `0x2aa0b0` | `0xf8` | نجاح إيجاد الرمز |
| `[-] Library %s did not return a valid response (code: %d, len: %d).` | `0x2aa0e0` | `0x1a` | المكتبة لم تستجب |
| `00000000` | `0x2aa128` | `0xbb` | قيمة افتراضية |
| `[-] Failed to extract a valid key from %s's response.` | `0x2aa140` | `0x48` | فشل استخراج مفتاح المصادقة |
| `[SUCCESS] Challenge key (CALC) obtained from %s: %s` | `0x2aa181` | `0x89` | نجاح الحصول على مفتاح CALC |
| `[SECURE] Encrypted CALC for use: %s` | `0x2aa1c0` | `0x6d` | CALC مشفر |
| `echo ` | `0x2aa308` | `0xa6` | بادئة بناء الأمر |
| ` \| /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1` | `0x2aa310` | `0x4e` | لاحقة القناة |

### 6.2 إثبات عدم استيراد `dlopen`/`dlsym`

**الملف:** `AT` (ELF 64-bit AArch64, ET_DYN/PIE, NDK r23c, API 21)

**إحصاء الواردات:**

| البند | العدد |
|---|---:|
| إجمالي `.dynsym` | 113 |
| إجمالي `.rela.plt` (PLT stubs) | 112 |
| `dlopen` | **0** ✗ |
| `dlsym` | **0** ✗ |
| `dlclose` | **0** ✗ |
| `dlerror` | **0** ✗ |
| `dl_iterate_phdr` | 1 (unwinder فقط) |
| `popen` | 8 مواقع استدعاء |
| `system` | 77 موقعاً |
| `fgets` | 5 |
| `getline` | 2 |
| `pclose` | 4 |
| `fopen` | 1 (فحص anti-debug على `/proc/self/status`) |

**مواقع `popen` (بناء أوامر `echo ... | nc 2600`):**
```text
0x659a8, 0x65b0c, 0x6bb5c, 0x6d0fc, 0x71ec4, 0x74374, 0x74dc8, 0x75be0
```

**الاستنتاج:** AT **لا يحمل المكتبة بنفسه** — يبني أوامر shell (`echo '<payload>' | toybox nc 127.0.0.1 2600`) وينفذها عبر `popen`/`system`، ويقرأ النتائج. التحميل الفعلي يجري في المكوّن المرافق (العميل المدمج أو libril.so المدمجة في APK).

### 6.3 الآلية المرصودة: `echo ... | nc 2600`

**النمط المكتشف (22222.md + التفكيك):**

```text
echo '<payload>' | /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1
```

- النص `echo ` (Ghidra `0x2aa308`) **يُلحق** بأوامر أخرى.
- النص ` | /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1` (Ghidra `0x2aa310`) **يُلحق** بالأمر.
- يُنفذ عبر `popen(cmd, "r")` — النتيجة تُقرأ عبر `fgets`/`getline`.
- رسائل الخطأ: `Error: popen() failed!`, `Error: No log entry found.`, `Error: Could not find [...] in log entry.`

**الغرض:** إرسال أوامر إلى مستمع shell يعمل بصلاحية UID 1000 (system) أو root على البورت 2600 — **مصعد الصلاحيات**.

---

## 7) التسلسل الزمني الكامل — متى وكيف تُستخدم

### 7.1 المرحلة 1: التحضير (يُنفذ مرة واحدة)

```text
1) قراءة ABI:
   adb shell getprop ro.product.cpu.abi
   → "arm64-v8a" أو "armeabi-v7a"

2) اختيار الوحدة المناسبة:
   arm64-v8a  → HUN_RIL_SAM2025_06_ALL_noapparm64v8a (athun)
   armeabi-v7a → HUN_RIL_SAM2025_06_ALL_noapparmiv7a (athun)

3) دفع الأداة:
   adb push <module> /data/local/tmp/athun        ← مرصود x2
   adb shell chmod 0777 /data/local/tmp/athun     ← مرصود x2

4) [اختياري] تثبيت APK يحتوي libril.so:
   adb install -r -g <apk>                        ← مرصود (QTMPX142)
   → التطبيق يحمّل lib/<abi>/libril.so عبر System.loadLibrary("ril")
```

### 7.2 المرحلة 2: مصعد الصلاحيات (البورت 2600)

```text
1) فتح نشاط audiocoredebug:
   am start -n com.sec.android.app.audiocoredebug/...AudioAdbShellActivity

2) ضبط المساعد:
   settings put Secure assistant com.sec.android.app.audiocoredebug/...

3) كتابة أمر nc في حقل النص:
   input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'

4) تنفيذ:
   input keyevent 66 (ENTER)

→ النتيجة: مستمع shell على TCP 2600 بصلاحية system (UID 1000)
```

### 7.3 المرحلة 3: تشغيل الجلسة (كل عملية)

```text
1) بدء الأداة:
   /data/local/tmp/athun

2) المصادقة:
   AT+AKSEEDNO=0           ← مرصود x2
   AT+AKSEEDNO=1,0         ← مرصود x4
   ← الرد: +AKSEEDNO:<seed>,<num>-<num>-<num>
   ← حساب CALC: AES + RSA_verify + SHA256 (في العميل المدمج)
   ← "[SUCCESS] Challenge key (CALC) obtained from %s: %s"

3) فتح قناة RIL:
   [مكتبة libsecril-client.so] → send_recv_cmd      ← أقوى قناة
   [أو] /dev/umts_ipc0                               ← IPC خام
   [أو] /dev/socket/rild                             ← Parcel (req 59)
   [أو] /dev/umts_atc0                               ← AT نصي

4) قراءة بيانات الجهاز:
   AT+DEVCONINFO            ← MNC/MCC + معلومات
   AT+SVCIFPGM=1,1          ← Info 2

5) العمليات:
   GETSPR                   ← قراءة SPR (نسخة احتياطية)
   SETSPR + payload         ← كتابة SPR (MCC/MNC جديدة)
   SRELOCK1 + MCCREQ        ← إعادة قفل بقائمة MCC/MNC
   AT+<cmd>                 ← تمرير أوامر مودم

6) التحقق:
   GETSPR (مرة أخرى)         ← تأكيد الكتابة

7) الإنهاء:
   exit / EXIT
   adb shell rm /data/local/tmp/athun    ← مرصود x2
   adb shell logcat -b radio -c          ← مرصود x4
```

### 7.4 جدول القرار: أي قناة لأي مهمة؟

| المهمة | القناة المفضلة | الأمر |
|---|---|---|
| قراءة DEVCONINFO | AT نصي (`/dev/umts_atc0`) | `AT+DEVCONINFO` |
| قراءة SPR | secril lib أو IPC | `GETSPR` |
| كتابة SPR (MCC/MNC) | secril lib أو IPC | `SETSPR` + payload |
| إعادة القفل | secril lib (أقوى) | `SRELOCK1` + MCCREQ |
| تغيير CSC | IPC خام (hex) | `0601000E0000...` (5 حزم) |
| فحص أقفال SIM | AT نصي | `AT+CPIN?` + `AT+CLCK=...` |
| مصادقة | عبر shell (2600) | `AT+AKSEEDNO` |
| عملية MCC/MNC (KEY-Tool) | libril.so داخل APK | `Reading All MCC+MNC` → `Writing SIMLock Blob` |

---

## 8) أوامر الجلسة التي تقود استخدام RIL

| الأمر | الوظيفة | يتطلب قناة RIL؟ |
|---|---|---|
| `AT+AKSEEDNO=0` / `=1,0` | مصادقة (بذرة) | نعم — قبل أي عملية |
| `AT+DEVCONINFO` | قراءة معلومات + MNC/MCC | نعم — AT نصي |
| `AT+SVCIFPGM=1,1` | Info 2 | نعم |
| `GETSPR` | قراءة SPR | نعم — secril/IPC |
| `SETSPR` | كتابة SPR (MCC/MNC) | نعم — secril/IPC |
| `SRELOCK1` | إعادة قلق بقائمة MCC/MNC | نعم — secril/IPC |
| `AT+<cmd>` | تمرير أمر مودم | نعم — حسب الأمر |
| `HEX:<data>` | إطار IPC خام | نعم — IPC/rild |
| `CSC:<code>` | تغيير CSC (5 حزم) | نعم — IPC |
| `SCAN` | فحص القنوات | لا — فحص فقط |
| `STATUS` | حالة القناة | لا |
| `EXIT` / `exit` | إنهاء | لا |

---

## 9) مصفوفة الحضور والإثبات

**الرموز:** ✅ موجود/مرصود، ◐ في المصدر فقط، ✗ غير موجود

| العنصر | داخل `AT` | `at-bridge` | `KEY-Tool.DMP` | `SimUnlockerPro.DMP` | مرصود (L24) |
|---|---|---|---|---|---|
| `libril.so` (خاصة بالأداة) | ✗ (لا نص) | ✗ | ✅ ZIP CD x2 | ✅ ZIP x3 | ✗ |
| `libsecril-client.so` | ✗ (لا نص، لكن `send_recv_cmd` مرصود) | ✅ سطر 35 | ✅ NEEDED @`0x68a2222` | ✗ | ✗ |
| `libsec-ril.so` | ✗ | ✗ | ✅ NEEDED @`0x68a2236` | ✗ | ✗ |
| `send_recv_cmd` | ✅ نص `0x2aa058` | ✅ `CH_SECRIL_LIB` | ✅ (مرتبط) | ✗ | ✗ |
| `dlopen`/`dlsym` | ✗ (لا استيراد!) | ✅ compile `-ldl` | ✅ imports | ✅ imports | ✗ |
| `/dev/socket/rild` + أخواتها | ✗ | ✅ سطور 152–158 | ✗ | ✗ (زائف!) | ✗ |
| `/dev/umts_*` + `/dev/smd*` | ✗ | ✅ سطور 129–149 | ✗ | ✗ | ✗ |
| البورت 2600 | ✅ `0x2aa310` | ✅ `try_escalate` | ✗ | ✗ | ✗ (لا التقاط حزم) |
| `athun` (الوحدة المدفوعة) | ✗ (لا نص، اسم خارجي) | ✗ | ✗ | ✗ | ✅ push x2 |
| عميل KEY-Tool (ELF 32-بت) | ✗ | ✗ | ✅ @`0x689f91e` | ✗ | ✗ |
| `ATRIL`/`ATRIL64`/`NoAppRil` | ✗ | ✗ | ✗ | ✅ UTF-16 | ✗ |
| `RIL202310`/`RIL202409` | ✗ | ✗ | ✗ | ✅ | ✗ |
| وحدات `HUN_RIL_*` | ✗ | ✗ | ✗ | ✗ | ✗ (نص في Q-Unlocker فقط) |
| `AT+AKSEEDNO` | ✗ (لا نص) | ✗ | ✅ (نص وكيل) | ✗ | ✅ x6 |
| `classes.dex` | ✗ | ✗ | ✅ UTF-16 | ✅ | ✗ |
| `libplayservice.so` | ✗ | ✗ | ✅ @`0x16ba75e` | ✗ | ✗ |

---

## 10) حدود وعدم يقين + تحذير منهجي

### 10.1 ما لم نجد

1. **لا نص `libril.so` داخل ملف `AT`** (0/141 نصاً مفكوكاً) — دليلها فقط ZIP CD داخل DMPs.
2. **لا نص `libsecril-client.so` داخل `AT`** — لكن `send_recv_cmd` (رمزها) موجود.
3. **آلية تحميل المكتبة داخل AT غير مفروغة 100%** — الكود C++ مشوّش بتسطيح التدفق (control-flow flattening)؛ المؤكد أن AT لا يستورد `dlopen` وأن الرسائل موجودة.
4. **محتوى `lib/arm64-v8a/libril.so` (351,901 بايت) داخل ZIP لم يُستخرج** — يمكن استخراجه بطلب (deflate method 8).
5. **سوكيتات `rild` وعقد `/dev` من مصدر at-bridge فقط** — لم تُرصد في جلسات L24 كأوامر منفذة.
6. **لا التقاط حزم على البورت 2600** — الدليل نصي فقط.

### 10.2 ⚠️ تحذير منهجي: نتائج زائفة (False Positives)

**البحث عن `rild` في SimUnlockerPro.DMP أعطى نتائج زائفة:**

| Offset | النص الفعلي | سبب المطابقة |
|---|---|---|
| `0x3c4cab3` | `"...gösterildikd..."` (تركية) | `rild` داخل `gösterildikd` |
| `0x9b07b9c` | `"...gösterildikd..."` (أذرية) | نفس السبب |

**القاعدة:** قبل قبول أي نتيجة بحث في ذاكرة خام، افحص **السياق** — ليست كل مطابقة دليلاً.

---

## 11) المصادر (مسارات مطلقة)

### 11.1 AT (HUNTER-UNLOCKER)

- `C:\Users\pc5\Documents\AT\AT` — الملف الأساسي (1,749,912 بايت).
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\AT_ALL_STRINGS.json` — 141 نصاً.
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\AT_XREF_HITS.json` — مراجع النصوص.
- `C:\Users\pc5\Documents\AT\22222.md` — تحليل سابق (البورت 2600 + APK المدمج + مفاتيح XOR).

### 11.2 at-bridge

- `C:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\at_bridge.c` — 843 سطراً:
  - سطر 30: `RIL_REQUEST_OEM_HOOK_RAW 59`
  - سطر 33–39: `channel_mode_t` (4 أوضاع)
  - سطر 89–123: `try_escalate_via_port_2600()`
  - سطر 129–158: جداول `IPC_NODES` / `AT_NODES` / `RILD_SOCKETS`
  - سطر 185–204: `open_rild_socket()`
  - سطر 206–222: `init_best_channel()` (الأولوية)
  - سطر 339–365: `send_rild_oem_parcel()` (حزمة Parcel)
  - سطر 593–624: `print_help()` (CLI + التفاعلية)
  - سطر 630–843: `main()` (CLI + حلقة التفاعل)
- `C:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\all_gsm_commands.h` — 192 سطراً.

### 11.3 تفريغ الذاكرة (DMPs)

- `C:\Users\pc5\Desktop\KEY-Tool.DMP` (241,154,382 بايت):
  - `0x689f91e` — عميل RIL 32-بت (ELF, 45,708 بايت).
  - `0x68a2222` — `libsecril-client.so` (dynstr).
  - `0x68a2236` — `libsec-ril.so` (dynstr).
  - `0x16ba75e` — `libplayservice.so: Permission denied`.
  - `0x2fdacc3` + `0x3cb9ff1` — ZIP CD: `lib/arm64-v8a/libril.so`.
  - `0x6c0fbf1` — `classes.dex` (UTF-16).
- `C:\Users\pc5\Desktop\SimUnlockerPro.DMP` (799,484,172 بايت):
  - `0x3ab1989`, `0x3b09a9f`, `0x52b2ebb` — `lib/armeabi-v7a/libril.so`.
  - `0x2015ecf` — `dlopen failed` + `RIL202310` + `RIL202409`.
  - `0x17920fa5` — `ATRIL`/`ATRIL64`/`NoAppRil`/`Ekbase.apk`/`DM.apk` (UTF-16).

### 11.4 سجلات L24

- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\MonitoringToolkit\MonitoringStudio\bin\Debug\net8.0-windows\Logs\L24_SourceSentinel\*.jsonl`
- `C:\Users\pc5\Documents\AT\OBSERVED_AT_COMMANDS.txt` — 419 أمراً.

### 11.5 استخراج أدوات Q-Unlocker

- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\region_results.txt` — وحدات `HUN_RIL_*` + `Hunter_Ril_Sam_spr_*`.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\relock_extracted_results.txt` — `adb install -r -g QTMPX142`.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\keytool_exploit_flow.txt` — `Push classes.dex File Error` + `Send AT Command Then Send OK`.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\keytool_dmp_analysis.txt` — واجهة KEY-Tool.

### 11.6 ملفات الاستخراج

- `C:\Users\pc5\Documents\AT\extract_ril_libraries.py` — سكربت الاستخراج والفحص.
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\KEYTool_RIL_agent_32bit.elf` — العميل المستخرج (45,708 بايت).

---

**نهاية الملف.**  
**القائمة المسطحة:** `LIBRIL_COMMANDS.txt`  
**المستندات السابقة:** `MCC_MNC_COMMANDS.md` / `MCC_MNC_COMMANDS.txt` / `MCC_MNC_LIST.csv`
