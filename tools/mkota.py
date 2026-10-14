#!/usr/bin/env python3
"""Stamp payload magic + print vendor-OTA params (webtool js/2_13c compatible)."""
import struct, sys, zlib
MAGIC = bytes([0x79,0x13,0xA5,0xF9,0x86,0xEC,0x5A,0x06])
def main():
    src, ver, dst = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    fw = open(src,'rb').read()
    if fw.find(MAGIC) < 0:
        fw += MAGIC + struct.pack('<I', ver)  # version trailer the file check scans for
    crc = zlib.crc32(fw) & 0xFFFFFFFF
    open(dst,'wb').write(fw)
    pages = (len(fw)+64+255)//256
    print(f"payload {len(fw)} B ver={ver} crc=0x{crc:08x} pages={pages} writes={pages*2+2} onair={len(fw)+64}")
    print(f"A0: a0 00 + u32LE({len(fw)})  HDR: 00aa5170/size/crc/ver@28")
if __name__ == '__main__':
    if len(sys.argv) != 4:
        sys.exit(f"usage: {sys.argv[0]} <payload.bin> <ver> <out.bin>")
    main()
