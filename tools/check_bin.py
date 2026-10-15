#!/usr/bin/env python3
"""Validate a DA14585 OTA payload against the vendor (elink/6275) webtool's
expectations, i.e. the 8 rules from PORT_CHECKLIST/bring-up notes.

Usage: check_bin.py [--strict] <file.bin> [file2.bin ...]
Exit 0 = shippable, 2 = failed. Warnings never fail (behavioral items need HW).
"""
import struct, sys, zlib

MAGIC = bytes([0x79, 0x13, 0xA5, 0xF9, 0x86, 0xEC, 0x5A, 0x06])
RAM_LO, RAM_HI = 0x07FC0000, 0x07FD8000
HDR_SIG = bytes([0x70, 0x51])          # 7051: a pre-wrapped SUOTA bank image
MAX_U16 = 0xFFFF
errors, warnings = [], []

def err(m): errors.append(m)
def warn(m): warnings.append(m)

def check(path):
    fw = open(path, 'rb').read()
    n = len(fw)
    print(f"== {path} ({n} bytes, crc32=0x{zlib.crc32(fw)&0xFFFFFFFF:08x})")
    # 1. version marker: magic + u32 version right after
    at = fw.find(MAGIC)
    if at < 0:
        err("no version magic 7913A5F986EC5A06 -> webtool refuses the file")
    else:
        if fw.count(MAGIC) > 1:
            warn(f"magic occurs {fw.count(MAGIC)}x, expected once")
        ver = struct.unpack('<I', fw[at+8:at+12])[0]
        print(f"  magic @0x{at:X}, firmVer={ver} (0x{ver:08X})")
    # 2/3. raw payload + valid vector table at base 0x07FC0000
    if fw[:2] == HDR_SIG:
        err("file starts with 7051 SUOTA header -> webtool would nest it; ship RAW payload")
    if n < 8:
        err("file shorter than vector table")
        return False
    sp, pc = struct.unpack('<II', fw[:8])
    ok_sp = RAM_LO <= sp < RAM_HI
    ok_pc = (pc & 1) == 1 and RAM_LO <= (pc & ~1) < RAM_LO + max(n, 1) + 0x10000
    print(f"  SP={sp:08X} {'OK' if ok_sp else 'BAD'}  PC={pc:08X} {'OK' if ok_pc else 'BAD'}")
    if not ok_sp: err(f"SP {sp:08X} outside SysRAM; bootloader jumps nowhere")
    if not ok_pc: err(f"reset vector {pc:08X} not Thumb-in-image; first boot hard-faults")
    # 4. size fits bank + HMCLOCK u16 path
    pages = (n + 64 + 255) // 256
    print(f"  on-air {n+64}B = {pages} pages, {pages*2+2} writes; "
          f"A0: a0 00 + u32LE({n})")
    if n > 73664:
        err(f"{n}B exceeds smallest bank (~73664); image cannot fit any slot")
    elif n > MAX_U16:
        warn(f"{n}B > 65535: needs u32 size path on BOTH sender and receiver")
    # 8. vendor-service strings (behavioral: warn only, needs HW to prove)
    svc_le = bytes([0xAE, 0x6C, 0x11, 0xB6, 0x46, 0xFB, 0x8D, 0x91,
                    0x28, 0xD8, 0x01, 0x00, 0x75, 0x62])
    if svc_le not in fw and b'62750001' not in fw:
        warn("no 6275 service strings: elink webtool won't see this image after cutover")
    return True

def main():
    strict = '--strict' in sys.argv
    paths = [a for a in sys.argv[1:] if not a.startswith('-')]
    if not paths:
        sys.exit("usage: check_bin.py [--strict] <file.bin> [...]")
    for p in paths:
        try: check(p)
        except OSError as e: err(f"{p}: {e}")
    for w in warnings: print("WARN:", w)
    for e in errors: print("FAIL:", e)
    if errors or (strict and warnings):
        print(f"RESULT: NOT shippable ({len(errors)} errors, {len(warnings)} warnings)")
        return 2
    print(f"RESULT: shippable ({len(warnings)} warnings)")
    return 0

if __name__ == '__main__':
    sys.exit(main())
