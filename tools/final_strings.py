import re

data = open(r"C:\Users\pc5\Documents\AT\AT","rb").read()

# exhaustive: for each key, decode whole file; find strings containing these tokens
TOKENS = ["uiautomator","pm path","pm clear","Wrong","Ril ","RIL","EXECUTE","execute",
          "BOUNDS","bounds","Serial","SERIAL","GETPROP","getprop","am start","input text",
          "input tap","input keyevent","nc ","2600","6000","hiddenmenu","IOT","SETTINGS",
          "screen_off","sleep","dump","settings put","logcat","toybox","/dev/","fota",
          "library","Library","conn=","code:","%s","%d"]

hits = {}
for key in range(1,256):
    dec = bytes(b ^ key for b in data)
    for m in re.finditer(rb"[\x20-\x7e]{5,}", dec):
        s = m.group().decode("ascii")
        low = s.lower()
        # verify terminator byte == key (encoded \0)
        end = m.end()
        if end < len(data) and data[end] == key:
            pass
        else:
            continue
        matched = [t for t in TOKENS if t.lower() in low]
        if not matched:
            continue
        # cut string: strip leading padding (decoded from raw 0x00)
        core = s.lstrip(chr(key))
        if len(core) < 4:
            continue
        sc = len(core) + 200*len(matched)
        keyid = core[:60]
        if keyid not in hits or hits[keyid][0] < sc:
            hits[keyid] = (sc, key, m.start(), core, matched[:4])

rows = sorted(hits.values(), key=lambda r: -r[0])
print(f"{len(rows)} strings\n")
for sc, key, off, s, matched in rows:
    print(f"key=0x{key:02X} @0x{off:06x} hits={matched}")
    print(f"    {s}")
