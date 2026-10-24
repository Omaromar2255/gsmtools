import re, base64, zipfile, io, os

data = open(r"C:\Users\pc5\Documents\AT\AT","rb").read()
outdir = r"C:\Users\pc5\Documents\AT\extracted"
os.makedirs(outdir, exist_ok=True)

found = None
for key in range(1,256):
    dec = bytes(b ^ key for b in data)
    for m in re.finditer(rb"UEsDBB[A-Q]", dec):
        start = m.start()
        # walk back to start of base64 run
        s = start
        while s > 0:
            c = dec[s-1]
            if (0x41<=c<=0x5a) or (0x61<=c<=0x7a) or (0x30<=c<=0x39) or c in b"+/=":
                s -= 1
            else:
                break
        # walk forward
        e = start
        while e < len(dec):
            c = dec[e]
            if (0x41<=c<=0x5a) or (0x61<=c<=0x7a) or (0x30<=c<=0x39) or c in b"+/=":
                e += 1
            else:
                break
        b64 = dec[s:e]
        print(f"key=0x{key:02X} b64 range file[0x{s:x}..0x{e:x}] len={len(b64)}")
        try:
            raw = base64.b64decode(b64, validate=False)
        except Exception as ex:
            print("  b64 decode failed:", ex); continue
        if raw[:4] == b"PK\x03\x04":
            print("  >>> VALID ZIP! size", len(raw))
            ap = os.path.join(outdir, "embedded.apk")
            open(ap, "wb").write(raw)
            found = (key, ap, len(raw))
            break
    if found: break

if found:
    key, ap, n = found
    print("saved", ap, "key", hex(key), "size", n)
    z = zipfile.ZipFile(ap)
    for zi in z.infolist():
        print(f"  {zi.filename:60s} {zi.file_size}")
