# -*- coding: utf-8 -*-
"""
extract_ril_libraries.py
=========================
يستخرج ويفحص كل ما يتعلق بمكتبات RIL (libril.so / libsecril-client.so / libsec-ril.so)
من KEY-Tool.DMP و SimUnlockerPro.DMP + يوثق offsets الأدلة.

الاستخدام:  python extract_ril_libraries.py
النتائج:
  - AT_Source_Extract/KEYTool_RIL_agent_32bit.elf   (عميل RIL 32-بت المدمج في KEY-Tool)
  - طباعة: NEEDED/الواردات/الصادرات + مداخل ZIP الخاصة بـ lib/arm64-v8a/libril.so

الأدلة الموثقة (Offsets):
  KEY-Tool.DMP:
    0x689f91e  : ELF 32-bit ARM (50,172 بايت) — عميل RIL للأداة
                 NEEDED: libsecril-client.so + libsec-ril.so
                 imports: dlopen, dlsym, execv, setuid, system, popen-frontend
                 exports: main, onclick(), AES, RSA_verify, Encrypt_Massage*sha256...
    0x68a19e2  : dynstr يحوي dlopen/dlsym (نفس الـELF أعلاه)
    0x68a2222  : dynstr "libsecril-client.so"
    0x68a2236  : dynstr "libsec-ril.so"
    0x16ba75e  : "libplayservice.so: Permission denied" (وكيل KEY-Tool)
    0x2f8517d / 0x2fdacff / 0x3c65a2d / 0x3cba02d : "lib/arm64-v8a/libril.so"
    0x2fdacc3  : ZIP CD entry  lib/arm64-v8a/libril.so  csize=343,768  usize=351,901  CRC=0x3b4f66b3
    0x3cb9ff1  : ZIP CD entry  lib/arm64-v8a/libril.so  csize=338,257  usize=346,333  CRC=0x0bf9fa74
  SimUnlockerPro.DMP:
    0x3ab1989 / 0x3b09a9f / 0x52b2ebb : "lib/armeabi-v7a/libril.so" (داخل APK مع META-INF/ANDROID.RSAPK)
    0x2015ecf  : "dlopen failed" + RIL202310 + RIL202409 + audiocoredebug + AT+SIMLGDATA=
    0x17920fa5 : UTF-16 names: ATRIL / ATRIL64 / NoAppRil / Ekbase.apk / DM.apk / sim_android_1.5.apk
"""
import struct, os, sys

KEY_DMP   = r"C:\Users\pc5\Desktop\KEY-Tool.DMP"
SUP_DMP   = r"C:\Users\pc5\Desktop\SimUnlockerPro.DMP"
OUT_DIR   = r"C:\Users\pc5\Documents\AT\AT_Source_Extract"
AGENT_OFF = 0x689f91e
AGENT_WIN = 0xd000  # نافذة قراءة كافية (p_filesz الحقيقي = 0xB28C)


def elf32_dynamic(blob):
    """يحلل ELF 32-bit: NEEDED + imports + exports عبر PH_DIRECT."""
    e_phoff = struct.unpack_from("<I", blob, 0x1c)[0]
    e_phentsize = struct.unpack_from("<H", blob, 0x2a)[0]
    e_phnum = struct.unpack_from("<H", blob, 0x2c)[0]
    loads, dyn_off = [], None
    for i in range(e_phnum):
        o = e_phoff + i * e_phentsize
        p_type, p_offset, p_vaddr, _pa, p_filesz, _msz, _fl, _al = struct.unpack_from("<IIIIIIII", blob, o)
        if p_type == 1:
            loads.append((p_vaddr, p_offset, p_filesz))
        elif p_type == 2:
            dyn_off = p_offset
    def v2o(v):
        for va, o, sz in loads:
            if va <= v < va + sz:
                return o + (v - va)
        return None
    strtab = symtab = None
    needed, soname = [], None
    o = dyn_off
    while True:
        tag, val = struct.unpack_from("<iI", blob, o); o += 8
        if tag == 0:
            break
        if tag == 1:  needed.append(val)
        if tag == 14: soname = val
        if tag == 5:  strtab = val
        if tag == 6:  symtab = val
    stroff = v2o(strtab)
    def s(idx):
        no = stroff + idx
        e = blob.find(b"\0", no)
        return blob[no:e].decode("latin1")
    imports, exports = [], []
    symoff = v2o(symtab)
    i = 0
    while True:
        o2 = symoff + i * 16
        if o2 + 16 > dyn_off:
            break
        nameoff, val, size, info, other, shndx = struct.unpack_from("<IIIBBH", blob, o2)
        nm = s(nameoff) if nameoff else ""
        if nm:
            (imports if shndx == 0 else exports).append(nm)
        i += 1
    return {
        "needed": [s(x) for x in needed],
        "soname": s(soname) if soname else None,
        "imports": imports,
        "exports": exports,
    }


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    data = open(KEY_DMP, "rb").read()

    # 1) استخراج عميل RIL 32-بت المدمج
    agent = data[AGENT_OFF:AGENT_OFF + AGENT_WIN]
    out = os.path.join(OUT_DIR, "KEYTool_RIL_agent_32bit.elf")
    open(out, "wb").write(agent[:0xB28C])  # p_filesz الفعلي من PT_LOAD الأول
    print(f"[+] extracted: {out} (0x{AGENT_OFF:x}, 0xB28C bytes)")

    info = elf32_dynamic(agent)
    print("\n=== NEEDED libraries ===")
    for n in info["needed"]:
        print("   ", n)
    ril_needed = [n for n in info["needed"] if "ril" in n.lower()]
    print("\n[*] RIL NEEDED:", ril_needed)
    key_imports = [i for i in info["imports"] if i in
                   ("dlopen", "dlsym", "dlclose", "system", "popen", "execv",
                    "setuid", "getuid", "__system_property_get")]
    print("\n=== key imports ===")
    for i in key_imports:
        print("   ", i)
    key_exports = [e for e in info["exports"] if e in
                   ("main", "_Z7onclickv", "_Z3AESv", "_Z10RSA_verifyPKhi") or
                   e.startswith("_Z24Encrypt") or e.startswith("_Z18Encrypt") or
                   e.startswith("_Z9Get_Build")]
    print("\n=== key exports (auth/RIL) ===")
    for e in key_exports:
        print("   ", e)

    # 2) مداخل ZIP الخاصة بـ libril.so
    print("\n=== ZIP entries: lib/arm64-v8a/libril.so ===")
    for hit in (0x2f8517d, 0x2fdacff, 0x3c65a2d, 0x3cba02d):
        s0 = data.rfind(b"PK\x01\x02", max(0, hit - 200), hit)
        if s0 < 0:
            print(f"   hit {hex(hit)}: (local-header copy, not CD)")
            continue
        (_sig, _vm, _vn, _fl, method, _mt, _md, crc, csize, usize,
         nlen, _el, _cl, _dk, _ia, _ea, _lh) = struct.unpack_from("<IHHHHHHIIIHHHHHII", data, s0)
        name = data[s0 + 46:s0 + 46 + nlen].decode("latin1")
        print(f"   CD@{hex(s0)}  {name}  method={method} csize={csize:,} usize={usize:,} crc={hex(crc)}")

    # 3) سلاسل نصية مرجعية
    for label, off, needle in [
        ("libplayservice.so", 0x16ba75e, b"libplayservice"),
        ("libsecril-client.so", 0x68a2222, b"libsecril-client.so"),
        ("libsec-ril.so", 0x68a2236, b"libsec-ril.so"),
    ]:
        assert needle in data[off - 16:off + 32], f"missing {label}"
        print(f"[+] verified string: {label} @ KEY-Tool.DMP {hex(off)}")

    print("\nDone.")


if __name__ == "__main__":
    main()
