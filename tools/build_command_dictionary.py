# -*- coding: utf-8 -*-
"""
build_command_dictionary.py  —  بناء القاموس النهائي الكامل للأوامر
====================================================================
المصادر:
  1) AT_ALL_STRINGS.json        : كل نصوص AT المفكوكة من رام الهاتف (141 نصاً) + المفاتيح + الدوال
  2) AT_BRIDGE_EXTRACT.json     : أوامر جسر at-bridge المستخرجة من كود المصدر C
  3) AT_unpacked.elf            : نصوص أداة AT_unpacked (نسخة أخرى) — تُستخرج هنا مباشرة
المخرجات:
  ALL_COMMANDS_COMPLETE.md      : القاموس الشامل المنظم
  ALL_COMMANDS_MASTER.txt       : قائمة مسطحة لكل الأوامر بوسم المصدر (قابلة للـgrep)
  AT_UNPACKED_COMMANDS.txt      : أوامر/نصوص AT_unpacked
لا يحذف أي سكربت سابق.
"""
import re, json, os, io, sys
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")

ROOT   = r"C:\Users\pc5\Documents\AT"
OUT    = ROOT + r"\AT_Source_Extract"
UNP    = r"C:\Users\pc5\Documents\GitHub\PhoenixGSM\AT\AT_Extracted_Output\AT_unpacked.elf"

recs = json.load(open(OUT + r"\AT_ALL_STRINGS.json", encoding="utf-8"))
bridge = json.load(open(OUT + r"\AT_BRIDGE_EXTRACT.json", encoding="utf-8"))

# ---------------------------------------------------------------- AT_unpacked
unp = open(UNP, "rb").read()
unp_strings = []
for m in re.finditer(rb"[\x20-\x7e]{4,}", unp):
    s = m.group().decode("ascii")
    if s.startswith(("_Z", "NSt", "N12_GLOBAL", "GLIBC", "gnu_", "__")):
        continue
    if any(k in s for k in ("AT+", "AT:", "CSC", "HEX:", "INFO|", "Auth", "Token", "auth",
                            "Ril", "RIL", "ril", "modem", "Modem", "/dev/", "send_recv",
                            "Challenge", "CALC", "SUCCESS", "Wrong", "Worng", "Error",
                            "ERROR", "pm ", "am ", "settings", "input ", "logcat",
                            "echo ", "toybox", "nc ", "uiautomator", "cat ", "rm -",
                            "> /dev/null", "Usage", " -c", " -a", " -x", " -s", " -l",
                            " -d", " -p", " -v", " -h", "port", "socket", "serial",
                            "SECURE", "EXIT", "QUIT", "HELP", "SCAN", "STATUS", "LOCKS")):
        unp_strings.append((m.start(), s))
seen = set(); unp_u = []
for o, s in unp_strings:
    if s in seen: continue
    seen.add(s); unp_u.append((o, s))
with open(ROOT + r"\AT_UNPACKED_COMMANDS.txt", "w", encoding="utf-8", errors="replace") as f:
    f.write("AT_unpacked.elf — نصوص/أوامر مستخرجة من الملف (نسخة أخرى من الأداة)\n")
    f.write(f"الحجم: {len(unp)} بايت | عدد النصوص ذات الصلة: {len(unp_u)}\n")
    f.write("ملاحظة: هذه نسخة مختلفة عن AT الحالي؛ بعض نصوصها مشفرة/مبعثرة وتحتاج تفريغ رام لتأكيدها.\n")
    f.write("=" * 100 + "\n\n")
    for o, s in unp_u:
        if len(s) > 1000:
            s = s[:120] + f"... <Base64/blob — {len(s)} chars; extracted to EMBEDDED_APK>"
        f.write(f"0x{o:06x}  {s}\n")

# ---------------------------------------------------------------- تصنيف نصوص AT
def ignore(s):
    return (s.startswith(("r+e", "w+e", "a+e", "wbe", "abe", "rbe", "r+be", "w+be", "a+be"))
            or s.strip() in ("i32", "3w", ";w", "p.w", "x w", "`,w", "p,w", "null", "00000000"))

CATS = [
    ("البانر وهوية الأداة", lambda s: any(k in s for k in ("HUNTER", "SAM RIL", "******", "Product", "Model :", "CPU :", "Serial", "Security patch", "Build", "SDK Version"))),
    ("خصائص النظام المقروءة", lambda s: s.startswith("ro.")),
    ("حزم وأنشطة سامسونج", lambda s: s.startswith("com.") and " " not in s),
    ("أوامر الشل والأتمتة", lambda s: s.startswith(("input ", "settings ", "pm ", "am ", "cmd ", "logcat", "ps ", "kill ", "echo ", "cat ", "rm ", "uiautomator", "pclose")) or "toybox" in s or "logcat" in s),
    ("مسارات وقنوات", lambda s: s.startswith(("/proc/", "/dev/", "/sdcard/", "/system/")) or s in ("package:", "shell ", "%s%s")),
    ("واجهة RIL والمكتبة", lambda s: s == "AT+" or s in ("EXIT", "RIL Exit") or any(k in s for k in ("send_recv_cmd", "Failed to load library", "Failed to find symbol", "Found 'send_recv_cmd'", "did not return a valid response", "RIL Exit", "Ril Error", "RIL Error", "Worng Ril", "Ril Token", "ERORR: Connect"))),
    ("المصادقة والتوكن والتشفير", lambda s: any(k in s for k in ("Challenge key", "Encrypted CALC", "SETSPR", "GETSPR", "SRELOCK", "Remove KG", "Open Done", "B1EEBF", "Xgvkw", "01426", "5119", "5118", "OK:%s", "01000101", "000000000000", "120d"))),
    ("ترميز Base64/Hex", lambda s: ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" in s) or s in ("%02X", "%02x") or "base64" in s),
    ("APK المدمج (Base64)", lambda s: s.startswith("UEsDBA") or s == "com.sdet.fotaagent"),
    ("رسائل وأخطاء", lambda s: any(k in s for k in ("Error", "ERROR", "EXECUTE", "aerr_wait", "thread constructor", "allocator<", "Open Done", "Remove KG"))),
]
def cat_of(s):
    for name, fn in CATS:
        if fn(s): return name
    return "أخرى / غير مصنفة"

use = [r for r in recs if not ignore(r["text"])]
bycat = {}
for r in use:
    bycat.setdefault(cat_of(r["text"]), []).append(r)

# ---------------------------------------------------------------- MASTER TXT
with open(ROOT + r"\ALL_COMMANDS_MASTER.txt", "w", encoding="utf-8", errors="replace") as f:
    f.write("ALL_COMMANDS_MASTER — كل الأوامر/النصوص المستخرجة (AT + at-bridge + AT_unpacked)\n")
    f.write("=" * 100 + "\n\n")
    f.write(f"### AT (HUNTER-UNLOCKER) — {len(use)} نصاً (من 141 خام، بعد استبعاد الضجيج)\n")
    for r in use:
        fns = ",".join(r["functions"][:5]) or "-"
        t = r["text"]
        if len(t) > 1000:
            t = t[:120] + f"... <Base64/blob — {len(r['text'])} chars; extracted to EMBEDDED_APK>"
        f.write(f"[AT] GH:{r['ghidra']} key={','.join(r['keys'])} fn={fns}\t{t}\n")
    f.write("\n### at-bridge (كود المصدر C)\n")
    for n, v in bridge["defines"]:
        f.write(f"[AT-BRIDGE define {n}]\t{v}\n")
    for s in bridge["literals"]:
        f.write(f"[AT-BRIDGE src]\t{s}\n")
    f.write("\n### AT_unpacked.elf\n")
    for o, s in unp_u:
        if len(s) > 1000:
            s = s[:120] + f"... <Base64/blob — {len(s)} chars>"
        f.write(f"[AT-UNPACKED 0x{o:06x}]\t{s}\n")

print("master lines done; AT used:", len(use), "| bridge defines:", len(bridge["defines"]),
      "| bridge lits:", len(bridge["literals"]), "| unpacked:", len(unp_u))

# ---------------------------------------------------------------- COMPLETE MD
W = 0
def md(s=""):
    global W
    f.write(s + "\n")

with open(ROOT + r"\ALL_COMMANDS_COMPLETE.md", "w", encoding="utf-8", errors="replace") as f:
    md("# القاموس الشامل الكامل للأوامر — نسخة موثّقة نهائية")
    md()
    md("**التاريخ:** 23/10/2026 — **الهدف:** استخراج *كل* الأوامر بالكامل، بلا استثناء، مع إثبات المصدر.")
    md()
    md("## منهجية الإثبات")
    md("1. **AT (الملف الأساسي):** شُغّل على الهاتف SM-G715U1، جُمّد، وفُرّغت ذاكرته. النصوص تُفكّ في مكانها وقت التشغيل.")
    md("2. كل نص في الرام قُوبل مع القرص: `key = disk_byte XOR ram_byte` — ومنه استُخرج مفتاح كل نص.")
    md("3. المراجع من الكود حُسبت من بايتات التعليمات نفسها (ADRP+ADD) — لا تخمين.")
    md("4. **إثبات الاكتمال:** لا يوجد أي مقطع نصي (≥8 أحرف) بقي مشفراً في `.data` — كل المقاطع القابلة للطباعة في الرام كانت مفكوكة.")
    md("5. أوامر المودم `AT+...` **ليست في ملف AT إطلاقاً** (لا في القرص ولا في الرام) — مصدرها الحقيقي كود `at-bridge` (C) وهو مُستخرج كاملاً أدناه.")
    md()
    md("---")
    md()
    md("## 1) AT — أوامر ونصوص الأداة (كاملة ومفكوكة من الرام)")
    md()
    md(f"إجمالي النصوص الفريدة في الرام: **141** — بعد استبعاد الضجيج ومقاطع C++: **{len(use)}** نصاً/أمراً.")
    md()
    md("> **ملاحظة معمارية مهمة:** الأداة تحتوي البادئة `AT+` (العنوان `0x2aa424`) وتحمّل مكتبة RIL وتستدعي `send_recv_cmd`،")
    md("> لكن **أسماء أوامر المودم نفسها غير مخزّنة داخلها** — تُبنى ديناميكياً أو تأتي من جسر at-bridge على الكمبيوتر عبر منفذ 2600.")
    md("> لهذا فإن أوامر `AT+PRECONFG` / `AT+CLCK` / ... موجودة في كود at-bridge (القسم 2) وليست في ملف AT.")
    md()
    for name, _ in CATS + [("أخرى / غير مصنفة", None)]:
        items = bycat.get(name, [])
        if not items: continue
        md(f"### {name} ({len(items)})")
        md()
        md("| # | النص/الأمر | عنوان Ghidra | مفتاح XOR | الدوال |")
        md("|---:|---|---|---|---|")
        for i, r in enumerate(items, 1):
            t = r["text"]
            if len(t) > 300:
                t = t[:120] + f"... [Base64 APK — الطول {len(r['text'])} حرفاً]"
            t = t.replace("|", "\\|")
            fns = ", ".join(r["functions"][:3]) or "—"
            md(f"| {i} | `{t}` | `{r['ghidra']}` | `{','.join(r['keys'])}` | {fns} |")
        md()

    md("### الأوامر المركّبة ديناميكياً (قوالب تُبنى من المقاطع أعلاه)")
    md()
    md("| القالب | المقاطع المستخدمة | الدالة |")
    md("|---|---|---|")
    md("| `pm path <PACKAGE>` | `pm path ` (0x2aa4b8) + `package:` (0x2aa4c8) | FUN_0016d718 |")
    md("| `am force-stop <PACKAGE>` | `am force-stop ` (0x2aa578) + `<package>` | FUN_0016d718 |")
    md("| `kill pid <PID>` | `kill pid ` (0x273360) + PID | FUN_0015890c |")
    md("| `echo <AT...> \\| /system/bin/toybox nc 127.0.0.1 2600` | `echo ` (0x2aa308) + `AT+` (0x2aa424) + ` \\| /system/bin/toybox nc ...` (0x2aa310) | FUN_00161334 |")
    md("| `input tap <X> <Y>` | `input tap ` (0x2aa5e8) + إحداثيات من regex الـ XML | FUN_001790d8 |")
    md("| `uiautomator dump` → `cat` → `input tap` → `rm -rf` | سلسلة أتمتة الواجهة كاملة | FUN_001790d8 / FUN_00182684 |")
    md()
    md("### إثبات الاكتمال (أرقام)")
    md()
    md("- 141 نصاً فريداً في الرام (كل ما فُكّ عند الإقلاع).")
    md("- 518 مرجعاً مباشراً من التعليمات (ADRP+ADD) إلى 115 نصاً.")
    md("- **0** مقاطع قابلة للطباعة (≥8 أحرف) بقيت مشفّرة في `.data` — أي لا يوجد نص مخفي لم يُفك.")
    md("- **0** أوامر في `.rodata` — كل الأوامر في `.data` وقد فُكّت.")
    md("- 230,263 بايت تغيّرت في `.data` بين القرص والرام = مناطق فك التشفير (منها كتلة APK بمفتاح 0x47).")
    md()

    md("---")
    md()
    md("## 2) at-bridge — أوامر المودم والبروتوكول (من كود المصدر C)")
    md()
    md("المصدر: `gsmtools/at-bridge/jni/all_gsm_commands.h` + `at_bridge.c` — هذه هي أوامر `AT+` التي وردت في القاموس القديم، ومكانها الحقيقي هنا.")
    md()
    md("### 2.1 أوامر AT للمودم")
    md("| الثابت | الأمر |")
    md("|---|---|")
    for n, v in bridge["defines"]:
        if n.startswith("AT_"):
            md(f"| `{n}` | `{v.strip()}` |")
    md()
    md("### 2.2 أوامر الشل والتصعيد (نفس أوامر AT لكن موثقة كمصدر)")
    md("| الثابت | الأمر |")
    md("|---|---|")
    for n, v in bridge["defines"]:
        if n.startswith("CMD_"):
            md(f"| `{n}` | `{v}` |")
    md()
    md("### 2.3 الحزم والأنشطة")
    md("| الثابت | القيمة |")
    md("|---|---|")
    for n, v in bridge["defines"]:
        if n.startswith(("PKG_", "ACT_")):
            md(f"| `{n}` | `{v}` |")
    md()
    md("### 2.4 بروتوكول الجلسة التفاعلية والقيم المنطقية")
    md("| الثابت | القيمة |")
    md("|---|---|")
    for n, v in bridge["defines"]:
        if n.startswith(("PROTO_", "TELEMETRY_")):
            md(f"| `{n}` | `{v}` |")
    md()
    md("### 2.5 نصوص التشغيل الفعلية في at_bridge.c (مسارات، أوامر، ردود، مساعدة)")
    md("```text")
    for s in bridge["literals"]:
        md(s.replace("\\n", "\\n"))
    md("```")
    md()
    md("---")
    md()
    md("## 3) AT_unpacked.elf — النسخة الأخرى")
    md()
    md(f"عدد النصوص ذات الصلة: **{len(unp_u)}** — التفاصيل الكاملة في `AT_UNPACKED_COMMANDS.txt`.")
    md()
    md("أبرز ما تحتويه (رسائل المصادقة القديمة):")
    md("```text")
    for o, s in unp_u:
        if any(k in s for k in ("Auth", "Ril", "RIL", "Token", "Worng", "Wrong", "AT+", "> /dev/null", "hiddenmenu", "logcat", "ps -a", "ro.product", "EXIT", "Error", "ERROR", "Challenge", "CALC", "SUCCESS")):
            md(s)
    md("```")
    md()
    md("---")
    md()
    md("## 4) التحقق من القاموس القديم — أين كل أمر فعلاً؟")
    md()
    md("| ما ورد في `ALL_COMMANDS_DICTIONARY.md` | مكانه الحقيقي | الحالة |")
    md("|---|---|---|")
    rows = [
        ("أوامر الشل (settings / am / input / uiautomator / pm clear / nc)", "AT + at-bridge", "موجودة ومفكوكة (القسم 1)"),
        ("`pm list packages | grep com.skatd.unlock`", "AT (Ghidra 0x2aa530)", "موجودة"),
        ("`am force-stop`", "AT (Ghidra 0x2aa578)", "موجودة"),
        ("`com.samsung.rms.retailagent.global`", "AT (Ghidra 0x2aa4e0)", "موجودة"),
        ("`AT+PRECONFG` / `AT+CFUN` / `AT+FACTORST`", "at-bridge (all_gsm_commands.h)", "غير موجودة في AT — أُضيفت من المصدر"),
        ("`AT+CPIN?` / `AT+CLCK=...` / `AT+DEVCONINFO` / `AT+KSTRINGB`", "at-bridge", "غير موجودة في AT — أُضيفت من المصدر"),
        ("مسارات `/dev/umts_*` و `/dev/smd*` و `/dev/socket/rild*`", "at-bridge (at_bridge.c)", "غير موجودة في AT — أُضيفت من المصدر"),
        ("خصائص `ro.csc.*` / `gsm.*` / `ril.*` / `persist.sys.usb.*`", "at-bridge + خصائص أندرويد عامة", "غالبها ليس في AT"),
        ("واجهة `CSC:` / `AT:` / `HEX:` / `SCAN` / `STATUS` / `LOCKS` / `RECONNECT`", "at-bridge", "أوامر جسر وليست داخل AT"),
        ("سطر `INFO|MODEL=...`", "at-bridge", "مستخرج من المصدر"),
        ("رسائل `[+] Authantication Token` / `Wrong AuthKey Error` / `Ril Token Error`", "AT_unpacked.elf", "نسخة أخرى من الأداة"),
        ("`Worng Ril Command` / `RIL Exit` / `Ril Error 1`", "AT (Ghidra 0x2ac030/0x2aa010/0x2aa4a2)", "موجودة"),
        ("`send_recv_cmd` ورسائل تحميل المكتبة", "AT (Ghidra 0x2aa058+)", "موجودة"),
        ("`01000101`", "غير موجود حرفياً في AT", "ثابت بروتوكول عام — لم يُعثر عليه"),
    ]
    for a, b, c in rows:
        md(f"| {a} | {b} | {c} |")
    md()
    md("---")
    md()
    md("## 5) إحصاءات نهائية")
    md()
    md(f"- نصوص AT المفكوكة: **{len(recs)}** (منها **{len(use)}** أمر/نص ذو معنى).")
    md(f"- أوامر at-bridge من المصدر: **{len(bridge['defines'])}** ثابتاً + **{len(bridge['literals'])}** نصاً.")
    md(f"- نصوص AT_unpacked ذات الصلة: **{len(unp_u)}**.")
    md("- مفاتيح XOR لكل نص في AT موثقة في الجدول (القسم 1).")
    md("- ملفات الآلة: `AT_ALL_STRINGS.json` + `AT_XREF_HITS.json` + `AT_BRIDGE_EXTRACT.json`.")
    md()
    md("> كل سكربتات بايثون محفوظة: `extract_all_commands.py`, `decode_precise.py`, `final_strings.py`, `extract_apk.py`, `build_command_dictionary.py`.")

print("ALL_COMMANDS_COMPLETE.md written")
