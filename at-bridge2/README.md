# GsmTools AT Bridge

> Samsung AT RIL Command Bridge for modern Samsung Galaxy devices

## 📋 ما هو؟

ملف تنفيذي (binary) يعمل على أجهزة سامسونج ويسمح بتمرير أوامر AT إلى المودم عبر ADB Shell. بديل مستقل لا يعتمد على أي أداة خارجية.

## 🏗️ بنية المشروع

```
at-bridge/
├── jni/
│   ├── at_bridge.c        # الكود المصدري الرئيسي (C)
│   ├── Android.mk         # ملف بناء NDK
│   └── Application.mk     # إعدادات NDK
├── build.sh               # سكربت بناء (Linux/macOS/Server)
├── build.bat              # سكربت بناء (Windows)
└── README.md              # هذا الملف
```

## 🔧 متطلبات البناء

- **Android NDK** r21 أو أحدث
- تحميل: https://developer.android.com/ndk/downloads

## 🚀 البناء

### على Windows:
```batch
set ANDROID_NDK_HOME=C:\path\to\ndk
build.bat
```

### على Linux/macOS/Server:
```bash
export ANDROID_NDK_HOME=/path/to/ndk
chmod +x build.sh
./build.sh
```

### النتيجة:
```
at-bridge/libs/arm64-v8a/AT    # الملف الثنائي الجاهز
```

## 📱 الاستخدام

### 1. رفع الملف إلى الجهاز
```bash
adb push libs/arm64-v8a/AT /data/local/tmp/AT
adb shell chmod 755 /data/local/tmp/AT
```

### 2. التشغيل
```bash
# الوضع التفاعلي
adb shell /data/local/tmp/AT

# تغيير CSC مباشرة
adb shell /data/local/tmp/AT -c XAA

# إرسال أمر hex مباشر
adb shell /data/local/tmp/AT -x 060100090000584141

# إرسال أمر AT
adb shell /data/local/tmp/AT -a "AT+CLCK=?"

# فحص الأجهزة المتاحة
adb shell /data/local/tmp/AT -s
```

### 3. الأوامر التفاعلية
```
#> HELP              عرض المساعدة
#> STATUS            معلومات الجهاز
#> SCAN              فحص أجهزة المودم المتاحة
#> AT+CLCK=?         أمر AT خام
#> HEX:0601...       أمر IPC بصيغة hex
#> IPC:5841410000    بناء حزمة IPC تلقائياً
#> CSC:XAA           تغيير CSC
#> CLEANUP           تنظيف التطبيقات
#> VERBOSE           تفعيل/إيقاف التشخيص
#> RECONNECT         إعادة الاتصال بالمودم
#> EXIT              خروج
```

## 🔌 قنوات الاتصال بالمودم

البرنامج يحاول فتح المودم بالترتيب التالي:

| الأولوية | القناة | الوصف |
|---|---|---|
| 1 | `/dev/umts_atc0` | Samsung AT Channel (مباشر) |
| 2 | `/dev/smd11` | Qualcomm SMD (Snapdragon) |
| 3 | `/dev/at_distributor` | Samsung AT Distributor |
| 4 | `/dev/socket/rild` | RIL Socket |
| 5 | TCP localhost:2600 | Companion App Proxy |

## 🔑 أكواد CSC الشائعة

| الكود | الشبكة |
|---|---|
| `XAA` | AT&T Open/Unlocked |
| `XAG` | T-Mobile Open |
| `XAS` | Sprint Open |
| `XAB` | Verizon Open |
| `ATT` | AT&T Branded |
| `TMB` | T-Mobile Branded |
| `SPR` | Sprint Branded |
| `VZW` | Verizon Branded |

## ⚡ بناء ورفع سريع
```bash
# أمر واحد: بناء + رفع + تشغيل
./build.sh deploy-run
# أو على ويندوز
build.bat deploy-run
```
