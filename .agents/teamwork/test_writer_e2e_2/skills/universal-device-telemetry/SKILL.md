---
name: universal-device-telemetry
description: Comprehensive operational guide and engineering framework for zero-loss, universal monitoring of mobile servicing and GSM tools. Captures 100% of raw USB, Serial/COM, AT commands, process executions, network traffic, dropped files, and unknown binary payloads without selective filtering or blind spots.
---

# Universal Device & Tool Telemetry (الالتقاط الشامل والحيادي لحركة الأدوات)

دليل هندسي وعملياتي لبناء وتشغيل طبقة مراقبة شاملة وعديمة الفقدان (**Zero-Drop, Non-Selective Monitoring**) ترصد جميع العمليات والأوامر وحزم البيانات التي تنفذها برامج وأدوات الصيانة والفحص والأمن السيبراني، دون الاقتصار على أوامر محددة أو بروتوكولات مسبقة التعريف.

---

## 1. الفلسفة الأساسية: المراقبة الشاملة عديمة الفقدان (Zero-Drop Telemetry)

تعتمد معظم الأدوات المتقدمة (GSM Tools, Exploits, Forensic bypasses) على تقنيات التخفي لتجاوز أنظمة المراقبة التقليدية:
- إرسال أوامر AT غير قياسية أو مشفرة عبر منافذ COM.
- استخدام بروتوكول ADB المضمن في الذاكرة (In-Memory Wire Protocol) دون تشغيل `adb.exe`.
- التخاطب المباشر مع معالجات الهواتف عبر USB Endpoints وحزم خام (Raw Hex / Vendor BROM / EDL).
- استخراج ملفات وحقن سكربتات في المجلدات المؤقتة `%TEMP%` وحذفها فوراً بعد التنفيذ.

**القاعدة الذهبية للنظام:**
> **لا يتم استثناء أو تجاهل أي بايت.** أي حركة تمر بين الأداة والكمبيوتر أو الهاتف يتم التقاطها وتأطيرها وحفظها، سواء كانت أمراً معروفاً أو حزمة بيانات ثنائية مجهولة (Unknown Binary Stream).

---

## 2. طبقات الاستشعار والالتقاط الشامل (Multi-Vector Capture Architecture)

```mermaid
flowchart TD
    subgraph ToolSpace["فضاء الأداة (Target Tool Execution)"]
        TOOL["الأداة / البرنامج المشبوه<br/>(Process / Child Threads)"]
    end

    subgraph CaptureSensors["حساسات الالتقاط الشامل (Universal Sensors)"]
        USB["1. كيرنل USB و ETW<br/>(PhoenixUsbMonitor.sys / UsbEtwSensor)<br/>100% من حزم الـ URB الخام"]
        COM["2. المنافذ التسلسلية والمودم<br/>(SerialPortSensor)<br/>100% من تيار TX/RX ومنافذ COM"]
        PROC["3. مراقبة العمليات والنظام<br/>(ProcessMonitor / ETW Process)<br/>شجرة العمليات وسطور الأوامر"]
        FILE["4. استخراج الملفات المؤقتة<br/>(TempFileAtSensor / FileSystemWatcher)<br/>مراقبة %TEMP% والملفات المستخرجة"]
        NET["5. شبكة الاتصال والبروكسي<br/>(AdbTransparentProxy / AppleUsbmuxd)<br/>منافذ 5037 و 27015 والشبكة"]
        VAULT["6. مستودع الملفات أثناء الطيران<br/>(AdbPushArtifactSensor)<br/>نسخ فوري لأي ملف يُرسل للهاتف"]
    end

    subgraph CorePipeline["نواة التجميع والتحليل (OmniTrace Engine)"]
        BUS["ناقل الأحداث المركزي<br/>(OmniTraceEventBus)"]
        REASM["مُعيد تجميع التيارات<br/>(TransportReassembler)"]
        DET["مكتشف ومفكك البروتوكولات<br/>(ProtocolDetector & Decoders)"]
        RAW["حاضنة الحزم المجهولة<br/>(Raw Frame Vault - Hex/Entropy)"]
        CORR["محرك الربط والنسبة<br/>(CommandCorrelationEngine - PID Attribution)"]
    end

    TOOL --> USB
    TOOL --> COM
    TOOL --> PROC
    TOOL --> FILE
    TOOL --> NET
    TOOL --> VAULT

    USB --> BUS
    COM --> BUS
    PROC --> BUS
    FILE --> BUS
    NET --> BUS
    VAULT --> BUS

    BUS --> REASM
    REASM --> DET
    DET -->|بروتوكول معروف| CORR
    DET -->|بروتوكول مجهول| RAW
    RAW --> CORR
```

---

## 3. تفاصيل متجهات الالتقاط (The 6 Capture Vectors)

### المتجه 1: التقاط حزم الـ USB على مستوى الكيرنل (Kernel USB URB Sniffing)
* **المسؤول:** `PhoenixUsbMonitor.sys` و `UsbEtwSensor` و `UsbPcap`.
* **نطاق الالتقاط:**
  - التقاط جميع حزم URB (USB Request Blocks) لكل من:
    - **Bulk OUT / Bulk IN:** قنوات نقل البيانات السريعة ونقل الفلاشات والملفات.
    - **Control Transfers:** إعدادات الجهاز وتبديل الوضعيات وأوامر المصنع.
    - **Interrupt Transfers:** إشعارات الأجهزة وحالات التغيير.
* **الضمان:** لا يمكن لأي برنامج تجاوز طبقة الكيرنل؛ حتى لو كتب المطور درايفر خاص، يتم تسجيل كل حزمة قبل خروجها للناقل الفعلي.

### المتجه 2: الالتقاط الكامل لمنافذ الـ COM والمودم (Raw Serial I/O)
* **المسؤول:** `SerialPortSensor`.
* **نطاق الالتقاط:**
  - مراقبة كافة منافذ الـ COM الافتراضية والحقيقية (`\\.\COM1` إلى `\\.\COM255`).
  - اعتراض تيار البيانات الثنائي (Raw Byte Stream) في الاتجاهين (TX من الكمبيوتر إلى الهاتف، و RX من الهاتف إلى الكمبيوتر).
  - لا يعتمد الحساس على فحص أوامر معينة، بل يسجل:
    - أوامر الـ AT النصية الكاملة (بما فيها الأوامر الاحتكارية الخاصة بسامسونج وهواوي وميديا تيك).
    - الحزم الثنائية الثابتة (Diagnostic Frames / HDLC Frames / Binary Modems).

### المتجه 3: مراقبة تنفيذ العمليات وشجرة الأنساب (Process Tree & Lineage)
* **المسؤول:** `ProcessMonitor` و `Windows ETW (Microsoft-Windows-Kernel-Process)`.
* **نطاق الالتقاط:**
  - كل عملية يتم إطلاقها: المسار الكامل، المعرف البرمجي (`ProcessId`)، المعرف الأب (`ParentProcessId`).
  - سطر الأوامر الكامل (`CommandLine`) مع كافة المعاملات والـ Flags.
  - مراقبة العمليات الخفية المحقونة أو المحمولة التي يتم إنشاؤها عبر `CreateProcess`, `ShellExecute`, أو `cmd.exe /c`.

### المتجه 4: مراقبة وتجميد الملفات المستخرجة محلياً (Staging & Dropper Vault)
* **المسؤول:** `TempFileAtSensor` و مراقب مسارات النظام.
* **المسارات الخاضعة للرقابة:**
  - `%LOCALAPPDATA%\Temp\`
  - `%USERPROFILE%\AppData\Roaming\`
  - `C:\ProgramData\`
  - مسار الأداة نفسه والمجلدات الفرعية.
* **آلية الرصد الفوري:**
  - رصد أحداث `Created`, `Changed`, `Renamed`.
  - بمجرد إنشاء الملف، يتم عمل **نسخة فورية (Forensic Snapshot)** منه وحساب الـ Hash (SHA256) قبل أن تقوم الأداة بحذفه.

### المتجه 5: وكيل البروتوكولات والشبكة الذكي (Transparent Proxies & Network)
* **المسؤول:** `AdbTransparentProxy` و `AppleUsbmuxdProxy` و `NetworkUrlSensor`.
* **نطاق الالتقاط:**
  - اعتراض منفذ `127.0.0.1:5037` (خادم ADB المحلي): تسجيل جميع أوامر الـ Smart Sockets، وأوامر `shell`, `exec`, `push`, `pull`, `sync`.
  - اعتراض منفذ `127.0.0.1:27015` (خادم Usbmuxd لأجهزة آبل): تسجيل حزم الـ Plist و Lockdown و DFU.
  - مراقبة اتصالات الـ TCP الصادرة لرصد خوادم الترخيص (License servers) أو خوادم التنزيل التلقائي لملفات الفلاش.

### المتجه 6: مستودع الحمولات أثناء الطيران (In-Flight Artifact Vault)
* **المسؤول:** `AdbPushArtifactSensor`.
* **الهدف:** إذا قامت الأداة بإرسال ملف تنفيذي أو Payload إلى الهاتف (مثل `su`, `frida-server`, `busybox`, أو ملفات `.sh` أو `.apk`):
  - يتم استخراج الملف وتخزينه في مستودع الأدلة الجنائية للمعاينة دون إيقاف العملية.

---

## 4. استراتيجية تفكيك وتحليل الحزم (Universal Decoding & Unknown Payload Handling)
...
