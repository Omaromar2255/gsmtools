# -*- coding: utf-8 -*-
"""
extract_all_commands.py — استخراج شامل لكل الأوامر/النصوص من ملف AT ومن رام الهاتف
المنهجية:
  1) على القرص: لكل مفتاح XOR من 1..255 نفك المنطقة المشفرة ونقسم على واصف المفتاح
     (البايت == المفتاح لأن 0x00 ^ key = key) — بلا أي فلترة كلمات مفتاحية.
  2) من الرام (dump3): كل السلاسل القابلة للطباعة (النصوص مفكوكة في مكانها وقت التشغيل).
  3) من rodata: السلاسل الصريحة الأصلية.
  4) دمج + توثيق المصادر + ربط الدوال من func_strings.json (xref من ADRP+ADD).
"""
import re, struct, json, bisect, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")

AT_PATH   = r"C:\Users\pc5\Documents\AT\AT"
DUMPS     = r"C:\Users\pc5\Documents\AT\AT_Source_Extract\DUMPS"
SRC_DIR   = r"C:\Users\pc5\Documents\AT\AT_Source_Extract"
# ملاحظة: هذا السكربت هو النسخة الخام الأولى (تنتج نتائج كثيرة زائفة لأنها تفك كل
# المفاتيح على كل البايتات). المخرجات النهائية الموثّقة ينتجها build_command_dictionary.py.
# نكتب هنا إلى RAW_* كي لا نطمس الملفات النهائية.
OUT_MD    = r"C:\Users\pc5\Documents\AT\RAW_XOR_SCAN.md"
OUT_TXT   = r"C:\Users\pc5\Documents\AT\RAW_XOR_SCAN.txt"

data = open(AT_PATH, "rb").read()
d3   = open(DUMPS + r"\dump3_data_rw-p.bin", "rb").read()
d2   = open(DUMPS + r"\dump2_relro_r--p.bin", "rb").read()
d4   = open(DUMPS + r"\dump4_bss_anon.bin", "rb").read()

# ---------------------------------------------------------------- section table
shoff  = struct.unpack_from("<Q", data, 40)[0]
shent  = struct.unpack_from("<H", data, 58)[0]
shnum  = struct.unpack_from("<H", data, 60)[0]
shstrx = struct.unpack_from("<H", data, 62)[0]
o  = shoff + shstrx * shent
stro = struct.unpack_from("<Q", data, o + 24)[0]
sts  = struct.unpack_from("<Q", data, o + 32)[0]
st   = data[stro:stro + sts]
SECS = []
for i in range(shnum):
    s = shoff + i * shent
    nm   = st[struct.unpack_from("<I", data, s)[0]:].split(b"\0")[0].decode()
    addr = struct.unpack_from("<Q", data, s + 16)[0]
    off  = struct.unpack_from("<Q", data, s + 24)[0]
    sz   = struct.unpack_from("<Q", data, s + 32)[0]
    if sz: SECS.append((nm, addr, off, sz))
def sec_of(off):
    for nm, addr, o_, sz in SECS:
        if o_ <= off < o_ + sz: return nm
    return "?"

# file offset -> virtual address (for xref matching with code)
def file_to_va(foff):
    # LOAD2: file 0x00000-0x169930 -> va same ; LOAD3: +0x1000 ; LOAD4: +0x2000
    if foff < 0x169af0: return foff
    if foff < 0x170a70: return foff + 0x1000
    return foff + 0x2000

# ---------------------------------------------------------------- 1) disk XOR decode
# encoded candidates live anywhere in .data / .data.rel.ro / early .data / rodata tail
REGIONS = [("rodata", 0x13250, 0x1c3a8), ("data", 0x170a70, 0x1aa930)]
disk = {}   # plaintext -> dict(sources=[(fileoff,key)], ...)
for rname, s0, s1 in REGIONS:
    blob = data[s0:s1]
    for key in range(1, 256):
        dec = bytes(b ^ key for b in blob)
        parts = dec.split(bytes([key]))          # terminator = key byte
        pos = 0
        for part in parts:
            if 3 <= len(part) <= 400 and all(0x20 <= c <= 0x7e for c in part):
                try: s = part.decode("ascii")
                except UnicodeDecodeError: s = None
                if s:
                    foff = s0 + pos
                    # التحقق: النص الحقيقي يبدأ بعد أصفار الحشو (البيانات الفعلية)
                    rec = disk.setdefault(s, {"foffs": [], "keys": set(), "regions": set()})
                    rec["foffs"].append(foff); rec["keys"].add(key); rec["regions"].add(rname)
            pos += len(part) + 1

# ---------------------------------------------------------------- 2) RAM plaintext
D3_VA = 0x172000
ram = {}    # plaintext -> list of ram offsets
for m in re.finditer(rb"[\x20-\x7e]{3,}", d3):
    s = m.group().decode("ascii")
    ram.setdefault(s, []).append(m.start())
ram2 = {}
for m in re.finditer(rb"[\x20-\x7e]{3,}", d2):
    s = m.group().decode("ascii")
    ram2.setdefault(s, []).append(m.start())

# ---------------------------------------------------------------- 3) rodata plaintext
rod = {}
for m in re.finditer(rb"[\x20-\x7e]{3,}", data[0x13250:0x1c3a8]):
    s = m.group().decode("ascii")
    rod.setdefault(s, []).append(0x13250 + m.start())

# ---------------------------------------------------------------- 4) function xrefs
fs = {}
try:
    js = json.load(open(SRC_DIR + r"\func_strings.json"))
    for k, strs in js.items():
        gh, nm = k.split("|")
        for s in strs: fs.setdefault(s, set()).add(nm)
except FileNotFoundError:
    pass

# ---------------------------------------------------------------- 5) master merge
allstr = set(disk) | set(ram) | set(rod) | set(fs)
rows = []
for s in allstr:
    if len(s) < 3: continue
    src = []
    if s in disk:
        k = sorted(disk[s]["keys"])
        f0 = min(disk[s]["foffs"])
        src.append(f"DISK-XOR key=0x{k[0]:02X} @file+0x{f0:06x} [{sec_of(f0)}]")
    if s in ram:
        r0 = min(ram[s])
        src.append(f"RAM+0x{r0:06x}")
    if s in ram2 and s not in ram:
        src.append(f"RAM-relro+0x{min(ram2[s]):06x}")
    if s in rod:
        src.append(f"RODATA@0x{min(rod[s]):06x}")
    fns = sorted(fs.get(s, []))
    rows.append((s, src, fns))

rows.sort(key=lambda r: (-len(r[1]), -len(r[0])))

with open(OUT_TXT, "w", encoding="utf-8", errors="replace") as f:
    f.write(f"MASTER LIST — كل النصوص/الأوامر المستخرجة من ملف AT ورام الهاتف\n")
    f.write(f"إجمالي السلاسل الفريدة: {len(rows)}\n")
    f.write("=" * 110 + "\n\n")
    for i, (s, src, fns) in enumerate(rows, 1):
        f.write(f"[{i:04d}] {s}\n")
        for x in src: f.write(f"       src: {x}\n")
        if fns: f.write(f"       used by: {', '.join(sorted(fns)[:8])}\n")
        f.write("\n")

# ---------------------------------------------------------------- 6) keyword categories for MD
CATS = [
 ("أوامر Shell التنفيذية", ["am ", "pm ", "settings ", "input ", "cmd ", "echo ", "logcat", "ps ", "cat ", "rm -", "toybox", "nc ", "kill", "chmod", "chown", "mount", "setprop", "getprop", "sleep", "cp ", "mv ", "dd ", "grep"]),
 ("أوامر AT للمودم", ["AT+", "AT&", "+CPIN", "+CLCK", "+PRECONFG", "+CFUN", "IMEI", "KSTRING", "DEVCON", "FACTOR", "OK"]),
 ("مسارات المودم والأجهزة", ["/dev/", "/proc/", "/sys/", "/system/", "/data/", "/sdcard/", "/efs/", "/cache/", "/vendor/"]),
 ("خصائص النظام ro./gsm./ril./persist.", ["ro.", "gsm.", "ril.", "persist.", "sys.usb", "net."]),
 ("حزم وأنشطة سامسونج", ["com.", "android."]),
 ("بروتوكول الجلسة والمصادقة", ["Auth", "auth", "Token", "token", "RESPONSE", "SUCCESS", "Error", "ERROR", "Exit", "EXIT", "HELP", "SCAN", "STATUS", "LOCKS", "RECONNECT", "CSC:", "AT:", "HEX:", "INFO|", "QUIT"]),
 ("ملفات وواجهات XML", ["xml", "XML", "bounds", "resource", "node", "button", "EXECUTE", "aerr"]),
 ("رسائل وأخطاء أخرى", ["Failed", "failed", "Not Found", "Wrong", "Worng", "invalid", "Invalid", "%s", "%d", "conn=", "code:", "len="]),
]
def cat_of(s):
    low = s.lower()
    for name, kws in CATS:
        for kw in kws:
            if kw.lower() in low: return name
    return "أخرى"

with open(OUT_MD, "w", encoding="utf-8", errors="replace") as f:
    f.write("# القائمة الكاملة النهائية لجميع أوامر ونصوص ملف AT\n\n")
    f.write("**المصادر:** فك XOR الشامل على القرص (كل المفاتيح 1..255) + رام الهاتف الحية (نصوص مفكوكة تلقائياً) + rodata الصريحة + xref من كود Ghidra\n\n")
    f.write(f"**إجمالي السلاسل الفريدة المستخرجة: {len(rows)}**\n\n")
    f.write("> ملاحظة: كل سطر يحمل مصادره (مفتاح XOR وإزاحته على القرص / إزاحته في الرام) والدوال التي تستخدمه.\n\n")
    f.write("---\n\n")
    bycat = {}
    for s, src, fns in rows: bycat.setdefault(cat_of(s), []).append((s, src, fns))
    order = [c for c, _ in CATS] + ["أخرى"]
    for name in order:
        items = bycat.get(name, [])
        if not items: continue
        f.write(f"## {name} ({len(items)})\n\n")
        f.write("| # | النص/الأمر | المصدر (مفتاح XOR / الرام) | الدوال المستخدمة |\n|---:|---|---|---|\n")
        for i, (s, src, fns) in enumerate(items, 1):
            esc = s.replace("|", "\\|").replace("\n", " ")
            f.write(f"| {i} | `{esc[:300]}` | {'؛ '.join(src[:2])} | {', '.join(sorted(fns)[:4]) or '—'} |\n")
        f.write("\n")

print(f"unique strings: {len(rows)}")
print(f"disk-XOR decoded: {len(disk)} | RAM .data: {len(ram)} | rodata: {len(rod)}")
