# القاموس الشامل الكامل للأوامر — نسخة موثّقة نهائية

**التاريخ:** 23/10/2026 — **الهدف:** استخراج *كل* الأوامر بالكامل، بلا استثناء، مع إثبات المصدر.

## منهجية الإثبات
1. **AT (الملف الأساسي):** شُغّل على الهاتف SM-G715U1، جُمّد، وفُرّغت ذاكرته. النصوص تُفكّ في مكانها وقت التشغيل.
2. كل نص في الرام قُوبل مع القرص: `key = disk_byte XOR ram_byte` — ومنه استُخرج مفتاح كل نص.
3. المراجع من الكود حُسبت من بايتات التعليمات نفسها (ADRP+ADD) — لا تخمين.
4. **إثبات الاكتمال:** لا يوجد أي مقطع نصي (≥8 أحرف) بقي مشفراً في `.data` — كل المقاطع القابلة للطباعة في الرام كانت مفكوكة.
5. أوامر المودم `AT+...` **ليست في ملف AT إطلاقاً** (لا في القرص ولا في الرام) — مصدرها الحقيقي كود `at-bridge` (C) وهو مُستخرج كاملاً أدناه.

---

## 1) AT — أوامر ونصوص الأداة (كاملة ومفكوكة من الرام)

إجمالي النصوص الفريدة في الرام: **141** — بعد استبعاد الضجيج ومقاطع C++: **123** نصاً/أمراً.

> **ملاحظة معمارية مهمة:** الأداة تحتوي البادئة `AT+` (العنوان `0x2aa424`) وتحمّل مكتبة RIL وتستدعي `send_recv_cmd`،
> لكن **أسماء أوامر المودم نفسها غير مخزّنة داخلها** — تُبنى ديناميكياً أو تأتي من جسر at-bridge على الكمبيوتر عبر منفذ 2600.
> لهذا فإن أوامر `AT+PRECONFG` / `AT+CLCK` / ... موجودة في كود at-bridge (القسم 2) وليست في ملف AT.
>
> **ملحق MCC/MNC:** أوامر قراءة MCC/MNC الفعلية (`AT+DEVCONINFO` / `AT+CIMI` / خصائص أندرويد)، وحزمة `MCCREQ`، وأمر `SRELOCK` الداخلي، موثقة في **القسم 6**.

### البانر وهوية الأداة (14)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `Model :` | `0x272a98` | `0xc9` | FUN_0013840c |
| 2 | `  CPU :` | `0x272aa3` | `0x56` | — |
| 3 | `  Product Name  :` | `0x272ab3` | `0x6b` | — |
| 4 | `   Serial :` | `0x272aca` | `0x6b` | — |
| 5 | ` Security patch :` | `0x272ae3` | `0x49` | — |
| 6 | `****************************************************` | `0x272b00` | `0x5d` | FUN_0013b87c, FUN_00150730 |
| 7 | `******        HUNTER-UNLOCKER TOOL 2025       ******` | `0x272b35` | `0x5d` | — |
| 8 | `******                                        ******` | `0x272b6a` | `0x5d` | — |
| 9 | `******           SAM RIL  V2.3 2025           ******` | `0x272b9f` | `0x5d` | — |
| 10 | ` [+] Product Manufacturer  : %s` | `0x2730f1` | `0x87` | — |
| 11 | ` [+] Product Model         : %s` | `0x273141` | `0x84` | — |
| 12 | ` [+] Product Serial        : %s` | `0x273181` | `0x29` | — |
| 13 | ` [+] Build  Number         : %s` | `0x2731d1` | `0x3c` | — |
| 14 | ` [+] SDK Version           : %s` | `0x273221` | `0x1c` | — |

### خصائص النظام المقروءة (9)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `ro.product.manufacturer` | `0x2730d0` | `0xcb` | FUN_00156f44, FUN_00159e84, FUN_0020a284 |
| 2 | `ro.product.model` | `0x273120` | `0x52` | FUN_00156f44, FUN_00159e84, FUN_0020a284 |
| 3 | `ro.serialno` | `0x273168` | `0x4e` | FUN_00156f44, FUN_00161334, FUN_00204730 |
| 4 | `ro.build.version.incremental` | `0x2731b0` | `0xf6` | FUN_00156f44, FUN_0020a284, FUN_00212e50 |
| 5 | `ro.build.version.sdk` | `0x273200` | `0x3d` | FUN_00156f44, FUN_0020a284, FUN_00212e50 |
| 6 | `ro.build.version.release` | `0x2ac130` | `0x32` | FUN_0020a284, FUN_00212e50, FUN_0021420c |
| 7 | `ro.build.version.security_patch` | `0x2ac150` | `0xfe` | FUN_0020a284, FUN_00212e50 |
| 8 | `ro.boot.debug_level` | `0x2ac1b0` | `0x19` | FUN_0020a284, FUN_00212e50 |
| 9 | `ro.boot.cp_debug_level` | `0x2ac1d0` | `0x17` | FUN_0020a284, FUN_00212e50 |

### حزم وأنشطة سامسونج (4)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `com.samsung.rms.retailagent.global` | `0x2aa4e0` | `0x76` | FUN_0016d718, FUN_00204730 |
| 2 | `com.sdet.fotaagent` | `0x2aa510` | `0xd` | FUN_0016d718, FUN_00204730 |
| 3 | `com.sec.android.app.audiocoredebug` | `0x2aa670` | `0x41` | FUN_0016efc4, FUN_001790d8, FUN_00204730 |
| 4 | `com.samsung.android.FactoryTestLauncher` | `0x2aaba0` | `0x46` | FUN_0016efc4, FUN_00204730 |

### أوامر الشل والأتمتة (31)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `input keyevent 4 > /dev/null 2>&1` | `0x2732f0` | `0x5e` | FUN_0015890c |
| 2 | `ps -a 2>/dev/null\|grep logcat 2>/dev/null` | `0x273320` | `0x38` | FUN_0015890c |
| 3 | `logcat` | `0x273350` | `0xa7` | FUN_0015890c |
| 4 | `kill pid ` | `0x273360` | `0x9d` | FUN_0015890c |
| 5 | `logcat -b system,main -c > /dev/null 2>&1` | `0x273380` | `0x4c` | FUN_0015890c |
| 6 | `logcat -b radio -c > /dev/null 2>&1` | `0x2733b0` | `0x7f` | FUN_0015890c |
| 7 | `echo ` | `0x2aa308` | `0xa6` | FUN_00161334, FUN_00204730 |
| 8 | ` \| /system/bin/toybox nc 127.0.0.1 2600 > /dev/null 2>&1` | `0x2aa310` | `0x4e` | FUN_00161334, FUN_00204730 |
| 9 | `echo "am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1 ` | `0x2aa390` | `0x7f` | FUN_00161334, FUN_00204730 |
| 10 | `pclose` | `0x2aa404` | `0xa1` | FUN_00161334, FUN_00204730 |
| 11 | `pm path ` | `0x2aa4b8` | `0x15` | FUN_00174704, FUN_00204730 |
| 12 | `pm list packages \| grep com.skatd.unlock` | `0x2aa530` | `0xd8` | FUN_00175498, FUN_00204730 |
| 13 | `am force-stop ` | `0x2aa578` | `0xfc` | FUN_00175498, FUN_00204730 |
| 14 | `input tap ` | `0x2aa5e8` | `0x13` | FUN_00177d90, FUN_00178bc4, FUN_00204730 |
| 15 | `input keyevent 3 > /dev/null 2>&1` | `0x2aa6a0` | `0x13` | FUN_001790d8, FUN_00204730 |
| 16 | `settings put system screen_off_timeout 600000 > /dev/null 2>&1` | `0x2aa6d0` | `0x24` | FUN_001790d8, FUN_00204730 |
| 17 | `settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1` | `0x2aa710` | `0x66` | FUN_001790d8, FUN_00204730 |
| 18 | `settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1` | `0x2aa7a0` | `0xe6` | FUN_001790d8, FUN_00204730 |
| 19 | `settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1` | `0x2aa830` | `0x9a` | FUN_001790d8, FUN_00204730 |
| 20 | `pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1` | `0x2aa8e0` | `0x22` | FUN_001790d8, FUN_00204730 |
| 21 | `input keyevent 219 > /dev/null 2>&1` | `0x2aa920` | `0xcf` | FUN_001790d8, FUN_00182684, FUN_00204730 |
| 22 | `rm -rf /sdcard/window_dump.xml > /dev/null 2>&1` | `0x2aa950` | `0xd5` | FUN_001790d8, FUN_00204730 |
| 23 | `uiautomator dump /sdcard/window_dump.xml > /dev/null 2>&1` | `0x2aa980` | `0x18` | FUN_001790d8, FUN_00182684, FUN_00204730 |
| 24 | `cat /sdcard/window_dump.xml` | `0x2aa9c0` | `0xd4` | FUN_001790d8, FUN_00182684, FUN_00204730 |
| 25 | `input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'` | `0x2aa9e0` | `0xed` | FUN_001790d8, FUN_00182684, FUN_00204730 |
| 26 | `input keyevent 61 > /dev/null 2>&1` | `0x2aaa30` | `0x2b` | FUN_001790d8, FUN_00204730 |
| 27 | `input keyevent 66 > /dev/null 2>&1` | `0x2aaa60` | `0x89` | FUN_001790d8, FUN_00182684, FUN_00204730 |
| 28 | `pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1` | `0x2aaa90` | `0x20` | FUN_00182684, FUN_00204730 |
| 29 | `am start -a android.settings.SETTINGS > /dev/null 2>&1` | `0x2aaaf0` | `0x91` | FUN_00182684, FUN_00204730 |
| 30 | `cmd wifi set-wifi-enabled disabled > /dev/null 2>&1` | `0x2aab30` | `0xc3` | FUN_0016efc4, FUN_00204730 |
| 31 | `input keyevent 82 > /dev/null 2>&1` | `0x2aab70` | `0xc1` | FUN_0016efc4, FUN_00204730 |

### مسارات وقنوات (5)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `/proc/self/status` | `0x272a70` | `0x41` | FUN_00150730 |
| 2 | `/proc/self/exe` | `0x273248` | `0x28` | FUN_00154d68, FUN_00156f44 |
| 3 | `%s%s` | `0x273258` | `0x5e` | FUN_00156f44 |
| 4 | `shell ` | `0x273358` | `0xb8` | FUN_0015890c |
| 5 | `package:` | `0x2aa4c8` | `0xf1` | FUN_00174704, FUN_00175498, FUN_00204730 |

### واجهة RIL والمكتبة (13)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `RIL Exit` | `0x2aa010` | `0xfb` | FUN_0015f304, FUN_0016b3c4, FUN_0016b604 |
| 2 | `[-] Failed to load library %s: %s` | `0x2aa030` | `0x18` | FUN_0016b3c4, FUN_0016b604, FUN_0016bc88 |
| 3 | `send_recv_cmd` | `0x2aa058` | `0xed` | FUN_0016b574, FUN_00185ff0, FUN_001878b0 |
| 4 | `[-] Failed to find symbol 'send_recv_cmd' in %s: %s` | `0x2aa070` | `0x64` | FUN_00185d3c, FUN_00204730, FUN_002094e4 |
| 5 | `[i] Found 'send_recv_cmd' in %s.` | `0x2aa0b0` | `0xf8` | FUN_00204730, FUN_0020abd0, FUN_0020e7a4 |
| 6 | `[-] Library %s did not return a valid response (code: %d, len: %d).` | `0x2aa0e0` | `0x1a` | FUN_00204730, FUN_0020abd0 |
| 7 | `ERORR: Connect , Try Again` | `0x2aa282` | `0xb6` | — |
| 8 | `AT+` | `0x2aa424` | `0x2d` | FUN_00161334, FUN_00204730 |
| 9 | `EXIT` | `0x2aa43c` | `0x40` | FUN_00161334, FUN_00204730 |
| 10 | ` [*] RIL Error ex1 [*] ` | `0x2aa482` | `0xe4` | — |
| 11 | `  [+] Ril Error 1 ` | `0x2aa4a2` | `0xa8` | — |
| 12 | `Worng Ril Command` | `0x2ac030` | `0x19` | FUN_002088f0, FUN_0020a130, FUN_0020abd0 |
| 13 | `Ril Token Error, Try Again` | `0x2ac1f2` | `0x5b` | FUN_0021b034, FUN_0021ba10 |

### المصادقة والتوكن والتشفير (16)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `120d` | `0x2aa01c` | `0x72` | FUN_0015f6ac, FUN_0018ada4, FUN_00204730 |
| 2 | `[SUCCESS] Challenge key (CALC) obtained from %s: %s` | `0x2aa181` | `0x89` | — |
| 3 | `[SECURE] Encrypted CALC for use: %s` | `0x2aa1c0` | `0x6d` | FUN_00204730 |
| 4 | `B1EEBF38E789633D222F9040418EF4CBD5536B871BC8866AAEFE655E60C57295` | `0x2aa1f0` | `0x1b` | FUN_00161334, FUN_00204730, FUN_0020edd4 |
| 5 | `XgvkwKl8K6T8eAGsvyE2ydmLgBOpjk0F` | `0x2aa2b0` | `0xdd` | FUN_00161334, FUN_00204730 |
| 6 | `SETSPR` | `0x2aa2d4` | `0xaf` | FUN_00161334, FUN_00204730 |
| 7 | `5119010532` | `0x2aa2e8` | `0x90` | FUN_00161334, FUN_00204730 |
| 8 | `01426X1` | `0x2aa2f8` | `0x32` | FUN_00161334 |
| 9 | `01426x1` | `0x2aa300` | `0x4f` | FUN_00161334 |
| 10 | `SRELOCK` | `0x2aa350` | `0x44` | FUN_00161334 |
| 11 | ` Remove KG Done ` | `0x2aa372` | `0xd2` | — |
| 12 | `  Open Done ` | `0x2aa412` | `0x94` | — |
| 13 | `GETSPR` | `0x2aa428` | `0x2e` | FUN_00161334, FUN_00204730 |
| 14 | `51180004` | `0x2aa430` | `0x7` | FUN_00161334, FUN_00204730 |
| 15 | `000000000000` | `0x2aa448` | `0x50` | FUN_00161334, FUN_00204730 |
| 16 | `OK:%s` | `0x2aa472` | `0xf4` | — |

### ترميز Base64/Hex (5)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `%02X` | `0x2ac210` | `0x22` | FUN_00212e50 |
| 2 | `%02x` | `0x2ac274` | `0xdd` | FUN_0020a284, FUN_0020dc7c, FUN_00212e50 |
| 3 | `ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/` | `0x2ac690` | `0x48` | FUN_00227cf0 |
| 4 | `ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_` | `0x2ac6e0` | `0x6b` | FUN_00227cf0 |
| 5 | `Input is not valid base64-encoded data.` | `0x2ac730` | `0x80` | FUN_00227360, FUN_00227cf0 |

### APK المدمج (Base64) (1)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `UEsDBAoAAAgAAAAAITp9Yo4VYgYAAGIGAAAZAAUAcmVzL2RyYXdhYmxlL2ZkbV9sb2dvLnBuZ/7KAAAAiVBORw0KGgoAAAANSUhEUgAAAKcAAAA5CAQAAAD5... [Base64 APK — الطول 224092 حرفاً]` | `0x273430` | `0x47` | FUN_00158f88, FUN_00204730 |

### رسائل وأخطاء (10)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `allocator<T>::allocate(size_t n) 'n' exceeds maximum supported size` | `0x272c50` | `0x5d` | FUN_00150730, FUN_00156f44, FUN_0015890c |
| 2 | `Error: popen() failed!` | `0x2a9fa0` | `0x99` | FUN_00204730 |
| 3 | `Error: No log entry found.` | `0x2a9fc0` | `0x4e` | FUN_00204730 |
| 4 | `Error: Could not find [...] in log entry.` | `0x2a9fe0` | `0x78` | FUN_0018a244, FUN_0018ac6c, FUN_0018af38 |
| 5 | `ERROR: %s` | `0x2aa3fa` | `0xa4` | — |
| 6 | `<node[^>]*text="EXECUTE"[^>]*bounds="\[([0-9]+),([0-9]+)\]\[([0-9]+),([0-9]+)\]"[^>]*>` | `0x2aa590` | `0x3f` | FUN_00177d90, FUN_00204730 |
| 7 | `<node[^>]*resource-id="android:id/aerr_wait"[^>]*bounds="\[([0-9]+),([0-9]+)\]\[([0-9]+),([0-9]+)\]"` | `0x2aa600` | `0x4a` | FUN_00178bc4, FUN_00204730 |
| 8 | `EXECUTE` | `0x2aaa20` | `0x1a` | FUN_001790d8 |
| 9 | `aerr_wait` | `0x2aaad8` | `0x75` | FUN_00182684, FUN_00204730 |
| 10 | `thread constructor failed` | `0x2aac50` | `0x1a` | FUN_0017510c, FUN_00204730 |

### أخرى / غير مصنفة (15)

| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |
|---:|---|---|---|---|
| 1 | `TracerPid:` | `0x272a88` | `0x41` | FUN_00150730 |
| 2 | ` 2>/dev/null` | `0x273370` | `0x63` | FUN_0015890c |
| 3 | `%d-%d-%d` | `0x2a9f90` | `0xa0` | FUN_001597e8, FUN_00204730 |
| 4 | `[-] Failed to extract a valid key from %s's response.` | `0x2aa140` | `0x48` | FUN_00204730 |
| 5 | `ERORR` | `0x2aa1e8` | `0xef` | FUN_00160450, FUN_00160a40, FUN_00204730 |
| 6 | `52 i32 0 ` | `0x2aa238` | `0x21` | FUN_00161334, FUN_00204730 |
| 7 | `131 ` | `0x2aa244` | `0x8d` | FUN_00161334, FUN_00204730 |
| 8 | `128 ` | `0x2aa24c` | `0x1a` | FUN_00161334, FUN_00204730 |
| 9 | `132 ` | `0x2aa254` | `0xa` | FUN_00161334, FUN_00204730, FUN_0020a284 |
| 10 | `136 ` | `0x2aa25c` | `0xc` | FUN_00161334, FUN_00204730 |
| 11 | `130 ` | `0x2aa264` | `0x4f` | FUN_00161334, FUN_00204730 |
| 12 | `135 ` | `0x2aa26c` | `0x7d` | FUN_00161334, FUN_00204730 |
| 13 | `setspr` | `0x2aa2dc` | `0x60` | FUN_00161334, FUN_00204730 |
| 14 | `srelock` | `0x2aa358` | `0x5e` | FUN_00161334 |
| 15 | ` > /dev/null 2>&1` | `0x2aa560` | `0x25` | FUN_00174fd4, FUN_00175498, FUN_00204730 |

### الأوامر المركّبة ديناميكياً (قوالب تُبنى من المقاطع أعلاه)

| القالب | المقاطع المستخدمة | الدالة |
|---|---|---|
| `pm path <PACKAGE>` | `pm path ` (0x2aa4b8) + `package:` (0x2aa4c8) | FUN_0016d718 |
| `am force-stop <PACKAGE>` | `am force-stop ` (0x2aa578) + `<package>` | FUN_0016d718 |
| `kill pid <PID>` | `kill pid ` (0x273360) + PID | FUN_0015890c |
| `echo <AT...> \| /system/bin/toybox nc 127.0.0.1 2600` | `echo ` (0x2aa308) + `AT+` (0x2aa424) + ` \| /system/bin/toybox nc ...` (0x2aa310) | FUN_00161334 |
| `input tap <X> <Y>` | `input tap ` (0x2aa5e8) + إحداثيات من regex الـ XML | FUN_001790d8 |
| `uiautomator dump` → `cat` → `input tap` → `rm -rf` | سلسلة أتمتة الواجهة كاملة | FUN_001790d8 / FUN_00182684 |

### إثبات الاكتمال (أرقام)

- 141 نصاً فريداً في الرام (كل ما فُكّ عند الإقلاع).
- 518 مرجعاً مباشراً من التعليمات (ADRP+ADD) إلى 115 نصاً.
- **0** مقاطع قابلة للطباعة (≥8 أحرف) بقيت مشفّرة في `.data` — أي لا يوجد نص مخفي لم يُفك.
- **0** أوامر في `.rodata` — كل الأوامر في `.data` وقد فُكّت.
- 230,263 بايت تغيّرت في `.data` بين القرص والرام = مناطق فك التشفير (منها كتلة APK بمفتاح 0x47).

---

## 2) at-bridge — أوامر المودم والبروتوكول (من كود المصدر C)

المصدر: `gsmtools/at-bridge/jni/all_gsm_commands.h` + `at_bridge.c` — هذه هي أوامر `AT+` التي وردت في القاموس القديم، ومكانها الحقيقي هنا.

### 2.1 أوامر AT للمودم
| الثابت | الأمر |
|---|---|
| `AT_CMD_TEST` | `AT\r\n` |
| `AT_CMD_PRECONFG_READ` | `AT+PRECONFG=1,0\r\n` |
| `AT_CMD_PRECONFG_WRITE_FMT` | `AT+PRECONFG=2,%s\r\n` |
| `AT_CMD_REBOOT_RADIO` | `AT+CFUN=1,1\r\n` |
| `AT_CMD_MODEM_FACTORY_RESET` | `AT+FACTORST=0,0\r\n` |
| `AT_CMD_DEVCONINFO` | `AT+DEVCONINFO\r\n` |
| `AT_CMD_KSTRINGB` | `AT+KSTRINGB=0,3\r\n` |
| `AT_CMD_IMEI_QUERY` | `+IMEITEST:1,\r\n` |
| `AT_CMD_SIM_STATUS` | `AT+CPIN?\r\n` |
| `AT_CMD_QUERY_NETWORK_LOCK` | `AT+CLCK=\"PN\",2\r\n` |
| `AT_CMD_QUERY_SUBSET_LOCK` | `AT+CLCK=\"PU\",2\r\n` |
| `AT_CMD_QUERY_SP_LOCK` | `AT+CLCK=\"PP\",2\r\n` |
| `AT_CMD_QUERY_CORP_LOCK` | `AT+CLCK=\"PC\",2\r\n` |
| `AT_CMD_QUERY_SIM_LOCK` | `AT+CLCK=\"SC\",2\r\n` |

### 2.2 أوامر الشل والتصعيد (نفس أوامر AT لكن موثقة كمصدر)
| الثابت | الأمر |
|---|---|
| `CMD_FLUSH_RADIO_LOGS` | `logcat -b radio -c > /dev/null 2>&1` |
| `CMD_SET_SCREEN_TIMEOUT` | `settings put system screen_off_timeout 600000 > /dev/null 2>&1` |
| `CMD_SET_ASSISTANT_SECURE` | `settings put Secure assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1` |
| `CMD_SET_ASSISTANT_SYSTEM` | `settings put system assistant com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1` |
| `CMD_SET_DOUBLEPRESS_GLOBAL` | `settings put global function_key_config_doublepress_value com.sec.android.app.audiocoredebug/com.sec.android.app.audiocoredebug.AudioAdbShellActivity > /dev/null 2>&1` |
| `CMD_LAUNCH_SETTINGS` | `am start -a android.settings.SETTINGS > /dev/null 2>&1` |
| `CMD_TRIGGER_ASSIST_KEY` | `input keyevent 219 > /dev/null 2>&1` |
| `CMD_INJECT_NC_ROOT_SHELL` | `input text 'toybox nc -s 127.0.0.1 -p 2600 -L /system/bin/sh'` |
| `CMD_KEYEVENT_ENTER` | `input keyevent 66 > /dev/null 2>&1` |
| `CMD_KEYEVENT_TAB` | `input keyevent 61 > /dev/null 2>&1` |
| `CMD_KEYEVENT_HOME` | `input keyevent 3 > /dev/null 2>&1` |
| `CMD_KEYEVENT_MENU` | `input keyevent 82 > /dev/null 2>&1` |
| `CMD_LAUNCH_SAMSUNG_HIDDEN_MENU` | `am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1` |
| `CMD_CHECK_SKT_UNLOCK` | `pm list packages | grep com.skatd.unlock` |
| `CMD_CLEAR_FACTORY_TEST` | `pm clear com.samsung.android.FactoryTestLauncher > /dev/null 2>&1` |
| `CMD_CLEAR_AUDIO_DEBUG` | `pm clear com.sec.android.app.audiocoredebug > /dev/null 2>&1` |
| `CMD_DUMP_UI_HIERARCHY` | `uiautomator dump /sdcard/window_dump.xml > /dev/null 2>&1` |
| `CMD_CAT_UI_HIERARCHY` | `cat /sdcard/window_dump.xml` |
| `CMD_CLEAN_UI_DUMP` | `rm -rf /sdcard/window_dump.xml > /dev/null 2>&1` |

### 2.3 الحزم والأنشطة
| الثابت | القيمة |
|---|---|
| `PKG_AUDIO_DEBUG` | `com.sec.android.app.audiocoredebug` |
| `ACT_AUDIO_DEBUG_SHELL` | `com.sec.android.app.audiocoredebug.AudioAdbShellActivity` |
| `PKG_HIDDEN_MENU` | `com.sec.hiddenmenu` |
| `ACT_IOT_HIDDEN_MENU` | `com.sec.hiddenmenu.IOTHiddenMenu` |
| `PKG_FACTORY_TEST` | `com.samsung.android.FactoryTestLauncher` |
| `PKG_RETAIL_MODE` | `com.samsung.rms.retailagent.global` |
| `PKG_SKT_UNLOCK` | `com.skatd.unlock` |
| `PKG_FOTA_AGENT` | `com.sdet.fotaagent` |

### 2.4 بروتوكول الجلسة التفاعلية والقيم المنطقية
| الثابت | القيمة |
|---|---|
| `PROTO_PREFIX_CSC` | `CSC:` |
| `PROTO_PREFIX_AT` | `AT:` |
| `PROTO_PREFIX_HEX` | `HEX:` |
| `PROTO_CMD_SCAN` | `SCAN` |
| `PROTO_CMD_STATUS` | `STATUS` |
| `PROTO_CMD_LOCKS` | `LOCKS` |
| `PROTO_CMD_RECONNECT` | `RECONNECT` |
| `PROTO_CMD_HELP` | `HELP` |
| `PROTO_CMD_EXIT` | `EXIT` |
| `PROTO_CMD_QUIT` | `QUIT` |
| `TELEMETRY_FORMAT_STRING` | `INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s\n` |

### 2.5 نصوص التشغيل الفعلية في at_bridge.c (مسارات، أوامر، ردود، مساعدة)
```text
[+] UID 1000 Escalation Bridge confirmed on port %d\n
/dev/umts_ipc0
/dev/umts_ipc1
/dev/umts_rfs0
/dev/diag
/dev/umts_atc0
/dev/umts_atc1
/dev/umts_at0
/dev/at_distributor
/dev/at_distributor0
/dev/smd11
/dev/smd0
/dev/smd7
/dev/ttyGS0
/dev/ttyACM0
/dev/ttyUSB0
/dev/socket/rild
/dev/socket/rild-oem
/dev/socket/rild-debug
/dev/socket/rild2
/dev/socket/qmux_radio/qmux_client_socket0
/dev/socket/
[+] %s: Found (TTY AT - %s)\n
Read/Write OK
[+] %s: Found (Direct IPC - %s)\n
[%c] %s: Found (%s)\n
%s\r\n
AT+CPIN?
AT+CLCK=\"PN\",2
AT+CLCK=\"PU\",2
AT+CLCK=\"PP\",2
AT+CLCK=\"PC\",2
ERROR
MODEM_ERROR
+CLCK: 0
+CLCK:0
+CLCK: 1
+CLCK:1
  SIM State (CPIN)             : %s\n
  Network Lock (CLCK:PN)       : %s\n
  Network Subset Lock (CLCK:PU): %s\n
  Service Provider Lock (PP)   : %s\n
  Corporate Lock (CLCK:PC)     : %s\n
SIMLOCK|CPIN=%s|PN=%s|PU=%s|PP=%s|PC=%s\n
[ERROR] Invalid CSC code: %s (must be 2-5 uppercase chars)\n
.ERR_CSC_INVALID..\n
[*] Starting Samsung CSC change to: %s (Hex: %s)\n
0601000E0000%s0000%s
  [1/5] Injecting: %s\n
060100090000%s
  [2/5] Injecting: %s\n
060100120000%s0000%s%s
  [3/5] Injecting: %s\n
  [4/5] Injecting: %s\n
  [5/5] Injecting: %s\n
[*] CSC change sequence completed for: %s\n
pm uninstall -k --user 0 com.sec.android.AutoPreconfig 2>/dev/null
pm uninstall -k --user 0 com.sec.omadmspr 2>/dev/null
logcat -b radio -c 2>/dev/null
[+] Samsung CSC change successful.\n
.OK_CSC..\n
INFO|MODEL=%s|CSC=%s|AND=%s|PATCH=%s|DBG=%s|UID=%d|CH=%s\n
  Product Brand  : %s\n
  Product Model  : %s\n
  Serial Number  : %s\n
  Device Carrier : %s\n
  Android Version: %s\n
  Security Patch : %s\n
  Debug Level    : %s\n
  Process UID    : %d\n
  Active Channel : %s (%s)\n
Usage: %s [OPTIONS]\n\n
One-Shot CLI Options:\n
  -c <CSC>       Quick Samsung CSC change (e.g. -c XAA)\n
  -a <AT_CMD>    Send single AT command and exit (e.g. -a AT+DEVCONINFO)\n
  -s             Scan mode: list accessible modem & RIL devices\n
  -d <DEVICE>    Force specific modem device path (e.g. -d /dev/umts_atc0)\n
  -p <PORT>      Escalation port (default: 2600)\n
Interactive Commands (run without options):\n
  CSC:<CODE>     Automated 5-packet Samsung CSC change\n
  AT:<CMD>       Send raw AT command (e.g. AT:AT+DEVCONINFO)\n
  HEX:<DATA>     Send hex IPC command (or raw hex without prefix)\n
  SCAN           Scan and report accessible modem channels\n
  STATUS         Display full device, channel and SIM lock status\n
  LOCKS          Query SIM & Network lock status (CPIN, CLCK)\n
  RECONNECT      Re-probe and reconnect best modem channel\n
  MODE:AT        Switch to AT text channel (/dev/umts_atc0)\n
  MODE:IPC       Switch to IPC / RIL socket channel (/dev/socket/rild)\n
  AUTH:<SEED>    Activate dynamic Xorshift32 stream cipher\n
  HELP           Show this interactive help\n
  EXIT, QUIT     Terminate bridge session\n
%s\n
.ERR_AT_TIMEOUT..\n
.ERR_IPC..\n
.ERR_HEX_SYNTAX..\n
EXIT
QUIT
HELP
SCAN
STATUS
LOCKS
RECONNECT
.CH=%s..\n<< 
AUTH:
.OK_AUTH..\n<< 
CSC:
MODE:AT
MODE:IPC
AT:
HEX:
.OK..%s#\n
.OK..#\n
.ERR_SYNTAX..\n
```

---

## 3) AT_unpacked.elf — النسخة الأخرى

عدد النصوص ذات الصلة: **31** — التفاصيل الكاملة في `AT_UNPACKED_COMMANDS.txt`.

أبرز ما تحتويه (رسائل المصادقة القديمة):
```text
Error: %s
Ril Token Error, Try Again
Wrong AuthKey Error
EXIT
 [+] Authantication Token : %s
RIL Exit
 > /dev/null 2>&1
Worng Ril Command
  [+] Ril Error 1 
echo "am start -n com.sec.hiddenmenu/.IOTHiddenMenu -e 7267864872 72678647376477466 > /dev/null 2>&1 
 [+] Enter Authantication Key : 
  [+] Ril Error 2 
ps -a 2>/dev/nul\&
ro.product.modell|grep logcat 2>G
-c > /dev/null 2
logcat -b systemlogcat -b radio 
```

---

## 4) التحقق من القاموس القديم — أين كل أمر فعلاً؟

| ما ورد في `ALL_COMMANDS_DICTIONARY.md` | مكانه الحقيقي | الحالة |
|---|---|---|
| أوامر الشل (settings / am / input / uiautomator / pm clear / nc) | AT + at-bridge | موجودة ومفكوكة (القسم 1) |
| `pm list packages | grep com.skatd.unlock` | AT (Ghidra 0x2aa530) | موجودة |
| `am force-stop` | AT (Ghidra 0x2aa578) | موجودة |
| `com.samsung.rms.retailagent.global` | AT (Ghidra 0x2aa4e0) | موجودة |
| `AT+PRECONFG` / `AT+CFUN` / `AT+FACTORST` | at-bridge (all_gsm_commands.h) | غير موجودة في AT — أُضيفت من المصدر |
| `AT+CPIN?` / `AT+CLCK=...` / `AT+DEVCONINFO` / `AT+KSTRINGB` | at-bridge | غير موجودة في AT — أُضيفت من المصدر |
| مسارات `/dev/umts_*` و `/dev/smd*` و `/dev/socket/rild*` | at-bridge (at_bridge.c) | غير موجودة في AT — أُضيفت من المصدر |
| خصائص `ro.csc.*` / `gsm.*` / `ril.*` / `persist.sys.usb.*` | at-bridge + خصائص أندرويد عامة | غالبها ليس في AT |
| واجهة `CSC:` / `AT:` / `HEX:` / `SCAN` / `STATUS` / `LOCKS` / `RECONNECT` | at-bridge | أوامر جسر وليست داخل AT |
| سطر `INFO|MODEL=...` | at-bridge | مستخرج من المصدر |
| رسائل `[+] Authantication Token` / `Wrong AuthKey Error` / `Ril Token Error` | AT_unpacked.elf | نسخة أخرى من الأداة |
| `Worng Ril Command` / `RIL Exit` / `Ril Error 1` | AT (Ghidra 0x2ac030/0x2aa010/0x2aa4a2) | موجودة |
| `send_recv_cmd` ورسائل تحميل المكتبة | AT (Ghidra 0x2aa058+) | موجودة |
| `01000101` | غير موجود حرفياً في AT | ثابت بروتوكول عام — لم يُعثر عليه |

---

## 5) إحصاءات نهائية

- نصوص AT المفكوكة: **141** (منها **123** أمر/نص ذو معنى).
- أوامر at-bridge من المصدر: **52** ثابتاً + **113** نصاً.
- نصوص AT_unpacked ذات الصلة: **31**.
- مفاتيح XOR لكل نص في AT موثقة في الجدول (القسم 1).
- ملفات الآلة: `AT_ALL_STRINGS.json` + `AT_XREF_HITS.json` + `AT_BRIDGE_EXTRACT.json`.

> كل سكربتات بايثون محفوظة: `extract_all_commands.py`, `decode_precise.py`, `final_strings.py`, `extract_apk.py`, `build_command_dictionary.py`.

---

## 6) ملحق MCC & MNC — الأوامر والخصائص والحزم

> **الملف التفصيلي:** `MCC_MNC_COMMANDS.md` — **القائمة المسطحة:** `MCC_MNC_COMMANDS.txt` — **البيانات:** `MCC_MNC_LIST.csv`.

### 6.0 الخلاصة

- `AT` لا يحتوي أي نص `MCC` / `MNC` / `COPS` / `CREG` / `CIMI` / `IMSI` — **0 من 141**.
- القراءة الفعلية: `AT+DEVCONINFO` (حقلا `MNC`/`MCC`) + `AT+CIMI` (IMSI) + `getprop gsm.sim.operator.numeric`.
- `SRELOCK` هو الأمر الداخلي الوحيد لإعادة قفل المشغل؛ يُستقبل عملياً كـ`SRELOCK1`، ويعتمد على قائمة `MCCREQ`.
- `MCCREQ` **قائمة اختيار** تُرسل للسيرفر، وليست قراءة من الجهاز.

### 6.1 أوامر AT المباشرة (مرصودة)

| الأمر | الوظيفة | العدد | ملاحظة |
|---|---|---:|---|
| `AT+DEVCONINFO` | قراءة `MNC(...);MCC(...)` وبيانات الجهاز | 35 | المصدر الأساسي — مثال: `MNC(01);MCC(421)` |
| `AT+SVCIFPGM=1,1` | Info 2 / Read Mobile Info | 16 | يُرسل بعد DEVCONINFO |
| `AT+CIMI` | قراءة IMSI | 34 | MCC (3) + MNC (2/3) + MSIN |
| `AT+CIMI=?` | استعلام النطاق | 27 | — |
| `AT+CPIN?` | جاهزية الشريحة | 29 | prerequisite قبل CIMI |
| `AT+CSQ` | شدة الإشارة | 38 | مساعد |
| `AT+CFUN?` / `AT+CFUN=1` | حالة الراديو | 27 / 30 | مساعد |
| `AT+CSCS` | مجموعة المحارف | 36 | مساعد |

**رد `AT+DEVCONINFO` حقيقي (UnlockTool RAM @ `0x22578D60`):**

```text
+DEVCONINFO: MN(SM-J260A);BASE(SM-N900);VER(...);HIDVER(...);MNC(01);MCC(421);PRD(ATT);;OMCCODE();SN(RF8M714T5WP);IMEI(...);UN(...)
```

> القيم قد تكون فارغة (`MNC();MCC();`) على أجهزة Wi-Fi فقط.

### 6.2 أوامر 3GPP المعيارية (غير مرصودة)

| الأمر | الوظيفة | الحالة |
|---|---|---|
| `AT+COPS?` | المشغل الحالي | معياري — غير مرصود |
| `AT+COPS=?` | سرد المشغلين مع MCC/MNC | معياري — غير مرصود |
| `AT+COPS=0` | اختيار تلقائي | معياري — غير مرصود |
| `AT+COPS=1,2,"42101"` | اختيار يدوي | معياري — غير مرصود |
| `AT+COPS=3,2` | صيغة numeric | معياري — غير مرصود |
| `AT+CREG?` | تسجيل GSM | معروف في المصنّف — غير مرصود |
| `AT+CGREG?` / `AT+CEREG?` | تسجيل GPRS / LTE | معياري — غير مرصود |
| `AT+CPOL?` | قائمة PLMN مفضلة | معياري — غير مرصود |
| `AT+CPLS=0/1/2` | اختيار قائمة PLMN | معياري — غير مرصود |

### 6.3 أوامر سامسونج/القفل (مرصودة)

| الأمر | الوظيفة | العدد | المصدر |
|---|---|---:|---|
| `AT+PRECONFG=2` | إعداد مسبق للمشغل/المنطقة | 18 | UnlockTool / Q-Unlocker |
| `AT+PRECONFG=2,0` | إعداد مسبق بقيمة 0 | 2 | Q-Unlocker |
| `AT+PRECONFG=1,0` | قراءة preconfig | — | at-bridge:79 |
| `AT+RFBYCODE=2,0,XAA` | تغيير CSC/Region | 2 | Q-Unlocker `0x01BF8AB1` |
| `AT+RFBYCODE=2,1,XAA` | تغيير CSC/Region | 4 | Q-Unlocker |
| `AT+UNLOCKSPR=OLD` | فك قفل SPR | 2 | Q-Unlocker `0x01BB02CA` |
| `AT+SIMLOCKU=0,0,0` | بيانات قفل SIM | 2 | Q-Unlocker `0x01BAD6D7` |
| `AT+SIMLGDATA=<...>` | بيانات قفل LG SIM | 8 | Q-Unlocker |
| `AT+CLCK="PN",2` | Network Lock | — | at-bridge |

### 6.4 `SRELOCK` و`MCCREQ`

**`SRELOCK` داخل AT:**

- النص عند Ghidra `0x2aa350` (raw `0x1aa350`)، والنسخة الصغيرة `srelock` عند `0x2aa358`.
- المقارنة داخل `FUN_00161334` (raw `0x61334`، الحجم 40984 بايت):

```asm
0x065a30  ldr   x0, [x19, #0x320]
0x065a38  mov   w2, #7              ; طول البادئة
0x065a3c  add   x1, x1, #0x350      ; "SRELOCK"
0x065a40  bl    #0x1695c0           ; مقارنة أول 7 أحرف
0x065a48  cset  w11, eq
0x065a5c  strb  w11, [x19, #0x32f]  ; علم الحالة
```

**النتيجة:** أي أمر يبدأ بـ`SRELOCK` يُقبل — لذلك `SRELOCK1` يعمل.

**`MCCREQ` المرصود (Q-Unlocker → `apirelock/`):**

```json
"MCCREQ": "421F0100421F0200421F0300421F0400421F1000421F1100",
"OprationType": "Carrier Relock",
"Opration_Function": "Carrier Relock M1",
"model": "SM-S928U1",
"CSC": "XAU"
```

| # | Hex | MCC | MNC | الشبكة (Octoplus) |
|---|---|---|---|---|
| 1 | `421F0100` | 421 | 01 | Sabaphone |
| 2 | `421F0200` | 421 | 02 | MTN/Spacetel |
| 3 | `421F0300` | 421 | 03 | Yemen Mob. CDMA |
| 4 | `421F0400` | 421 | 04 | HITS/Y Unitel |
| 5 | `421F1000` | 421 | 10 | غير موجودة في القاعدة |
| 6 | `421F1100` | 421 | 11 | غير موجودة في القاعدة |

**بنية السجل:** 4 بايت = `MCC BCD (2 بايت)` + `MNC BCD (بايت)` + `00`.
**مقابل ترميز 3GPP 24.008:** 3 بايت/PLMN — لا تخلط بينهما.
**MNC ثلاثي الخانات:** غير مرصود؛ الترميز غير مؤكد.
**احتمال بديل:** لو كان الحقلان الأخيران 16-bit little-endian، فإن `421F1000` = 16 و`421F1100` = 17 (وليس 10/11). لا يوجد مثال حاسم؛ تفسير 10/11 يعتمد على اعتبار `00` حشوًا.

### 6.5 خصائص/أوامر ADB

| الأمر/الخاصية | الناتج | العدد | المصدر |
|---|---|---:|---|
| `getprop gsm.sim.operator.numeric` | MCC+MNC للشريحة | 14 | CDMA-YTP.exe |
| `getprop gsm.operator.alpha` | اسم الشبكة | 13 | CDMA-YTP.exe |
| `getprop gsm.operator.iso-country` | ISO الدولة | 11 | CDMA-YTP.exe |
| `getprop gsm.sim.state` | حالة الشريحة | 11 | CDMA-YTP.exe |
| `getprop gsm.network.type` | نوع الشبكة | 16 | CDMA-YTP.exe |
| `getprop ro.boot.carrierid` | Carrier ID | 25 | CDMA-YTP.exe |
| `getprop persist.sys.prev_salescode` | CSC السابق | 12 | CDMA-YTP.exe |
| `getprop gsm.operator.numeric` | MCC+MNC للشبكة | 0 | at-bridge:148 — غير مرصود |
| `getprop ro.csc.sales_code` | CSC الحالي | 1 | تشخيصي |

**خصائص at-bridge:** `gsm.sim.operator.numeric`، `gsm.operator.numeric` (سطر 147–148)، و`ro.csc.sales_code` / `ro.csc.countryiso_code` (سطر 142–143).

**أوامر مساعدة (غير مرصودة):**

```bash
adb shell getprop | grep -E "gsm\.(sim\.)?operator\.(numeric|alpha|iso-country)"
adb shell dumpsys telephony.registry | grep -Ei "mcc|mnc|operatorNumeric|countryIso"
adb shell content query --uri content://telephony/carriers --projection numeric,name
```

> `service call iphonesubinfo 1/2/15` مرصودة، لكنها لمعرّفات الجهاز/الشريحة وليست MCC/MNC؛ معنى المعاملة يختلف بين إصدارات أندرويد. استخدم `AT+CIMI` لـIMSI.

### 6.6 قاعدة `MCC_MNC_LIST`

- **المسار:** `C:\Program Files (x86)\Octoplus\Octoplus_Samsung\MCC_MNC_LIST` — SQLite، جدول `mytable`.
- **الأعمدة:** `MCC, MCC_int, MNC, MNC_int, ISO, Country, Country_Code, Network, cheched`.
- **الإحصاءات:** 1688 صفاً — 225 MCC — 224 دولة — 1023 شبكة.
- **الملف المصدَّر:** `MCC_MNC_LIST.csv` (أعمدة: `MCC,MNC,ISO,Country,Network`).

```sql
SELECT MCC, MNC, ISO, Country, Network
FROM mytable WHERE MCC = 421 ORDER BY MNC;
```

```text
421 | 01 | ye | Yemen | Sabaphone
421 | 02 | ye | Yemen | MTN/Spacetel
421 | 03 | ye | Yemen | Yemen Mob. CDMA
421 | 04 | ye | Yemen | HITS/Y Unitel
```

### 6.7 أدوات KEY-Tool / Q-Unlocker

| النص | الدليل |
|---|---|
| `Invalid MCC-MNC` | Q-Unlocker — `relock_extracted_results.txt:20/76/80` |
| `Length of MCC-MNC bytes:` | Q-Unlocker — `region_results.txt:11/35` |
| `SRELOCK1` | Q-Unlocker — `relock_extracted_results.txt:20/76/80` |
| `Select MCC/MNC List` | Q-Unlocker — سطر 96/113 |
| `Search Networks (Country, Operator, MCC, MNC)` | Q-Unlocker — نفس السطر |
| `Reading All MCC+MNC Available :` | KEY-Tool.DMP @`0x16a515e` / `0x16ba48e` |
| `MCCMNC.txt` | KEY-Tool.DMP @`0x16c62c5` (ASCII) / `0x6c0f975` (UTF-16LE) |
| `AT_SamMCCMNCTool_V1_arm64_KT` | KEY-Tool.DMP @`0x6c0f9cf` (UTF-16LE) |
| `AT_SamMCCMNCTool_V1_KT` | KEY-Tool.DMP @`0x6c0fb87` (UTF-16LE) |
| `MNCReLockSlot(int)` / `MNCReLockSignal(int)` | `keytool_dmp_analysis.txt:42` |
| `listmcc` / `selectedmcc` / `lineMCC` / `lineMNC` | `keytool_dmp_analysis.txt:43` |

> `AT_SamMCCMNCTool_V1_arm64_KT` و`MCCMNC.txt` **غير موجودين محلياً**؛ الاسم فقط داخل DMP.

### 6.8 مصفوفة المصدر (مختصرة)

| الأمر/الخاصية | AT | at-bridge | أدوات PC | معياري/أندرويد |
|---|---|---|---|---|
| `AT+DEVCONINFO` | ✗ | ✅:83 | ✅ 35 | ✅ Samsung |
| `AT+SVCIFPGM=1,1` | ✗ | ✗ | ✅ 16 | ✅ Samsung |
| `AT+CIMI` | ✗ | ✗ | ✅ 34 | ⭐ 3GPP |
| `AT+COPS` / `AT+CREG` | ✗ | ✗ | ◐ مصنّف فقط | ⭐ 3GPP |
| `AT+PRECONFG=2` | ✗ | ✅:80 | ✅ 20 | ✅ Samsung |
| `AT+RFBYCODE` | ✗ | ✗ | ✅ 16 | ✅ Samsung |
| `AT+UNLOCKSPR` / `SIMLOCKU` / `SIMLGDATA` | ✗ | ✗ | ✅ | ✅ Samsung/LG |
| `SRELOCK` / `SRELOCK1` | ✅ `0x2aa350` | ✗ | ✅ Q-Unlocker | ✅ Samsung RIL |
| `gsm.sim.operator.numeric` | ✗ | ✅:147 | ✅ 14 | ✅ Android |
| `gsm.operator.numeric` | ✗ | ✅:148 | ✗ | ✅ Android |
| `ro.csc.sales_code` | ✗ | ✅:142 | ✅ 1 | ✅ Samsung |
| `MCCREQ` | ✗ | ✗ | ✅ Q-Unlocker | أداة خاصة |

### 6.9 حدود الاستخراج

1. لا نصوص MCC/MNC داخل `AT` (0/141).
2. لا أمر كتابة MCC/MNC مباشر داخل `AT` سوى مسار `SRELOCK` غير المفكوك بالكامل.
3. لا مثال MNC ثلاثي الخانات في `MCCREQ`.
4. لا رد كامل لـ`AT+SVCIFPGM=1,1`.
5. `AT+COPS` / `AT+CREG` لم يُرصدا في جلسات L24.
6. `gsm.operator.numeric` مذكور في at-bridge فقط.
7. `AT_SamMCCMNCTool` و`MCCMNC.txt` غير موجودين محلياً.
8. `MCCREQ` قائمة اختيار وليست قراءة جهاز.
9. `SRELOCK1` مؤكد كنص، لكن صيغة الحزمة بعده غير مؤكدة.

### 6.10 أوامر التغيير/الإضافة (Write / Add)

> **قاعدة:** لا يوجد أمر AT معياري يغيّر MCC/MNC المطبوع في الشريحة. التفصيل الكامل في `MCC_MNC_COMMANDS.md` — القسم 12.

#### 6.10.1 أوامر الكتابة داخل `AT` (`FUN_00161334`)

| الأمر | الوظيفة | الدليل (raw) |
|---|---|---|
| `SETSPR` | **كتابة SPR** (قائمة قفل المشغل التي تحوي MCC/MNC) | `0x68828` — `mov w2,#6` + `"SETSPR"` @ Ghidra `0x2aa2d4` — **بادئة 6 أحرف** |
| `setspr` | نسخة صغيرة | `0x6b26c` — `mov w2,#6` + `0x2aa2dc`؛ النتيجة في `[x19+0x2e0]` |
| `SRELOCK` → `SRELOCK1` | إعادة قفل المشغل بقائمة MCC/MNC | `0x65a30` — `mov w2,#7` + `0x2aa350` |
| `GETSPR` | قراءة SPR (نسخة احتياطية/تحقق) | `0x613cc` — `"GETSPR"` @ `0x2aa428` |
| `EXIT` / `exit` | إنهاء الجلسة | `0x69af4` / `0x69bf8` |
| `AT+` | تمرير أوامر المودم | `0x63130` — `0x2aa424` |

**ثوابت كتابة مرافقة:** `51180004` (`0x2aa430`)، `5119010532` (`0x2aa2e8`)، `01426X1` (`0x2aa2f8`)، `000000000000` (`0x2aa448` — تُقارن مع حقل عند `0x64784` كفحص «سجل أصفار»)، `52 i32 0` + `128/130/131/132/135/136` (`0x2aa238–0x2aa26c`).

#### 6.10.2 التسلسل المرصود للكتابة

```text
adb push <file> /data/local/tmp/athun        (x2)
adb shell chmod 0777 /data/local/tmp/athun  (x2)
AT+AKSEEDNO=0 / AT+AKSEEDNO=1,0             (x2 / x4)  ← مصادقة
GETSPR → SETSPR → Result : OK → exit/EXIT              ← قراءة ثم كتابة SPR
SRELOCK1 (مع MCCREQ من apirelock/)                     ← إعادة قفل بقائمة MCC/MNC
adb shell rm /data/local/tmp/athun          (x2)
adb shell logcat -b radio -c                (x4)
```

- نص Q-Unlocker: `GETSPR. . .1.sam.SETSPR.exit.EXIT` و`SETSPR.Result : OK.exit.EXIT.Unlocking Done`.
- KEY-Tool: `Reading All MCC+MNC Available :` → `Writing SIMLock Blob :` → `Writing Shared2 Blob :` → `- Writing SIMLock :` → إعادة تشغيل.
- محلّل رد AKSEEDNO المرصود: `+AKSEEDNO:\d+,(\d+)-(\d+)-(\d+)` (General Unlocker Pro، RAM `0x01F2EE00`).

#### 6.10.3 أوامر الكتابة المعيارية والإضافية

| الأمر | النوع | الحالة |
|---|---|---|
| `AT+CPOL=<index>,2,"42101"` | 3GPP — إضافة MCC/MNC لقائمة PLMN | معياري — غير مرصود |
| `AT+CPOL=<index>` | 3GPP — حذف مدخل | معياري — غير مرصود |
| `AT+CPLS=0/1/2` | 3GPP — اختيار القائمة | معياري — غير مرصود |
| `AT+COPS=1,2,"42101"` | 3GPP — اختيار يدوي | معياري — غير مرصود |
| `AT+CLCK="PN",1,"<pwd>"` | 3GPP — تفعيل Network Lock | معياري — at-bridge يستخدم 2 فقط |
| `content insert --uri content://telephony/carriers --bind numeric:s:"42101" ...` | Android — APN بـMCC/MNC | معياري — غير مرصود |
| `content insert --uri content://com.lge.node/omadm_node --bind key:s:OMADM_SIMLOCK --bind value:i:2` | LG — علم قفل SIM | **مرصود x2** |
| `pm uninstall ... AutoPreconfig / CarrierCodeChanger` | Samsung — إزالة preconfig | **مرصود x2 لكل أمر** |
| `AT+PRECONFG=2,...` / `AT+RFBYCODE=...` | Samsung — كتابة preconfig/CSC | **مرصود 20 / 16** |

**تحذير:** `SETSPR`/`SRELOCK1` يكتبان بيانات قفل المشغل — الخطأ قد يقفل الجهاز على شبكة لا تستخدمها. خُذ نسخة (`GETSPR`) أولاً، واختبر على جهاز اختبار.

**انظر `MCC_MNC_COMMANDS.md` للتفاصيل الكاملة، و`MCC_MNC_COMMANDS.txt` للقائمة المسطحة.**
