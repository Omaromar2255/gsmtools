# أوامر MCC & MNC — الاستخراج الكامل الموثّق

**التاريخ:** 23/10/2026  
**النطاق:** كل أمر/خاصية/رسالة لها علاقة مباشرة أو غير مباشرة بـ **MCC** (Mobile Country Code) و **MNC** (Mobile Network Code) في:

- `AT` (HUNTER-UNLOCKER — SAM RIL V2.3 2025)
- `at-bridge` (مشروع gsmtools)
- `AT_unpacked.elf`
- أدوات الكمبيوتر المرصودة: **UnlockTool**، **Q-Unlocker**، **General Unlocker Pro**، **CDMA-YTP**، **KEY-Tool**
- سجلات الجلسات `L24_SourceSentinel` (COM104 / COM123)
- قاعدة بيانات **Octoplus MCC_MNC_LIST**

> **قاعدة ذهبية:** كل أمر هنا مرفق بدليله. ما لم يُرصد يُكتب بجانبه صراحةً: **معياري/غير مرصود** أو **في المصدر فقط**. لا خلط بين «مذكور في الكود» و«مُنفَّذ فعلياً».

---

## 0) الخلاصة التنفيذية

1. **ملف `AT` لا يحتوي حرفياً على أي نص** `MCC` أو `MNC` أو `COPS` أو `CREG` أو `CIMI` أو `IMSI` — **0 من 141** نصاً.
2. القراءة الفعلية لـ MCC/MNC تتم بثلاث طرق:
   - رد المودم: `AT+DEVCONINFO` → `MNC(01);MCC(421)`
   - IMSI: `AT+CIMI` → أول 3 أرقام MCC، ثم 2–3 أرقام MNC.
   - خصائص أندرويد: `getprop gsm.sim.operator.numeric` أو `gsm.operator.numeric`.
3. الأمر الوحيد **داخل `AT`** المرتبط بإعادة قفل المشغل هو `SRELOCK` (يُستقبل عملياً كـ `SRELOCK1`)، ويعتمد على حزمة `MCCREQ`.
4. `MCCREQ` ليس قراءة من الجهاز؛ هو **قائمة MCC/MNC مختارة** تُرسل ضمن طلب `Carrier Relock` إلى سيرفر الأداة.
5. أداة أندرويد منفصلة باسم `AT_SamMCCMNCTool_V1_arm64_KT` وملف بيانات `MCCMNC.txt` مذكوران داخل `KEY-Tool.DMP`، لكن الملفين **غير موجودين محلياً**.
6. قاعدة Octoplus المحلية: **1688** صفاً، **225** MCC، **224** دولة، **1023** شبكة — مُصدَّرة إلى `MCC_MNC_LIST.csv`.
7. **أوامر التغيير/الإضافة (Write/Add):** لا يوجد أمر AT معياري يغيّر MCC/MNC على الشريحة. الأدوات تكتب **قائمة MCC/MNC لقفل المشغل** عبر `SETSPR` / `SRELOCK1` (أوامر داخل `AT`)، أو `AT+CPOL` (معياري) لقوائم PLMN — موثقة بالكامل في **القسم 12**.

---

## 1) كيف تُقرأ MCC/MNC فعلياً؟ (الطبقات الثلاث)

| الطبقة | الأمر/الخاصية | الناتج | الدقة |
|---|---|---|---|
| المودم (Samsung) | `AT+DEVCONINFO` | `MNC(01);MCC(421)` | مباشر — أفضل مصدر |
| الشريحة (IMSI) | `AT+CIMI` | `42101...` | مباشر، لكن طول MNC يعتمد على الخطة |
| أندرويد (SIM) | `getprop gsm.sim.operator.numeric` | `42101` | مباشر |
| أندرويد (الشبكة الحالية) | `getprop gsm.operator.numeric` | `42101` | مباشر (غير مرصود في جلساتنا) |
| الشبكة/التسجيل (3GPP) | `AT+COPS?` / `AT+COPS=?` | `"42101"` | معياري — غير مرصود |
| قاعدة محلية | `MCC_MNC_LIST` | اسم الدولة/الشبكة | مرجعي |

**الفرق المهم:**

- `gsm.sim.operator.numeric` = MCC+MNC **المخزّن على الشريحة**.
- `gsm.operator.numeric` = MCC+MNC **للشبكة المسجّل عليها حالياً** (قد يختلف عند التجوال).
- `AT+DEVCONINFO` يعطي `MNC` و`MCC` كحقلين منفصلين من المودم.
- `AT+CIMI` يعطي IMSI كاملاً؛ استخراج MCC/MNC يتطلب معرفة طول MNC (2 أو 3).

---

## 2) أوامر AT المباشرة (مرصودة فعلاً)

### 2.1 `AT+DEVCONINFO` — المصدر الأساسي لـ MCC/MNC

**الأمر:**

```text
AT+DEVCONINFO
```

**الدليل:**

- معرّف في `at-bridge`: `all_gsm_commands.h` سطر **83**:
  ```c
  #define AT_CMD_DEVCONINFO "AT+DEVCONINFO\r\n"
  ```
- مرصود **35 مرة** كأمر صريح في `OBSERVED_AT_COMMANDS.txt`.
- مصادره: **UnlockTool** (RAM: `0x01475E7E`، `0x22578D51`، `0x31410196`، `0x34566078`، `0x37979063`)، **Q-Unlocker** (`0x01B841E3`, `0x01BABB08`)، **General Unlocker Pro** (`0x01EEC79D`, `0x01F2F3E9`).

**مثال رد حقيقي (UnlockTool — RAM @ `0x22578D60`، الحجم 253 بايت):**

```text
+DEVCONINFO: MN(SM-J260A);BASE(SM-N900);VER(J260AUCU2ASE1/J260AATT2ASE1/J260AUCU2ASE1/J260AUCU2ASE1);HIDVER(J260AUCU2ASE1/J260AATT2ASE1/J260AUCU2ASE1/J260AUCU2ASE1);MNC(01);MCC(421);PRD(ATT);;OMCCODE();SN(RF8M714T5WP);IMEI(356718100860755);UN(CQE006F8D6...
```

**ملاحظات:**

- الترتيب في الرد: `MNC(...)` **قبل** `MCC(...)`.
- القيم قد تكون فارغة: `MNC();MCC();` — شوهدت على أجهزة Wi-Fi فقط (مثل SM-T580).
- `PRD(ATT)` هو كود المنتج/المشغل، وليس MCC/MNC.
- الرد الكامل قد يتجاوز 253 بايت؛ ما يهمنا حقلا `MNC` و`MCC`.

**استخراج عملي (Python):**

```python
import re

def parse_devconinfo(resp: str):
    mnc = re.search(r"MNC\(([^)]*)\)", resp)
    mcc = re.search(r"MCC\(([^)]*)\)", resp)
    mnc = mnc.group(1).strip() if mnc else ""
    mcc = mcc.group(1).strip() if mcc else ""
    if mcc.isdigit() and mnc.isdigit():
        return {"mcc": mcc, "mnc": mnc, "plmn": mcc + mnc}
    return {"mcc": mcc, "mnc": mnc, "plmn": ""}
```

> لا تستخدم `zfill(2)` بشكل أعمى مع MNC؛ هناك MNC ثلاثي الخانات. استخدم القيمة كما وردت.

---

### 2.2 `AT+SVCIFPGM=1,1` — «Info 2 / Read Mobile Info»

**الأمر:**

```text
AT+SVCIFPGM=1,1
```

**الدليل:**

- مرصود **16 مرة** كأمر صريح.
- مصادره: **UnlockTool** (`0x0147EF0C`)، **General Unlocker Pro** (`0x01F2F42E`)، **Q-Unlocker** (`0x01BABB29`).
- في تسلسل Q-Unlocker يظهر مقترناً بـ `AT+DEVCONINFO`:
  ```text
  AT+DEVCONINFO   → Info 1 / DEVCONINFO
  AT+SVCIFPGM=1,1 → Info 2 / Read Mobile Info
  ```
- لم يُلتقط رد `+SVCIFPGM` كاملاً في السجلات؛ الأداة تستخدمه كـ«Info 2» وقد يعيد حقولاً إضافية. لا تجزم بمحتواه دون اختبار على جهاز.

**الاستخدام:** يُرسل بعد `AT+DEVCONINFO` لجمع معلومات إضافية عن الجهاز/الشريحة. ليس بديلاً عن `DEVCONINFO` في قراءة MCC/MNC، لكنه جزء من نفس تسلسل القراءة.

---

### 2.3 `AT+CIMI` — IMSI ومنه MCC/MNC

**الأمر:**

```text
AT+CIMI
```

**الدليل:**

- مرصود **34 مرة** كأمر صريح، و**27 مرة** بصيغة `AT+CIMI=?`.
- معروف في مصنّف `SerialSniffer\ComPortMonitor.cs` سطر **335**:
  ```csharp
  _ when upper.StartsWith("AT+CIMI") => "SIM IMSI Request",
  ```
- مصادره: **UnlockTool** بكثافة (عشرات العناوين في الرام).

**بنية IMSI:**

```text
IMSI = MCC (3 أرقام) + MNC (2 أو 3 أرقام) + MSIN (الباقي)
مثال: 421011234567890
      421  01  1234567890
      MCC  MNC MSIN
```

**كيف تعرف طول MNC؟**

- الأفضل: `getprop gsm.sim.operator.numeric` (5 أو 6 أرقام).
- أو من قاعدة `MCC_MNC_LIST` (لكنها قد لا تشمل كل MNC جديد).
- لا تفترض أن كل MNC من خانتين؛ أمثلة MNC ثلاثية: 410 (AT&T)، 260 (T-Mobile US)، وغيرها.

**تحذير:** `AT+CIMI` قد يفشل أو يعيد `ERROR` إذا كانت الشريحة غير جاهزة. تحقق أولاً بـ `AT+CPIN?`.

---

### 2.4 جدول الأوامر المباشرة

| الأمر | الوظيفة | الحالة | العدد المرصود |
|---|---|---|---|
| `AT+DEVCONINFO` | قراءة `MNC`/`MCC` وبيانات الجهاز | مرصود | 35 |
| `AT+SVCIFPGM=1,1` | Info 2 / Read Mobile Info | مرصود | 16 |
| `AT+CIMI` | قراءة IMSI | مرصود | 34 (+27 لصيغة `=?`) |
| `AT+CPIN?` | التأكد من جاهزية الشريحة قبل CIMI | مرصود | 29 |
| `AT+CSQ` | شدة الإشارة (مساعدة) | مرصود | 38 |
| `AT+CFUN?` / `AT+CFUN=1` | حالة الراديو (مساعدة) | مرصود | 27 / 30 |
| `AT+CSCS` | مجموعة المحارف (مساعدة) | مرصود | 36 |

> الأعداد من `OBSERVED_AT_COMMANDS.txt` (لكل نص أمر صريح). عند احتساب الردود/السياقات قد تزيد.

---

## 3) أوامر اختيار/تغيير الشبكة (3GPP TS 27.007)

> هذه الأوامر **معيارية** ومعروفة، لكنها **لم تُرصد** في جلسات L24. أُدرجت لأنها الأوامر القياسية للتعامل مع MCC/MNC واختيار المشغل. جرّبها على جهاز اختبار فقط.

### 3.1 `AT+COPS` — اختيار المشغل

| الأمر | المعنى |
|---|---|
| `AT+COPS?` | المشغل الحالي |
| `AT+COPS=?` | قائمة المشغلين المتاحين (مع MCC/MNC) |
| `AT+COPS=0` | اختيار تلقائي |
| `AT+COPS=1,2,"42101"` | اختيار يدوي بالصيغة الرقمية |
| `AT+COPS=2` | إلغاء التسجيل |
| `AT+COPS=3,2` | ضبط صيغة العرض على numeric |
| `AT+COPS=4,...` | اختيار يدوي/تلقائي حسب الحالة |

**أمثلة ردود:**

```text
AT+COPS?
+COPS: 0,2,"42101",7

AT+COPS=?
+COPS: (2,"Sabaphone","Sabaphone","42101",7),(1,"MTN","MTN","42102",7),(1,"Yemen Mob. CDMA","Yemen Mob. CDMA","42103",7)
```

- `<format>=2` يعني أن `<oper>` رقمي `"MCC+MNC"`.
- `AT+COPS=?` هو أفضل أمر لسرد MCC/MNC للشبكات المجاورة.
- بعد `AT+COPS=1,2,"42101"` قد يفقد الجهاز التسجيل إذا كان التردد/الشريحة غير متوافقين.

**الدليل:** `ComPortMonitor.cs` سطر **333** يصنّف `AT+COPS` كـ`Operator Selection / Registration`. لم يُرصد تنفيذه في السجلات.

---

### 3.2 `AT+CREG` / `AT+CGREG` / `AT+CEREG` — حالة التسجيل

| الأمر | المعنى |
|---|---|
| `AT+CREG?` | حالة تسجيل GSM |
| `AT+CREG=2` | تمكين عرض LAC/CI |
| `AT+CGREG?` | حالة تسجيل GPRS/PS |
| `AT+CEREG?` | حالة تسجيل LTE/EPS |

**مثال:**

```text
AT+CREG?
+CREG: 2,1,"1A2B","0C3D4E5F",7
```

- لا يعيد MCC/MNC مباشرة؛ يعيد حالة التسجيل وLAC/CI.
- لاستخراج MCC/MNC استخدم `AT+COPS?` أو `AT+COPS=?`.
- `AT+CREG` معروف في `ComPortMonitor.cs` سطر **334**؛ `AT+CGREG`/`AT+CEREG` معياريان وغير مرصودين.

---

### 3.3 `AT+CPOL` / `AT+CPLS` — قوائم المشغلين المفضلة

| الأمر | المعنى |
|---|---|
| `AT+CPOL?` | قراءة قائمة PLMN المفضلة (تحتوي MCC/MNC) |
| `AT+CPOL=<index>,2,"42101"` | إضافة/تعديل مشغل بالصيغة الرقمية |
| `AT+CPLS=0` | اختيار قائمة PLMN على SIM |
| `AT+CPLS=1` | اختيار قائمة PLMN على USIM |
| `AT+CPLS=2` | اختيار قائمة OPLMN |

- معياري، غير مرصود.
- `AT+CPOL?` قد يعيد عشرات السجلات؛ MCC/MNC يكونان في حقل `<oper>` عند `<format>=2`.

---

### 3.4 جدول أوامر الشبكة

| الأمر | الوظيفة | الحالة |
|---|---|---|
| `AT+COPS?` | المشغل الحالي | معياري — غير مرصود |
| `AT+COPS=?` | سرد المشغلين | معياري — غير مرصود |
| `AT+COPS=0` | اختيار تلقائي | معياري — غير مرصود |
| `AT+COPS=1,2,"MCCMNC"` | اختيار يدوي | معياري — غير مرصود |
| `AT+CREG?` | تسجيل GSM | معروف في المصنّف — غير مرصود |
| `AT+CGREG?` | تسجيل GPRS | معياري — غير مرصود |
| `AT+CEREG?` | تسجيل LTE | معياري — غير مرصود |
| `AT+CPOL?` | قائمة PLMN مفضلة | معياري — غير مرصود |
| `AT+CPLS=...` | اختيار قائمة PLMN | معياري — غير مرصود |

---

## 4) أوامر سامسونج/الأدوات المرتبطة بالقفل (مرصودة)

> هذه ليست أوامر MCC/MNC بالمعنى الضيق، لكنها تأتي في نفس مسار **Carrier Relock / CSC / SIM Lock**، ولذلك تُوثّق هنا بشكل منفصل.

### 4.1 `AT+PRECONFG` — إعداد مسبق للمشغل/المنطقة

**الصيغ من المصدر (`all_gsm_commands.h` سطر 79–80):**

```c
#define AT_CMD_PRECONFG_READ      "AT+PRECONFG=1,0\r\n"
#define AT_CMD_PRECONFG_WRITE_FMT "AT+PRECONFG=2,%s\r\n"
```

**المرصود:**

- `AT+PRECONFG=2` — **18 مرة**.
- `AT+PRECONFG=2,0` — **2 مرة**.
- المصادر: UnlockTool (`0x014848A0`)، General Unlocker Pro (`0x01F33CF8`)، Q-Unlocker (`0x01BAF171`, `0x01BF43A7`, `0x01BF8D64`).

**ملاحظة:** Samsung preconfig، وليس أمر 3GPP. قد يؤثر على إعدادات المشغل/المنطقة. لا تُرسل قيماً عشوائية.

---

### 4.2 `AT+RFBYCODE` — تغيير CSC/Region

**الأوامر المرصودة:**

```text
AT+RFBYCODE=1,1,0
AT+RFBYCODE=2,0
AT+RFBYCODE=2,0,XAA
AT+RFBYCODE=2,1
AT+RFBYCODE=2,1,XAA
```

**العدد:** 16 مرة إجمالاً (2,0,XAA مرتين؛ 2,1,XAA أربع مرات؛ 1,1,0 أربع مرات؛ 2,0 مرتين؛ 2,1 أربع مرات).

**المصادر:** Q-Unlocker (`0x01BB0C5F`, `0x01BB0DA2`, `0x01BB0DF2`, `0x01BF43B6`, `0x01BF8AB1`, `0x01BF8AD4`, `0x01BF8B39`)، General Unlocker Pro (`0x01F2F513`).

**مثال رد:** `+RFBYCODE:2,OK`.

**ملاحظة:** `XAA` هو كود CSC لمنتجات US Open. الأمر يغيّر CSC/المنطقة، وليس MCC/MNC مباشرة. يُستخدم غالباً قبل إعادة القفل.

---

### 4.3 `AT+UNLOCKSPR=OLD` — فك قفل SPR

**الأمر:**

```text
AT+UNLOCKSPR=OLD
```

**الدليل:**

- مرصود **2 مرة**.
- Q-Unlocker — RAM @ `0x01BB02CA`.
- يظهر في تسلسل أداة Q-Unlocker ضمن أوامر فك القفل (SPR = Service Provider Lock، غالباً Sprint).

**ملاحظة:** مرتبط بقفل المشغل، وليس بقراءة MCC/MNC.

---

### 4.4 `AT+SIMLOCKU=0,0,0` و `AT+SIMLGDATA=<...>`

**الأوامر المرصودة:**

```text
AT+SIMLOCKU=0,0,0
AT+SIMLGDATA
AT+SIMLGDATA=357923101971102
AT+SIMLGDATA=990013500173492
```

**الدليل:**

- `AT+SIMLOCKU=0,0,0` — **2 مرة** — Q-Unlocker RAM @ `0x01BAD6D7`.
- `AT+SIMLGDATA` — **8 سياقات** — Q-Unlocker RAM @ `0x01BB046A`, `0x01BF9D05`, `0x03A86C1D`, `0x03B05C39`.
- تظهر ضمن مسار **LG SIM Lock** في Q-Unlocker.

**ملاحظة:** بيانات قفل SIM (LG/Android)، وليست MCC/MNC.

---

### 4.5 `AT+CLCK` — حالة أقفال الشبكة/التخصيص

**من `at-bridge`:**

```text
AT+CLCK="PN",2
AT+CLCK="PU",2
AT+CLCK="PP",2
AT+CLCK="PC",2
```

**الردود:**

```text
+CLCK: 0
+CLCK: 1
```

**المعنى:**

| الأمر | القفل |
|---|---|
| `"PN"` | Network Lock |
| `"PU"` | Network Subset Lock |
| `"PP"` | Service Provider Lock |
| `"PC"` | Corporate Lock |

**ملاحظة:** ليس MCC/MNC، لكنه يحدد حالة قفل المشغل. `at-bridge` يبني منه سطر `SIMLOCK|CPIN=...|PN=...|PU=...|PP=...|PC=...`.

---

### 4.6 `SRELOCK` / `SRELOCK1` — الأمر الداخلي لإعادة القفل

**داخل `AT`:**

- النص `SRELOCK` في Ghidra عند **`0x2aa350`** (raw `0x1aa350`).
- نسخة صغيرة `srelock` عند **`0x2aa358`**.
- المقارنة داخل `FUN_00161334` (raw `0x61334`، الحجم 40984 بايت، ينتهي `0x6b34c`).

**التفكيك الفعلي (raw offsets):**

```asm
0x065a30  ldr   x0, [x19, #0x320]      ; مؤشر الأمر المُدخل
0x065a34  adrp  x1, #0x1aa000
0x065a38  mov   w2, #7                 ; طول البادئة = 7
0x065a3c  add   x1, x1, #0x350         ; "SRELOCK"
0x065a40  bl    #0x1695c0              ; مقارنة أول 7 أحرف
0x065a44  cmp   w0, #0
0x065a48  cset  w11, eq                ; 1 إذا تطابقت البادئة
0x065a5c  strb  w11, [x19, #0x32f]     ; علم الحالة
```

**النتيجة:** أي أمر يبدأ بـ`SRELOCK` يُقبل، وليس `SRELOCK` فقط. لذلك `SRELOCK1` يعمل. هذا يفسر وجود `SRELOCK1` في Q-Unlocker.

**في أداة Q-Unlocker:**

```text
Invalid MCC-MNC
SRELOCK1
```

**التسلسل (مُستنتج من تجاور النصوص وواجهة الأداة، وليس من التقاط حزم كامل):**

```text
1) AT+DEVCONINFO                 ← قراءة معلومات الجهاز
2) اختيار MCC/MNC من الواجهة     ← Select MCC/MNC List
3) MCCREQ (JSON)                 ← إرسال القائمة للسيرفر
4) استلام Blob/Token             ← apirelock/
5) SRELOCK1 + البيانات           ← إلى AT على الجهاز
6) إعادة تشغيل الجهاز            ← "Carrier Relock Done. Please Reboot Device Manually."
```

**حدود التحليل:** لم نفكك كامل آلة الحالة (`FUN_00161334` ضخمة ومشوّشة)، لذلك **صيغة الحزمة التي يتوقعها `AT` بعد `SRELOCK1` غير مؤكدة**. المؤكد: قبول البادئة ووضع علم الحالة. لا تدّعِ صيغة غير مثبتة.

---

## 5) `MCCREQ` — صيغة الحزمة المرصودة

**المصدر:** `region_results.txt` سطر **23** و**26** (طلب `apirelock/` من Q-Unlocker).

**مقتطف الطلب:**

```json
{
  "CSC": "XAU",
  "CSC2": "",
  "IMEI": "35118406037239",
  "IMEI2": "352048830372390",
  "MCCREQ": "421F0100421F0200421F0300421F0400421F1000421F1100",
  "OprationType": "Carrier Relock",
  "Opration_Function": "Carrier Relock M1",
  "Opration_name": "Carrier Relock",
  "Ril": "1",
  "SN": "R5CX105PKGA",
  "model": "SM-S928U1"
}
```

**الطول:** 48 حرف Hex = **24 بايت** = **6 سجلات × 4 بايت**.

**التفكيك:**

| # | Hex | MCC | MNC | الشبكة في قاعدة Octoplus |
|---|---|---|---|---|
| 1 | `421F0100` | 421 | 01 | Sabaphone |
| 2 | `421F0200` | 421 | 02 | MTN/Spacetel |
| 3 | `421F0300` | 421 | 03 | Yemen Mob. CDMA |
| 4 | `421F0400` | 421 | 04 | HITS/Y Unitel |
| 5 | `421F1000` | 421 | 10 * | غير موجودة في القاعدة المحلية |
| 6 | `421F1100` | 421 | 11 * | غير موجودة في القاعدة المحلية |

**بنية السجل (4 بايت):**

```text
42 1F 01 00
│  │  │  └─ 00: حشو/فاصل
│  │  └──── MNC = 01 (بايت واحد BCD)
│  └─────── 1F: الرقم الثالث للمـ MCC=1 + F حشو
└────────── 42: أول رقمين للمـ MCC=42
```

- `0x42` = BCD للرقمين `4` و`2`.
- `0x1F` = BCD للرقم `1` + `F` (حشو).
- `0x01` = MNC `01`; `0x10` = MNC `10`.
- `0x00` = حشو/فاصل نهاية السجل.

**مقارنة مع ترميز 3GPP TS 24.008 (3 بايت):**

| الترميز | MCC 421 + MNC 01 | الحجم |
|---|---|---|
| ترميز الأداة المرصود | `42 1F 01 00` | 4 بايت/سجل |
| ترميز 3GPP القياسي | `24 01 F1` | 3 بايت/PLMN |

> **لا تخلط بين الترميزين.** ترميز 3GPP يضع nibbles بترتيب مختلف (`MCC2|MCC1`, `MNC1|MCC3`, `MNC3|MNC2`). ترميز الأداة يبدو كـ«MCC BCD رباعي + MNC بايت + 00».

**تحذير بخصوص MNC ثلاثي الخانات:** لا يوجد في العيّنة المرصودة أي MNC من 3 خانات. لذلك **لا يمكن الجزم** كيف يُرمّز `MNC=410` داخل هذا الحقل الرباعي. الاحتمالات: بايت MNC واحد فقط (حتى 99)، أو استخدام البايت الرابع للرقم الثالث. لا تبنِ أداة على افتراض غير مثبت.

**\* احتمال ترميز بديل:** إذا كان الحقلان الأخيران عدداً 16-bit little-endian بدلاً من «BCD + حشو»، فإن `421F1000` = MNC **16** و`421F1100` = MNC **17**، وليس 10/11. لا يوجد مثال حاسم في العيّنة. التفسير المذكور (10/11) يعتمد على أن البايت الأخير `00` حشو، وهو الأرجح نظراً لتكرار `00` في كل السجلات، لكنه ليس مؤكداً 100%.

**هل `MCCREQ` قراءة من الجهاز؟** لا. هو **قائمة اختيار** من واجهة Q-Unlocker (`Select MCC/MNC List`، `Selected MCC-MNC`) تُرسل للسيرفر. قد تكون قائمة الشبكات المسموح بها لإعادة القفل، وليست بالضرورة الشبكة الحالية للجهاز. الدليل: وجود 421 (اليمن) مع موديل أمريكي `SM-S928U1` وCSC `XAU`.

---

## 6) Android / ADB — خصائص وأوامر MCC/MNC

### 6.1 الخصائص المرصودة

| الأمر | ما يعيده | العدد | المصدر |
|---|---|---|---|
| `adb -s <serial> shell getprop gsm.sim.operator.numeric` | MCC+MNC للشريحة (`42101`) | 14 | CDMA-YTP.exe |
| `adb -s <serial> shell getprop gsm.operator.alpha` | اسم الشبكة | 13 | CDMA-YTP.exe |
| `adb -s <serial> shell getprop gsm.operator.iso-country` | ISO الدولة | 11 | CDMA-YTP.exe |
| `adb -s <serial> shell getprop gsm.sim.state` | حالة الشريحة | 11 | CDMA-YTP.exe |
| `adb -s <serial> shell getprop gsm.network.type` | نوع الشبكة (LTE/UMTS...) | 16 | CDMA-YTP.exe |
| `adb -s <serial> shell getprop ro.boot.carrierid` | Carrier ID | 25 | CDMA-YTP.exe |
| `adb -s <serial> shell getprop persist.sys.prev_salescode` | CSC السابق | 12 | CDMA-YTP.exe |
| `adb shell getprop ro.csc.sales_code` | CSC الحالي | 1 (تشخيصي) | PowerShell |

### 6.2 خصائص MCC/MNC في `at-bridge`

من `all_gsm_commands.h` سطر **147–148**:

```c
"gsm.sim.operator.numeric",
"gsm.operator.numeric",
```

- `gsm.sim.operator.numeric` — **مرصود** (14 مرة) و**مُعرّف** في at-bridge.
- `gsm.operator.numeric` — **مُعرّف** في at-bridge لكن **لم يُرصد** كأمر ADB منفّذ في جلساتنا.
- `ro.csc.sales_code` و`ro.csc.countryiso_code` — سطر **142–143** في نفس الملف.

### 6.3 خصائص قياسية إضافية (غير مرصودة)

| الخاصية | المعنى |
|---|---|
| `gsm.sim.operator.alpha` | اسم مشغل الشريحة |
| `gsm.sim.operator.iso-country` | دولة مشغل الشريحة |
| `gsm.operator.numeric` | MCC+MNC للشبكة الحالية |
| `gsm.operator.alpha` | اسم الشبكة الحالية |
| `gsm.operator.iso-country` | دولة الشبكة الحالية |
| `gsm.network.type` | نوع الشبكة |
| `gsm.sim.state` | حالة الشريحة |

### 6.4 أوامر ADB مساعدة

```bash
# قراءة مباشرة
adb shell getprop gsm.sim.operator.numeric
adb shell getprop gsm.operator.numeric
adb shell getprop gsm.operator.alpha
adb shell getprop gsm.operator.iso-country

# كل خصائص gsm مرة واحدة
adb shell getprop | grep -E "gsm\.(sim\.)?operator\.(numeric|alpha|iso-country)|gsm\.network\.type|gsm\.sim\.state"

# سجل الهاتف (غير مرصود — معياري)
adb shell dumpsys telephony.registry | grep -Ei "mcc|mnc|operatorNumeric|countryIso"

# قائمة APN مع MCC/MNC (غير مرصود — معياري)
adb shell content query --uri content://telephony/carriers --projection numeric,name
```

**أوامر `iphonesubinfo` المرصودة:**

```text
adb shell service call iphonesubinfo 1
adb shell service call iphonesubinfo 2
adb shell service call iphonesubinfo 15
```

- مرصودة من Q-Unlocker (7 مرات لـ`1`، و3+ لـ`2` و`15`).
- `1` يُستخدم عملياً لقراءة IMEI في الأداة (`service call iphonesubinfo 1 | cut -c 52-66`).
- **لا تفترض** أن `2` أو `15` = IMSI؛ أرقام المعاملات تختلف بين إصدارات أندرويد. لقراءة IMSI استخدم `AT+CIMI` أو `getprop gsm.sim.operator.numeric`.

---

## 7) قاعدة `MCC_MNC_LIST` (Octoplus)

**المسار:**

```text
C:\Program Files (x86)\Octoplus\Octoplus_Samsung\MCC_MNC_LIST
```

**النوع:** SQLite — جدول `mytable`.

**الأعمدة:**

```text
MCC, MCC_int, MNC, MNC_int, ISO, Country, Country_Code, Network, cheched
```

**الإحصاءات:**

| المقياس | القيمة |
|---|---|
| إجمالي الصفوف | 1688 |
| عدد MCC الفريد | 225 |
| عدد الدول الفريد | 224 |
| عدد الشبكات الفريد | 1023 |

**الملف المصدَّر:** `MCC_MNC_LIST.csv` — الأعمدة: `MCC,MNC,ISO,Country,Network` (1688 صف بيانات + رأس).

**أمثلة استعلام:**

```sql
-- كل شبكات اليمن
SELECT MCC, MNC, ISO, Country, Network
FROM mytable WHERE MCC = 421 ORDER BY MNC;

-- شبكة AT&T الأمريكية
SELECT MCC, MNC, ISO, Country, Network
FROM mytable WHERE MCC = 310 AND MNC = 410;
```

**النتائج:**

```text
421 | 01 | ye | Yemen | Sabaphone
421 | 02 | ye | Yemen | MTN/Spacetel
421 | 03 | ye | Yemen | Yemen Mob. CDMA
421 | 04 | ye | Yemen | HITS/Y Unitel

310 | 410 | us | United States | AT&T Wireless Inc.
```

**ملاحظة:** `421/10` و`421/11` غير موجودين في هذه القاعدة. قد تكون شبكات أحدث أو PLMN اختبارية. لا تجزم بهويتهما من القاعدة وحدها.

---

## 8) أدوات KEY-Tool / Q-Unlocker — نصوص MCC/MNC

### 8.1 Q-Unlocker

| النص | الدليل |
|---|---|
| `Invalid MCC-MNC` | `relock_extracted_results.txt` سطر 20/76/80؛ `region_results.txt` سطر 11/35 |
| `Length of MCC-MNC bytes:` | `region_results.txt` سطر 11/35 (سياق Carrier Relock عند 26221313) |
| `SRELOCK1` | `relock_extracted_results.txt` سطر 20/76/80 |
| `Select MCC/MNC List` | `relock_extracted_results.txt` سطر 96/113 |
| `Search Networks (Country, Operator, MCC, MNC)` | نفس السطر 96/113 |
| `MCCREQ` | `region_results.txt` سطر 23/26 |
| `Carrier Relock` / `Method 1` / `Method 2` | `relock_extracted_results.txt` |
| `Carrier Relock Done. Please Reboot Device Manually.` | `relock_extracted_results.txt` |

### 8.2 KEY-Tool.DMP

| النص | Offset في DMP | الترميز |
|---|---|---|
| `MCCMNC.txt` | `0x16c62c5` | ASCII |
| `MCCMNC.txt` | `0x6c0f975` | UTF-16LE |
| `AT_SamMCCMNCTool_V1_arm64_KT` | `0x6c0f9cf` | UTF-16LE |
| `AT_SamMCCMNCTool_V1_KT` | `0x6c0fb87` | UTF-16LE |
| `Reading All MCC+MNC Available :` | `0x16a515e` و`0x16ba48e` | ASCII |

**رموز الواجهة (من `keytool_dmp_analysis.txt` سطر 42–43):**

```text
MNCReLockSlot(int)
MNCReLockSignal(int)
mccmnccontinueslot(bool,QString)
MCCMNCcontinuesignal(bool,QString)
```

**حقول الواجهة:**

```text
lineMCC
lineMNC
listmcc
selectedmcc
List MCC-MNC
Search Line { Country / MCC-MNC }
Selected MCC-MNC
Save Profile
Load Profile
Add Manual
```

> هذه أسماء أدوات/ملفات/إشارات واجهة، وليست أوامر AT. الملفان `AT_SamMCCMNCTool_V1_arm64_KT` و`MCCMNC.txt` **غير موجودين محلياً**؛ الاسم فقط داخل DMP.

---

## 9) مصفوفة الحضور والإثبات

**الرموز:** ✅ = موجود/مرصود، ◐ = في المصدر فقط، ✗ = غير موجود/غير مرصود، ⭐ = معياري 3GPP.

| الأمر/الخاصية | داخل `AT` | `at-bridge` | أدوات PC (مرصود) | أندرويد/معياري |
|---|---|---|---|---|
| `AT+DEVCONINFO` | ✗ (لا نص) | ✅ سطر 83 | ✅ 35 | ✅ Samsung |
| `AT+SVCIFPGM=1,1` | ✗ | ✗ | ✅ 16 | ✅ Samsung |
| `AT+CIMI` | ✗ | ✗ | ✅ 34 | ⭐ |
| `AT+CPIN?` | ✗ | ✅ | ✅ 29 | ⭐ |
| `AT+COPS` | ✗ | ✗ | ◐ مصنّف فقط | ⭐ |
| `AT+CREG` | ✗ | ✗ | ◐ مصنّف فقط | ⭐ |
| `AT+CGREG` / `AT+CEREG` | ✗ | ✗ | ✗ | ⭐ |
| `AT+CPOL` / `AT+CPLS` | ✗ | ✗ | ✗ | ⭐ |
| `AT+PRECONFG=1,0` | ✗ | ✅ سطر 79 | ✗ | ✅ Samsung |
| `AT+PRECONFG=2,%s` | ✗ | ✅ سطر 80 | ✅ 18+2 | ✅ Samsung |
| `AT+RFBYCODE=...` | ✗ | ✗ | ✅ 16 | ✅ Samsung |
| `AT+UNLOCKSPR=OLD` | ✗ | ✗ | ✅ 2 | ✅ Samsung |
| `AT+SIMLOCKU=0,0,0` | ✗ | ✗ | ✅ 2 | ✅ LG/Samsung |
| `AT+SIMLGDATA=...` | ✗ | ✗ | ✅ 8 | ✅ LG |
| `AT+CLCK="PN",2` | ✗ | ✅ | ✗ | ⭐ |
| `SRELOCK` / `SRELOCK1` | ✅ `0x2aa350` | ✗ | ✅ Q-Unlocker | ✅ Samsung RIL |
| `gsm.sim.operator.numeric` | ✗ | ✅ سطر 147 | ✅ 14 | ✅ Android |
| `gsm.operator.numeric` | ✗ | ✅ سطر 148 | ✗ | ✅ Android |
| `gsm.operator.alpha` | ✗ | ✗ | ✅ 13 | ✅ Android |
| `gsm.operator.iso-country` | ✗ | ✗ | ✅ 11 | ✅ Android |
| `gsm.sim.state` | ✗ | ✗ | ✅ 11 | ✅ Android |
| `gsm.network.type` | ✗ | ✗ | ✅ 16 | ✅ Android |
| `ro.csc.sales_code` | ✗ | ✅ سطر 142 | ✅ 1 | ✅ Samsung |
| `ro.boot.carrierid` | ✗ | ✗ | ✅ 25 | ✅ Android |
| `MCCREQ` | ✗ | ✗ | ✅ Q-Unlocker | أداة خاصة |
| `MCCMNC.txt` | ✗ | ✗ | ✅ اسم في KEY-Tool | أداة خاصة |
| `AT_SamMCCMNCTool` | ✗ | ✗ | ✅ اسم في KEY-Tool | أداة خاصة |

**ملاحظة:** عمود «داخل AT» يعني وجود النص/المنطق في ملف `AT` نفسه. `AT` يملك بادئة `AT+` ويستدعي RIL/`send_recv_cmd`، لكنه لا يخزّن أسماء أوامر المودم. لذلك `AT+DEVCONINFO` «خارج AT» رغم أن `AT` قد يمرر أمراً بهذا الشكل.

---

## 10) ما لم نجده / حدود الاستخراج

1. **لا نصوص MCC/MNC داخل `AT`:** 0 من 141 نصاً. لا `COPS`، لا `CREG`، لا `CIMI`، لا `IMSI`.
2. **لا أمر كتابة MCC/MNC مباشر** داخل `AT` سوى مسار `SRELOCK` (غير مفكوك بالكامل).
3. **لا مثال MNC ثلاثي الخانات** في `MCCREQ`؛ ترميزه غير مؤكد.
4. **لا رد كامل لـ`AT+SVCIFPGM=1,1`** في السجلات؛ المحتوى الإضافي غير موثق.
5. **`AT+COPS` / `AT+CREG` لم يُرصدا** في جلسات L24؛ مصدرهما المصنّف/المعيار فقط.
6. **`gsm.operator.numeric`** مذكور في at-bridge فقط، ولم يُرصد كأمر ADB منفّذ.
7. **`AT_SamMCCMNCTool_V1_arm64_KT` و`MCCMNC.txt`** غير موجودين محلياً؛ الاسم فقط في `KEY-Tool.DMP`.
8. **`MCCREQ` قائمة اختيار وليست قراءة جهاز**؛ لا تفسّرها كالشبكة الحالية.
9. **`SRELOCK1`** مؤكد كنص في Q-Unlocker، لكن صيغة الحزمة التي يتوقعها `AT` بعد الأمر غير مؤكدة.
10. **قاعدة Octoplus** قديمة جزئياً: 421/10 و421/11 غير موجودين.

---

## 11) وصفات جاهزة

### 11.1 قراءة آمنة (بدون تغيير)

```text
AT+CPIN?
AT+DEVCONINFO
AT+CIMI
```

```bash
adb shell getprop gsm.sim.operator.numeric
adb shell getprop gsm.operator.numeric
adb shell getprop gsm.operator.alpha
adb shell getprop gsm.operator.iso-country
```

### 11.2 سرد الشبكات (معياري — قد يستهلك وقتاً)

```text
AT+COPS=?
```

### 11.3 اختيار شبكة يدوياً (اكتب فقط على جهاز اختبار)

```text
AT+COPS=3,2
AT+COPS=1,2,"42101"
```

للعودة إلى الاختيار التلقائي:

```text
AT+COPS=0
```

### 11.4 تحويل MCC/MNC إلى PLMN رقمي

```python
mcc = "421"
mnc = "01"          # لا تملأه أصفاراً إن كان ثلاثي الخانات
plmn = mcc + mnc     # "42101"
```

### 11.5 البحث في قاعدة Octoplus

```sql
SELECT Country, Network
FROM mytable
WHERE MCC = 421 AND MNC = 1;
```

---

## 12) أوامر تغيير/إضافة MCC & MNC (Write / Add)

> **قاعدة أساسية:** MCC/MNC مطبوعان في بيانات الشريحة (IMSI) — **لا يوجد أمر AT معياري لتغييرهما على الشريحة**. الممكن عملياً:
> 1. **إضافة/تعديل قائمة PLMN المفضلة** على الشريحة: `AT+CPOL=...`
> 2. **اختيار شبكة يدوياً**: `AT+COPS=1,2,"<MCC><MNC>"`
> 3. **تغيير/إضافة قائمة MCC/MNC لقفل المشغل** — وهذا ما تفعله الأدوات فعلاً: `SRELOCK1` / `SETSPR` → كتابة SPR / SIMLock Blob
> 4. **تغيير CSC/المنطقة** (يغيّر إعدادات المشغل الافتراضية): `AT+PRECONFG=2,...` / `AT+RFBYCODE=...`
> 5. **إضافة APN يحمل MCC/MNC**: `content insert --uri content://telephony/carriers ...`

### 12.1 أوامر الكتابة داخل `AT` (المُوزّع `FUN_00161334`)

| الأمر | الوظيفة | دليل المقارنة (raw) | ملاحظة |
|---|---|---|---|
| `SETSPR` | **كتابة SPR** (قائمة قفل المشغل التي تحوي MCC/MNC) | `0x68828–0x68854` | `mov w2,#6` + `add x1,#0x2d4` → النص `"SETSPR"` @ Ghidra `0x2aa2d4` |
| `setspr` | نفس الوظيفة (نسخة صغيرة) | `0x6b26c–0x6b284` | `mov w2,#6` + `add x1,#0x2dc`؛ النتيجة تُخزَّن في `[x19+0x2e0]` |
| `SRELOCK` | **إعادة قفل المشغل** (يُرسل فعلياً كـ`SRELOCK1`) | `0x65a30–0x65a5c` | `mov w2,#7` + `add x1,#0x350` |
| `srelock` | نسخة صغيرة | `0x68000–0x68010` | `mov w2,#7` + `add x1,#0x358` |
| `GETSPR` | قراءة SPR الحالي (للتحقق قبل/بعد الكتابة) | `0x613cc–0x613f8` | `add x1,#0x428` + `bl 0x169570` — عبر دالة مختلفة؛ الدقة الكاملة غير محسومة |
| `EXIT` / `exit` | إنهاء جلسة الأداة | `0x69af4` / `0x69bf8` | — |
| `AT+` | تمرير أوامر المودم | `0x63130–0x6316c` | عبر `0x60284` مع فحص نتيجة |

**تفكيك مقارنة `SETSPR`:**

```asm
0x068828  ldur  x0, [x29, #-0xf8]     ; المدخل
0x06882c  adrp  x1, #0x1aa000
0x068830  mov   w2, #6                ; طول البادئة = 6
0x068834  add   x1, x1, #0x2d4        ; "SETSPR"
0x068838  bl    #0x1695c0             ; مقارنة أول 6 أحرف
0x068844  cmp   w0, #0
0x068850  csel  w12, w8, w9, eq       ; اختيار الحالة التالية
```

> مثل `SRELOCK`: **أي أمر يبدأ بـ`SETSPR` يُقبل** (`SETSPR`, `SETSPR1`, `SETSPR=...`).

**ثوابت الكتابة في نفس المُوزّع:**

| الثابت | Ghidra | الاستخدام المرجّح |
|---|---|---|
| `51180004` | `0x2aa430` | ثابت بروتوكول SPR — يظهر أيضاً في Q-Unlocker (`# . 51180004`) |
| `5119010532` | `0x2aa2e8` | ثابت بروتوكول — يظهر في Q-Unlocker |
| `01426X1` / `01426x1` | `0x2aa2f8` / `0x2aa300` | بادئة توكن المصادقة (`athun.X1.AUTHORIZATION KEY:`) |
| `000000000000` | `0x2aa448` | **تُقارن مع حقل** (فحص «سجل أصفار/فارغ» — raw `0x64784–0x6479c`) — يرشّح أنها فحص سجل MCC/MNC فارغ |
| `52 i32 0 ` | `0x2aa238` | مقطع وسيطات (يُستخدم عند `0x64430` و`0x67fe8`) |
| `128/130/131/132/135/136 ` | `0x2aa24c–0x2aa26c` | مقاطع أكواد داخل الموزّع — الدلالة الدقيقة غير محسومة |
| `XgvkwKl8K6T8eAGsvyE2ydmLgBOpjk0F` | `0x2aa2b0` | مفتاح/ثابت (32 حرفاً) |
| `B1EEBF38E789633D…E60C57295` | `0x2aa1f0` | ثابت 64-hex (بصمة/توكن) |

### 12.2 التسلسل المرصود لتغيير/إضافة قائمة MCC/MNC

**A) دفع أداة `athun` وتشغيلها (مرصود):**

```text
adb push <file> /data/local/tmp/athun          (مرتان)
adb shell chmod 0777 /data/local/tmp/athun     (مرتان)
```

**B) المصادقة (مرصود):**

```text
AT+AKSEEDNO=0        (مرتان)
AT+AKSEEDNO=1,0      (4 مرات)
```

- محلّل الرد المرصود (General Unlocker Pro — RAM `0x01F2EE00`): `+AKSEEDNO:\d+,(\d+)-(\d+)-(\d+)`
- في وكيل KEY-Tool: `AT+AKSEEDNO=0,%04x` + `Send AT Command Then Send OK to continue` + `OK`

**C) قراءة SPR ثم كتابته (نص Q-Unlocker المستخرج):**

```text
GETSPR ... 1 ... sam ... SETSPR ... exit ... EXIT
SETSPR . Result : OK . exit . EXIT . Unlocking Done
```

- `region_results.txt` (سياق `GsaVoiceInteractionService` @ 26247734): `GETSPR. . .1.sam.SETSPR.exit.EXIT`
- `relock_extracted_results.txt`: `Authenticated . Fail . # . . .1.lg.SETSPR.Result : OK.exit.EXIT.Unlocking Done`
- التسلسل: **`GETSPR` (قراءة) → معالجة → `SETSPR` (كتابة) → `Result : OK` → `exit`/`EXIT` → إعادة تشغيل**.
- وحدات مرصودة في نفس السياق: `Hunter_Ril_Sam_spr_2020armiv7a`, `HUN_RIL_SAM2025_06_ALL_noapparmiv7a`, `HUN_RIL_SAM2025_06_ALL_noapparm64v8a`, `LGE_RIL_SPR_HUNTER`, `AT_MotoSPRUnLockTool_V1`, `ATMOTOSPRGU`, `RIL_OEM_MTIK_2023`, `AT_King122020`, `/data/local/tmp/athun.sprsam`.

**D) إعادة القلق بقائمة MCC/MNC (Carrier Relock):**

```text
Select MCC/MNC List → Selected MCC-MNC → MCCREQ (JSON إلى apirelock/)
→ استلام Blob موقّع → SRELOCK1 → "Carrier Relock Done. Please Reboot Device Manually."
```

- المشغلون المدعومون (نص مرصود): `TMB-TMK-VZW-APP-AIO-XAU`
- رسائل الخطأ: `Invalid MCC-MNC`، `This carrier is Not Support`
- الطريقتان: `Method 1` / `Method 2` (`Carrier Relock M1` / `Carrier Relock M2`)
- نقطة النهاية: `https://api.q-unlocker.com/api/sere/2` + `apirelock/`

**E) KEY-Tool — كتابة SIMLock/Shared2 Blob:**

```text
Reading All MCC+MNC Available :  →  [ SIM] OK  →  Writing SIMLock Blob :
→  Writing Shared2 Blob :  →  - Writing SIMLock :   →  reboot debug
```

- الإضافة اليدوية: `Add Manual` + الحقلان `lineMCC` / `lineMNC` + `btnOK`
- ملفات التعريف: `Save Profile` / `Load Profile` (مع `MCCMNC.txt`)
- الأداة المنفذة على الهاتف: `AT_SamMCCMNCTool_V1_arm64_KT` / `AT_SamMCCMNCTool_V1_KT`
- تنظيف: `pm clear KEYTool`

**F) التنظيف بعد العملية (مرصود):**

```text
adb shell rm /data/local/tmp/athun          (مرتان)
adb shell logcat -b radio -c                 (4 مرات)
```

### 12.3 أوامر المودم المرصودة التي تغيّر المشغل/المنطقة

| الأمر | العدد | التأثير |
|---|---:|---|
| `AT+PRECONFG=2` / `AT+PRECONFG=2,0` | 18 + 2 | كتابة preconfig (مشغل/منطقة) |
| `AT+RFBYCODE=2,0,XAA` / `=2,1,XAA` | 2 + 4 | كتابة CSC — الرد: `+RFBYCODE:2,OK` |
| `AT+UNLOCKSPR=OLD` | 2 | فك قفل SPR (إزالة قيد المشغل) |
| `AT+SIMLOCKU=0,0,0` | 2 | التحكم في بيانات قفل SIM |
| `AT+SIMLGDATA=<...>` | 8 | كتابة بيانات قفل LG SIM |
| `AT+AKSEEDNO=0` / `=1,0` | 2 + 4 | بذرة المصادقة قبل الكتابة |

**حِزم تغيير CSC من at-bridge (قوالب كتابة):**

```text
0601000E0000%s0000%s     (1/5)
060100090000%s           (2/5)
060100120000%s0000%s%s   (3/5)
+ خطوتان [4/5] و[5/5] ببيانات إضافية
```

**أوامر ADB المرافقة (مرصودة):**

| الأمر | العدد |
|---|---:|
| `adb shell pm uninstall -k --user -0 com.sec.android.AutoPreconfig` | 2 |
| `adb shell pm uninstall -k --user 0 com.sec.android.CarrierCodeChanger` | 2 |
| `adb shell settings put  Secure assistant com.google.android.googlequicksearchbox/...GsaVoiceInteractionService` | 2 |
| `adb shell content insert --uri content://com.lge.node/omadm_node --bind key:s:OMADM_SIMLOCK --bind value:i:2` | 2 |

> لاحظ `-0` (سالب صفر) في أمر AutoPreconfig — كما ورد حرفياً في السجل.

### 12.4 أوامر 3GPP لإضافة/تعديل MCC/MNC (معيارية)

| الأمر | الوظيفة |
|---|---|
| `AT+CPOL=<index>,2,"42101"` | **إضافة/تعديل** مشغل رقمي في قائمة PLMN المفضلة |
| `AT+CPOL=<index>` | حذف مدخل |
| `AT+CPOL?` | قراءة القائمة |
| `AT+CPLS=0/1/2` | اختيار القائمة (SIM / USIM / OPLMN) |
| `AT+COPS=1,2,"42101"` | اختيار يدوي (لا يُضاف إلى القائمة) |
| `AT+CLCK="PN",1,"<pwd>"` | تفعيل Network Lock (الوضع 1 = قفل، 0 = فك، 2 = استعلام) |

> `at-bridge` يستخدم `AT+CLCK="PN",2` (استعلام فقط). الوضعان 1 و0 معياريان وغير مرصودين في جلساتنا.

### 12.5 إضافة APN يحمل MCC/MNC (Android — معياري غير مرصود)

```bash
adb shell content insert --uri content://telephony/carriers \
  --bind numeric:s:"42101" \
  --bind name:s:"Sabaphone" \
  --bind apn:s:"internet" \
  --bind mcc:s:"421" --bind mnc:s:"01"
```

- `numeric` = MCC+MNC معاً؛ و`mcc`/`mnc` عمودان منفصلان في جدول `carriers`.
- تعديل: `content update --uri content://telephony/carriers --where "numeric='42101'" --bind ...`
- حذف: `content delete --uri content://telephony/carriers --where "numeric='42101'"`

### 12.6 مصفوفة أوامر الكتابة/الإضافة

| الأمر | داخل AT | مرصود | المعيار |
|---|---|---|---|
| `SETSPR` / `setspr` | ✅ `0x2aa2d4` / `0x2aa2dc` | ✅ نص Q-Unlocker | خاص بالأداة |
| `SRELOCK` → `SRELOCK1` | ✅ `0x2aa350` | ✅ نص Q-Unlocker | خاص |
| `GETSPR` | ✅ `0x2aa428` | ✅ نص Q-Unlocker | خاص |
| `AT+AKSEEDNO=0` / `=1,0` | ✗ | ✅ 6 مرات | Samsung |
| `AT+PRECONFG=2,...` | ✗ (at-bridge:79–80) | ✅ 20 | Samsung |
| `AT+RFBYCODE=...` | ✗ | ✅ 16 | Samsung |
| `AT+CLCK="PN",1,"<pwd>"` | ✗ (at-bridge: وضع 2 فقط) | ✗ | ⭐ 3GPP |
| `AT+CPOL=...` / `AT+CPLS=...` | ✗ | ✗ | ⭐ 3GPP |
| `AT+COPS=1,2,...` | ✗ | ✗ | ⭐ 3GPP |
| `content insert .../telephony/carriers` | ✗ | ✗ | Android |
| `content insert .../com.lge.node/omadm_node` | ✗ | ✅ 2 | LG خاص |
| `pm uninstall ... AutoPreconfig / CarrierCodeChanger` | ✗ | ✅ 2+2 | Samsung مساعد |

### 12.7 تحذير الكتابة

- `SETSPR` / `SRELOCK1` يكتبان بيانات قفل المشغل (SPR / SIMLock Blob). الخطأ قد **يقفل الجهاز على شبكة لا تستخدمها** أو يُتلف بيانات EFS.
- إعادة القفل عملية خادمية مدفوعة؛ السيرفر يوقّع الحزمة ولا يمكن تكرارها محلياً.
- `AT+CPOL` / `AT+CPLS` يكتبان على الشريحة؛ بعض الشرائح ترفض الكتابة أو تُتلف المدخلات.
- `AT+CLCK` بالوضع 1 يطلب كلمة مرور القفل؛ المحاولات الخاطئة تستهلك فرص الإدخال.
- **خُذ نسخة احتياطية أولاً** (`GETSPR` / `Reading SIMLock Blob`) قبل أي كتابة، واختبر على جهاز اختبار فقط.

---

## 13) المصادر (مسارات مطلقة)

### 13.1 `AT`

- `C:\Users\pc5\Documents\AT\AT` — ملف AT (1,749,912 بايت).
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\AT_ALL_STRINGS.json` — 141 نصاً (0 MCC/MNC).
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\AT_XREF_HITS.json` — مراجع النصوص.
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\GHIDRA_DECOMPILED_FUNCTION_LIST.txt` — حدود الدوال (`FUN_00161334`).
- `C:\Users\pc5\Documents\AT\AT_Source_Extract\KEY_FUNCTIONS.c` — نصوص `FUN_00161334`.

**عناوين مهمة:**

- `SRELOCK` — Ghidra `0x2aa350` (raw `0x1aa350`).
- `srelock` — Ghidra `0x2aa358`.
- مقارنة بادئة SRELOCK (7 أحرف) — raw `0x65a30`–`0x65a64`؛ علم الحالة `[x19+0x32f]`.
- `SETSPR` — Ghidra `0x2aa2d4`؛ المقارنة (6 أحرف) — raw `0x68828`–`0x68854`.
- `setspr` — Ghidra `0x2aa2dc`؛ المقارنة (6 أحرف) — raw `0x6b26c`–`0x6b284`؛ النتيجة في `[x19+0x2e0]`.
- `GETSPR` — Ghidra `0x2aa428`؛ المقارنة — raw `0x613cc`–`0x613f8`.
- `AT+` — Ghidra `0x2aa424`؛ الفحص — raw `0x63130`–`0x6316c`.
- `EXIT` — Ghidra `0x2aa43c`؛ المراجع — raw `0x69af4` / `0x69bf8`.
- `000000000000` — Ghidra `0x2aa448`؛ الفحص — raw `0x64784`–`0x6479c`.
- `51180004` — Ghidra `0x2aa430`؛ `5119010532` — `0x2aa2e8`؛ `01426X1` — `0x2aa2f8`.

### 13.2 `at-bridge`

- `C:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\all_gsm_commands.h`
  - سطر 79: `AT+PRECONFG=1,0`
  - سطر 80: `AT+PRECONFG=2,%s`
  - سطر 83: `AT+DEVCONINFO`
  - سطر 142–143: `ro.csc.sales_code`, `ro.csc.countryiso_code`
  - سطر 147–148: `gsm.sim.operator.numeric`, `gsm.operator.numeric`
- `C:\Users\pc5\Documents\GitHub\gsmtools\at-bridge\jni\at_bridge.c` — `fast_getprop` والخصائص المقروءة فعلياً.

### 13.3 سجلات L24

- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\MonitoringToolkit\MonitoringStudio\bin\Debug\net8.0-windows\Logs\L24_SourceSentinel\*.jsonl`
- `C:\Users\pc5\Documents\AT\OBSERVED_AT_COMMANDS.txt` — 419 أمراً مرصوداً + 28 خاصية.

**أمثلة مراجع:**

- `AT+DEVCONINFO` — UnlockTool RAM `0x22578D60`، 253 بايت.
- `AT+UNLOCKSPR=OLD` — Q-Unlocker RAM `0x01BB02CA`.
- `AT+RFBYCODE=2,0,XAA` — Q-Unlocker RAM `0x01BF8AB1`.
- `AT+SIMLOCKU=0,0,0` — Q-Unlocker RAM `0x01BAD6D7`.
- `AT+SIMLGDATA` — Q-Unlocker RAM `0x01BB046A`, `0x01BF9D05`, `0x03A86C1D`, `0x03B05C39`.
- `+AKSEEDNO:\d+,(\d+)-(\d+)-(\d+)` (محلّل رد) — General Unlocker Pro RAM `0x01F2EE00`.
- `AT+AKSEEDNO=0` (x2) و`AT+AKSEEDNO=1,0` (x4) — أوامر مرصودة في `OBSERVED_AT_COMMANDS.txt`.
- `adb push ... /data/local/tmp/athun` + `chmod 0777` + `rm` — مرصودة (x2 لكل أمر).
- `content insert --uri content://com.lge.node/omadm_node ... OMADM_SIMLOCK ... 2` — مرصود (x2).

### 13.4 تحليل الأدوات

- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\relock_extracted_results.txt` — `SRELOCK1`, `Invalid MCC-MNC`, `Select MCC/MNC List`, `SETSPR . Result : OK`, `TMB-TMK-VZW-APP-AIO-XAU`, `AT_MotoSPRUnLockTool_V1`.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\region_results.txt` — `MCCREQ`, `Length of MCC-MNC bytes:`, وتسلسل `GETSPR. . .1.sam.SETSPR.exit.EXIT` (سياق `GsaVoiceInteractionService` @ 26247734) + وحدات `Hunter_Ril_Sam_spr_2020armiv7a` / `HUN_RIL_SAM2025_06_ALL_noapparm64v8a`.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\keytool_exploit_flow.txt` — `Reading All MCC+MNC Available :`, `Writing SIMLock Blob :`, `Writing Shared2 Blob :`, `- Writing SIMLock : `, `AT+AKSEEDNO=0,%04x`, `pm clear KEYTool`.
- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\DmpScanner\keytool_dmp_analysis.txt` — `MNCReLockSlot`, `listmcc`, `lineMCC`, `Add Manual`.
- `C:\Users\pc5\Desktop\KEY-Tool.DMP` — أسماء الأدوات و`MCCMNC.txt`.

### 13.5 قاعدة البيانات

- `C:\Program Files (x86)\Octoplus\Octoplus_Samsung\MCC_MNC_LIST` — SQLite، جدول `mytable`.
- `C:\Users\pc5\Documents\AT\MCC_MNC_LIST.csv` — 1688 صفاً.

### 13.6 مصنّف الأوامر

- `C:\Users\pc5\Documents\GitHub\PhoenixGSM\tools\MonitoringToolkit\SerialSniffer\ComPortMonitor.cs`
  - سطر 333: `AT+COPS`
  - سطر 334: `AT+CREG`
  - سطر 335: `AT+CIMI`

---

## 14) ملاحظة أمان

- `AT+COPS=...` يمكن أن يفصل الجهاز عن الشبكة.
- `AT+PRECONFG=2,...` و`AT+RFBYCODE=...` يغيّران إعدادات المشغل/المنطقة وقد يحتاجان إعادة تشغيل.
- `AT+SIMLOCKU` و`AT+SIMLGDATA` و`AT+UNLOCKSPR` تتعامل مع بيانات قفل؛ الخطأ قد يقفل الجهاز.
- `SRELOCK1` عملية إعادة قفل مشغل مدفوعة/خادمية؛ لا تُستخدم إلا على جهاز تملكه وبعد فهم المخاطر.

---

**نهاية الملف.**  
**الملف المسطح القابل للـgrep:** `MCC_MNC_COMMANDS.txt`  
**البيانات:** `MCC_MNC_LIST.csv`  
**مكتبات RIL:** `LIBRIL_COMMANDS.md`
