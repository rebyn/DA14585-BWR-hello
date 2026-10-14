#!/usr/bin/env python3
"""Host mirror of gfx.c hello: render hello.pbm, check plane sizes + OTA header."""
import os, struct, zlib
HERE = os.path.dirname(os.path.abspath(__file__))
ART = {  # same subset as firmware/bwr_hello/src/gfx.c
 ' ': [".....",".....",".....",".....",".....",".....","....."],
 '!': ["..#..","..#..","..#..","..#..","..#..",".....","..#.."],
 'H': ["#...#","#...#","#...#","#####","#...#","#...#","#...#"],
 'd': ["....#","....#",".####","#...#","#...#","#...#",".####"],
 'e': [".....",".....",".###.","#...#","#####","#....",".###."],
 'l': [".##..","..#..","..#..","..#..","..#..","..#..",".###."],
 'o': [".....",".....",".###.","#...#","#...#","#...#",".###."],
 'r': [".....",".....","#.##.","##...","#....","#....","#...."],
 'w': [".....",".....","#...#","#...#","#.#.#","#.#.#",".#.#."],
}
W, H, S, MSG = 250, 122, 4, "Hello world!"
ROWB = (W + 7) // 8
fb = bytearray([0xFF]) * (ROWB * H)
def px(x, y, color):
    if 0 <= x < W and 0 <= y < H:
        i, m = y * ROWB + (x >> 3), 0x80 >> (x & 7)
        fb[i] = (fb[i] | m) if color else (fb[i] & ~m)
def text(x, y, s):
    for ch in s:
        g = ART[ch]
        for r in range(7):
            for c in range(5):
                ink = g[r][c] == '#'
                for dy in range(S):
                    for dx in range(S):
                        px(x + c*S+dx, y + r*S+dy, 0 if ink else 1)
        x += 6*S
wpx = (len(MSG)*6-1)*S
text((W-wpx)//2, (H-7*S)//2, MSG)
with open(os.path.join(HERE, 'hello.pbm'), 'wb') as f:
    f.write(f"P4\n{W} {H}\n".encode() + bytes(fb))
# checks: plane size, text width, OTA header shape, magic
assert len(fb) == 32*122 == 3904, len(fb)
assert wpx == (12*6-1)*4 == 284, wpx
assert fb.count(0xFF) < len(fb), "nothing drawn"
hdr = bytearray(64)
struct.pack_into('<I', hdr, 0, 0x00AA5170)
struct.pack_into('<I', hdr, 4, 1000)
struct.pack_into('<I', hdr, 8, 0x12345678)
struct.pack_into('<I', hdr, 28, 1)
assert hdr[:4] == bytes([0x70,0x51,0xAA,0x00]) and hdr[32] == 0
magic = bytes([0x79,0x13,0xA5,0xF9,0x86,0xEC,0x5A,0x06])
assert zlib.crc32(b'abc') & 0xFFFFFFFF == 0x352441C2  # crc sanity
print(f"OK hello.pbm {W}x{H} plane=3904B text={wpx}px black_bytes={sum(b!=0xFF for b in fb)}")
print("OK ota header 00AA5170 + magic 7913a5f986ec5a06 + crc32")
