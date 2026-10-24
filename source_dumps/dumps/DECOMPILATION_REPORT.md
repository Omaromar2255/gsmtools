# تقرير الاستخراج الكامل لملف AT — من رام الهاتف + التفكيك البرمجي

**التاريخ:** 22/10/2026
**الجهاز:** Samsung SM-G715U1 (Exynos 9611، أندرويد 13، بدون روت، SELinux Enforcing)
**الملف الهدف:** `C:\Users\pc5\Documents\AT\AT`

---

## 1. بصمات الملف (تطابقت بين الكمبيوتر والهاتف قبل التنفيذ)

| الخوارزمية | القيمة |
| :--- | :--- |
| MD5 | `4C463D3085F45535EE89C8BE0A78619F` |
| SHA-256 | `AD40E6C9CED2CB0FB9CC5A5352ED525505B180F607077D4D1C8D3CBA2DD9F6A2` |
| الحجم | 1,749,912 بايت (~1.75 MB) |
| النوع | ELF 64-bit LSB (AArch64)، PIE، مبني بـ Android NDK r23c، minAPI 21 |
| المعمارية | AArch64 — يفكك بمعالج Ghidra `AARCH64:LE:64` |

## 2. مراحل التنفيذ الفعلي (كلها تمت بدون محاكاة)

1. **دفع الملف:** `adb push AT /data/local/tmp/AT` + `chmod 777` — تم التحقق من MD5 على الهاتف.
2. **التشغيل مع إبقاء الجلسة حية:** `tail -f /dev/null | /data/local/tmp/AT` في الخلفية — الأداة طلعت البانر وانتظرت الإدخال.
3. **التجميد:** `kill -STOP <PID>` (PID 24740).
4. **قراءة خرائط الذاكرة:** من `/proc/24740/maps`.
5. **تفريغ القطاعات:** عبر `dd if=/proc/<pid>/mem` — 4 قطاعات.
6. **التحقق من المطابقة** ثم إنهاء العملية.

### مخرجات البانر الحية من الهاتف (من الرام مباشرة)

```
******        HUNTER-UNLOCKER TOOL 2025       ******
******           SAM RIL  V2.3 2025           ******
 [+] Product Model  : SM-G715U1
 [+] Product CPU    : exynos9611
 [+] Product Name   : samsung
 [+] Serial Number  : R5CRC37NYRZ
 [+] Security patch : 2022-12-01
[*] Generate Authenticator
Authorisation Token:84DkqA8ARrOxvip8HTGfMzEjOt5IFK0BGO1McwLJBURWlreGDY9C0B9OCm25qcwfff6kc9pHtnHKMiVhq0YAN/usLVYuJRNVXncE6raVOQWN8MH0fYVVC0aiINKDJUWOrD1cNHjEw/NhRNWyBZ5B8GGPCFo807AcqqwyLzXFVU+YYIAw7dJgRr0rpvomeaQsMpcfQ5MB+k0JMu0c+qxEMdTH8iOeBPAO+07K8SHqcjfjMFh8nH0cBeZmDOVaHg9/x1LudZCbkFjT60xxrfDoaXw/Km2Mtaq5A4hyrIOM43U=
RESPONSE :
```

> ملاحظة: التوكن يُولَّد محلياً داخل هذه الأداة (Base64 طويل)، وهي تنتظر إدخال `RESPONSE` — هذا **مختلف** عن أداة KEY-Tool المذكورة في سجل الغيميني (التي كانت تجلب التوكن من خادم على البورت 2600).

## 3. القطاعات المسحوبة من الرام والتحقق

| الملف | العنوان في الرام | الحجم | المقابل في الملف | نتيجة المقارنة |
| :--- | :--- | :--- | :--- | :--- |
| `dump1_code_r-xp.bin` | 0x55a45c5000 | 1,482,752 | LOAD r-x (كود البرنامج) | **مطابق 100% — صفر اختلاف** |
| `dump2_relro_r--p.bin` | 0x55a472f000 | 32,768 | `.init_array` + `.data.rel.ro` + GOT | اختلافات = relocations وقت التشغيل (طبيعي) |
| `dump3_data_rw-p.bin` | 0x55a4737000 | 241,664 | `.data` + BSS | **معظمه مختلف لأن الأداة فكّت تشفير نصوصها في نفس المكان بالذاكرة** |
| `dump4_bss_anon.bin` | 0x55a4772000 | 12,288 | BSS (حالة تشغيل) | بيانات حية |

**نتيجة جوهرية:** الكود المنفَّذ في الهاتف مطابق حرفياً للملف على القرص — أي أن هذا الملف **غير مضغوط وغير محمي بطبقة UPX** (عكس ملفات KEY-Tool في السجل السابق)، ويمكن تحليله ثابتاً مباشرة. الحماية الوحيدة فيه هي:
1. **تشفير XOR أحادي لجدول الأوامر** داخل `.data`، تُفكّه مُنشئات `.init_array` عند الإقلاع في نفس المكان (الدالة الرئيسية للفك: `FUN_00204730` — فكّت 73 أمراً دفعة واحدة).
2. **تسطيح تدفق التحكم (OLLVM CFF)** في `main` والدوال الحساسة (موزّع بمفاتيح سحرية مثل `0x37a0fdc8`).

## 4. الأوامر المستخرجة بالكامل

- **`ALL_COMMANDS.txt`** — القائمة الكاملة: **128 نصاً/أمراً مفكوكاً** من رام الهاتف، مرقّمة، مع عنوان كل أمر في الذاكرة والغات (Ghidra) والدوال التي تستدعيه. أبرزها:

| الفئة | أمثلة |
| :--- | :--- |
| قناة الشل المحلية | ` | /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1` |
| فتح شل مستمع | `input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'` |
| أتمتة الشاشة | `uiautomator dump /sdcard/window_dump.xml`، `cat /sdcard/window_dump.xml`، `rm -rf /sdcard/window_dump.xml`، `input tap `، `input keyevent 3/4/61/66/82/219` |
| قوائم هندسية سامسونج | `echo "am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 ..."`، `com.sec.android.app.audiocoredebug`، `com.samsung.android.FactoryTestLauncher` |
| إعدادات النظام | `settings put system screen_off_timeout 600000`، `settings put system/Secure assistant ...AudioAdbShellActivity...`، `settings put global function_key_config_doublepress_value ...` |
| تدمير الأدلة | `pm clear com.sec.android.app.audiocoredebug`، `pm clear com.samsung.android.FactoryTestLauncher`، `logcat -b system,main -c`، `logcat -b radio -c`، `ps -a|grep logcat` |
| تعطيل الاتصال | `cmd wifi set-wifi-enabled disabled` |
| أتمتة النوافذ | Regex لزر `aerr_wait` وزر `EXECUTE` مع استخراج `bounds=` |
| خصائص الجهاز | `ro.serialno`، `ro.build.version.release/sdk/incremental`، `ro.boot.debug_level`، `ro.boot.cp_debug_level` |
| رسائل RIL | `Worng Ril Command` (هكذا)، `Ril Token Error, Try Again`، `RIL Exit`، `[-] Failed to load library %s: %s`، `[-] Library %s did not return a valid response (code: %d, len: %d).`، `[-] Failed to find symbol 'send_recv_cmd' in %s: %s` |
| بانر الأداة | `HUNTER-UNLOCKER TOOL 2025`، `SAM RIL V2.3 2025`، أسطر `[+] Product Model ...` |

- **`COMMANDS_BY_FUNCTION.txt`** — عكس الجدول: كل دالة والأوامر التي تستدعيها، مع بيان الدوال التي تنفّذ فعلياً عبر `system`/`popen`/`fwrite`/`fopen`. المرجعيات حُسبت من بايتات الكود نفسه (ADRP+ADD) وليست تخميناً.

## 5. التفكيك البرمجي (Ghidra Headless 12.1.4)

- الملف حلّل بنجاح و**2626 دالة** اكتُشفت، فُكّ **2514 دالة** بنجاح (فشل 2 فقط — قوائم المكتبات الخارجية).
- **`GHIDRA_DECOMPILED.c`** — الكود المفكك الكامل (~5 MB، 166 ألف سطر).
- **`GHIDRA_DECOMPILED_FUNCTION_LIST.txt`** — فهرس كل دالة بعنوانها وحجمها.
- **`KEY_FUNCTIONS.c`** — 93 دالة مفتاحية مختارة ومُعلّقة بالأوامر التي تستخدمها:

| المجموعة | الدوال | الدور |
| :--- | :--- | :--- |
| البداية | `FUN_0013b87c` | `main` — بانر + حلقة الجلسة التفاعلية (CFF مسطّح) |
| فاكّ التشفير | `FUN_00204730` (+13 منشئ init_array آخر) | فك جدول الأوامر XOR في مكانهم بالذاكرة عند الإقلاع |
| قراءة الجهاز | `FUN_0013840c` (11.9KB) | قراءة خصائص النظام عبر `__system_property_get` وبناء البانر |
| التنفيذ | 16 دالة تستدعي `system`، 8 تستدعي `popen`، 13 تستدعي `fwrite` | تنفيذ الأوامر المفكوكة في الشل/المودم |
| أتمتة الشاشة | `FUN_001790d8`، `FUN_00182684` | دورة uiautomator → regex → input tap على EXECUTE/aerr_wait |
| الإخفاء | `FUN_0016efc4` | تعطيل الواي فاي + مسح بيانات الأنشطة الهندسية + مسح logcat |
| التوكن | `FUN_00212e50` (تنسيق Hex %02X)، `FUN_0024e20c` (يحتوي ثابت تجزئة 64-hex) | توليد/تنسيق التوكن |

## 6. الملفات المدمجة داخل AT (استُخرجت فعلياً)

- **APK كامل مدمج** (Base64 XOR 0x47 في الملف، ويوجد مفكوكاً في الرام جاهزاً):
  - `EMBEDDED_APK/embedded.apk` — 168,069 بايت، ZIP سليم 22 ملفاً
  - الحزمة: `com.sdet.fotaagent` — "FOTA Agent by FDM Team" v6.3.0
  - `sharedUserId=android.uid.system` + توقيع شهادة منصة سامسونج + Device Admin
  - محتوياته مفكوكة بالكامل في `EMBEDDED_APK/apk_files/` وسلاسل dex في `APK_DEX_STRINGS.txt`

## 7. فهرس ملفات الناتج الكاملة (`AT_Source_Extract/`)

| الملف/المجلد | الوصف |
| :--- | :--- |
| `DUMPS/dump1..4 + maps.txt` | قطاعات ذاكرة الهاتف الحية + خرائط العملية |
| `DUMPS/at_stdout_banner.txt` | البانر الحي والتوكن من الهاتف |
| `ALL_COMMANDS.txt` | **كل الأوامر المفكوكة** (128) مع العناوين والدوال |
| `COMMANDS_BY_FUNCTION.txt` | خريطة دالة → أوامر (109 دوال) |
| `KEY_FUNCTIONS.c` | 93 دالة مفتاحية مفككة ومُعلّقة |
| `GHIDRA_DECOMPILED.c` + `_FUNCTION_LIST.txt` | التفكيك الكامل (2514 دالة) |
| `REAL_STRINGS.txt` | كل السلاسل من الرام + من الملف |
| `XOR_DECODED_STRINGS.txt` | نتائج فك XOR من الملف الثابت (مع المفاتيح) |
| `EMBEDDED_APK/` | الـ APK المدمج ومحتوياته |
| `DecompileAll.java` / `DecompileAll.py` | سكربت التفكيك الآلي لـ Ghidra (Java هو المستخدم فعلياً) |
| `func_strings.json` | خريطة xref خام (دالة ↔ أوامر) |

## 8. ملاحظات تقنية

1. **بدون روت**: التفريغ نجح لأن العملية ابن لجلسة `adb shell` (نفس الـ uid=2000) وسياسة SELinux تسمح لـ shell بقراءة ذاكرة أبنائه.
2. **الطريقة قابلة للتكرار** على أي ملف آخر من نفس العائلة: دفع → تشغيل مع stdin معلّق → STOP → قراءة maps → dd لكل قطاع → سحب → تحقق. (سكربتات المراحل محفوظة في `%TEMP%` ويمكن تجميعها في أداة واحدة عند الطلب).
3. للملفات **المضغوطة فعلاً** (مثل ملفات KEY-Tool في السجل السابق): نفس الطريقة تعمل، لكن التفريغ يكون من قطاعات `anon r-xp` بعد فك الضغط الذاتي — وهذا ما فُعل سابقاً وليس ضرورياً هنا.
4. **التوكن**: يُولَّد محلياً في هذه النسخة (لا يحتاج خادم 2600). خوارزمية توليده موجودة في `FUN_00212e50`/`FUN_0024e20c` وكود الجلسة في `main` — متاحة في `KEY_FUNCTIONS.c` لأي تطوير لاحق.

---

## تحديث 23/10/2026 — الاستخراج الكامل النهائي للأوامر

- أُعيد فحص **كل** نصوص `.data` بمقارنة القرص/الرام بايت-بايت: **141 نصاً فريداً** فُكّ في الرام، منها **123 أمراً/نصاً ذا معنى**.
- **إثبات الاكتمال:** لا يوجد أي مقطع قابل للطباعة (≥8 أحرف) بقي مشفّراً في `.data`، ولا أي أمر في `.rodata`.
- **تصحيح مهم:** أوامر `AT+PRECONFG` / `AT+CLCK` / `AT+DEVCONINFO` / `/dev/umts_*` **ليست في ملف AT** — مصدرها الحقيقي كود `gsmtools/at-bridge` (C). AT يحتوي فقط البادئة `AT+` (0x2aa424) ويحمّل مكتبة RIL ويستدعي `send_recv_cmd`.
- **المخرجات الجديدة:**
  - `ALL_COMMANDS_COMPLETE.md` — القاموس الشامل الموثّق (AT + at-bridge + AT_unpacked + جدول تحقق).
  - `ALL_COMMANDS_MASTER.txt` — قائمة مسطحة بوسم المصدر.
  - `AT_ALL_STRINGS.json` / `AT_XREF_HITS.json` / `AT_BRIDGE_EXTRACT.json` — بيانات آلية.
  - `build_command_dictionary.py` — سكربت البناء (محفوظ لإعادة الاستخدام).
- سكربتات بايثون السابقة كلها محفوظة، و`extract_all_commands.py` عُدّل لي writes إلى `RAW_XOR_SCAN.*` كي لا يطمس الملفات النهائية.
