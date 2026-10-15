#!/usr/bin/env python3
"""Portrait todo list for the 212x104 tag.

The firmware's logical canvas is always landscape (layouts[]: 212x104) and
the 0x94/0x95 image upload carries logical row-major 1bpp, MSB first,
1 = white, len = stride(xres)*yres = 27*104 = 2808 B, CRC-checked.

So a portrait reading layout is designed on a 104x212 canvas, rotated 90 CCW
into the 212x104 landscape the firmware stores; hold the tag with its LEFT
edge up to read it (portrait-top == landscape-left).

Outputs: todo_vert_preview.png (portrait, as the eye sees it),
todo_upload.bin (2808 B row-major payload for the 0x95 chunks),
todo_panel.pbm (panel wire order, reference).
"""
import os, zlib
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
CW, CH = 212, 104          # firmware logical canvas (landscape)
PW, PH = 104, 212          # portrait design canvas
STRIDE = (CW + 7) // 8     # 27
PAYLOAD = STRIDE * CH      # 2808

portrait = Image.new("1", (PW, PH), 1)
d = ImageDraw.Draw(portrait)
f_title = ImageFont.load_default(size=18)
f_task = ImageFont.load_default(size=11)
f_foot = ImageFont.load_default(size=11)

TASKS = [
    ("Buy milk", True),
    ("Call mom", True),
    ("Write report", False),
    ("Gym 18:00", False),
    ("Read book", False),
]

d.text((8, 6), "TODO", font=f_title, fill=0)
d.line([(8, 28), (PW - 8, 28)], fill=0, width=1)

y = 40
for text, done in TASKS:
    assert d.textlength(text, font=f_task) <= PW - 36, text
    d.rectangle([(10, y + 1), (19, y + 10)], outline=0, width=1)
    if done:
        d.line([(11, y + 2), (18, y + 9)], fill=0, width=1)
        d.line([(11, y + 9), (18, y + 2)], fill=0, width=1)
    d.text((24, y - 1), text, font=f_task, fill=0)
    if done:
        tw = d.textlength(text, font=f_task)
        d.line([(24, y + 5), (24 + tw, y + 5)], fill=0, width=1)
    y += 30

done_n = sum(1 for _, t in TASKS if t)
d.text((8, PH - 16), f"{len(TASKS)} tasks - {done_n} done", font=f_foot, fill=0)
portrait.save(os.path.join(HERE, "todo_vert_preview.png"))

land = portrait.transpose(Image.ROTATE_90)
assert land.size == (CW, CH), land.size
lx = land.load()
fb = bytearray([0xFF]) * (STRIDE * CH)
for yy in range(CH):
    for xx in range(CW):
        if lx[xx, yy] == 0:  # ink -> clear bit (1 = white on the wire)
            fb[yy * STRIDE + (xx >> 3)] &= ~(0x80 >> (xx & 7))
payload = bytes(fb)
assert len(payload) == PAYLOAD, len(payload)
assert any(b != 0xFF for b in payload), "nothing drawn"
with open(os.path.join(HERE, "todo_upload.bin"), "wb") as f:
    f.write(payload)

# panel wire order reference: panel(px,py) = canvas(CW-1-py, px)
pn = bytearray(13 * 212)
for py in range(212):
    for pxx in range(104):
        cx, cy = CW - 1 - py, pxx
        if not (payload[cy * STRIDE + (cx >> 3)] >> (7 - (cx & 7))) & 1:
            pn[py * 13 + (pxx >> 3)] &= ~(0x80 >> (pxx & 7))
with open(os.path.join(HERE, "todo_panel.pbm"), "wb") as f:
    f.write(b"P4\n104 212\n" + bytes(pn))
print(f"OK tasks={len(TASKS)} done={done_n} payload={PAYLOAD}B crc32={zlib.crc32(payload)&0xFFFFFFFF:08x}")
