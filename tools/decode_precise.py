import re, struct

data = open(r"C:\Users\pc5\Documents\AT\AT", "rb").read()

def sec_range(name):
    shoff=struct.unpack_from("<Q",data,40)[0]; shent=struct.unpack_from("<H",data,58)[0]
    shnum=struct.unpack_from("<H",data,60)[0]; shstrndx=struct.unpack_from("<H",data,62)[0]
    o=shoff+shstrndx*shent
    stro=struct.unpack_from("<Q",data,o+24)[0]; sts=struct.unpack_from("<Q",data,o+32)[0]
    st=data[stro:stro+sts]
    for i in range(shnum):
        s=shoff+i*shent
        nm=st[struct.unpack_from("<I",data,s)[0]:].split(b"\0")[0].decode()
        if nm==name:
            return (struct.unpack_from("<Q",data,s+16)[0],
                    struct.unpack_from("<Q",data,s+24)[0],
                    struct.unpack_from("<Q",data,s+32)[0])

ro_addr, ro_off, ro_sz = sec_range(".rodata")
da_addr, da_off, da_sz = sec_range(".data")

REGIONS = [("rodata", ro_off, ro_off+ro_sz), ("data", da_off, da_off+da_sz)]

STRONG = [
    "/system/","/data/","/dev/","/proc/","/sdcard/","/vendor/","/sbin/","/product/",
    "toybox","busybox","127.0.0.1","nc ",
    "ro.","persist.","gsm.","ril.",
    "com.","input ","uiautomator","window_dump","resource-id","bounds=",
    "settings ","pm path","pm clear","am start","am force","dumpsys","content ",
    "at+","at&","ril","modem","smd","ttyusb","at_mdm","diag",
    "screen_off","serialno","library","conn=","len=",
    "wrong","failed","button",".xml","fota","debug","launcher","audiocore",
    "serial","version","release","tap","keyevent","swipe","port","socket","connect","bind",
]

def score(s):
    low = s.lower()
    if len(s) < 5: return 0, []
    n_hit = 0; matched = []
    for kw in STRONG:
        if kw in low:
            n_hit += 1; matched.append(kw)
            if n_hit >= 2: break
    # word-character ratio
    cls = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 /._-:=[]\"'><|&$#?!*+%,(){}~@^`\\")
    ratio = sum(1 for c in s if c in cls)/len(s)
    if ratio < 0.9: return 0, []
    sc = ratio*20 + n_hit*60
    if len(s) >= 10: sc += 5
    return sc, matched

results = {}
for rname, start, end in REGIONS:
    blob = data[start:end]
    for key in range(1, 256):
        dec = bytes(b ^ key for b in blob)
        # split on terminator: plaintext \0 => ciphertext key byte => decoded 0 at positions where byte==key
        parts = dec.split(bytes([key]))
        pos = 0
        for part in parts:
            if 5 <= len(part) <= 200:
                try:
                    s = part.decode("ascii")
                except UnicodeDecodeError:
                    s = None
                if s:
                    sc, h = score(s)
                    if sc >= 80:
                        off = start + pos
                        cur = results.get(s)
                        if cur is None or cur[0] < sc:
                            results[s] = (sc, key, off, h, rname)
            pos += len(part) + 1

rows = sorted(results.items(), key=lambda kv: (-kv[1][0], kv[0]))
print(f"unique decoded strings: {len(rows)}\n")
for s,(sc,key,off,h,rname) in rows:
    print(f"0x{key:02X} @0x{off:06x} [{rname}] score={sc:.0f} hits={h}")
    print(f"    {s}")
