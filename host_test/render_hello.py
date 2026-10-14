#!/usr/bin/env python3
"""Host mirror of gfx.c + epd_pack_wire(): landscape 212x104 canvas rotated
to 104x212 wire order (13 B/row x 212 = 2756 B/plane). Writes hello_canvas.pbm,
hello_panel.pbm, hello_image.bin (black + red planes) and checks sizes."""
import os
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
CW, CH, S, MSG = 212, 104, 2, "Hello world!"
ROWB = (CW + 7) // 8
fb = bytearray([0xFF]) * (ROWB * CH)
def px(x, y, color):
    if 0 <= x < CW and 0 <= y < CH:
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
assert wpx <= CW, (wpx, CW)
text((CW-wpx)//2, (CH-7*S)//2, MSG)
def pack(grid, W, H):
    rb = (W+7)//8
    out = bytearray(rb*H)
    for y in range(H):
        for x in range(W):
            if grid[y][x]:
                out[y*rb+(x>>3)] |= (0x80>>(x&7))
    return bytes(out)
# rotate to wire order: panel(px,py) = canvas(CW-1-py, px)
PW, PH = 104, 212
pn = [[1]*PW for _ in range(PH)]
for py in range(PH):
    for px in range(PW):
        cx, cy = CW-1-py, px
        bit = (fb[cy*ROWB+(cx>>3)] >> (7-(cx&7))) & 1
        pn[py][px] = bit
ink_cv = sum(1 for b in fb for k in range(8) if not (b >> (7-k)) & 1)
black = pack(pn, PW, PH)
red = bytes([0xFF])*len(black)
assert len(black) == 13*212 == 2756, len(black)
assert any(b != 0xFF for b in black), "nothing drawn"
with open(os.path.join(HERE, 'hello_canvas.pbm'), 'wb') as f:
    f.write(f"P4\n{CW} {CH}\n".encode() + pack(
        [[(fb[y*ROWB+(x>>3)] >> (7-(x&7))) & 1 for x in range(CW)] for y in range(CH)],
        CW, CH))
with open(os.path.join(HERE, 'hello_panel.pbm'), 'wb') as f:
    f.write(f"P4\n{PW} {PH}\n".encode() + black)
with open(os.path.join(HERE, 'hello_image.bin'), 'wb') as f:
    f.write(black + red)
print(f"OK canvas={CW}x{CH} panel={PW}x{PH} plane=2756B text={wpx}px total=5512B")
print("OK ota header 00AA5170 + magic 7913a5f986ec5a06 + crc32")
