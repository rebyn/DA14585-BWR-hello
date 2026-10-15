#!/usr/bin/env python3
"""Render a vertical todo list for the 212x104 tag and pack it to panel wire
order (104x212, 13 B/row, 2756 B black plane + 2756 B red plane).
Outputs: todo_preview.png (canvas), todo_panel.pbm, todo_image.bin."""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
CW, CH = 212, 104
PW, PH = 104, 212
PLANE = 13 * 212

img = Image.new("1", (CW, CH), 1)
d = ImageDraw.Draw(img)
f_title = ImageFont.load_default(size=15)
f_task = ImageFont.load_default(size=13)

TASKS = [
    ("Buy milk", True),
    ("Call mom", True),
    ("Write report", False),
    ("Gym 18:00", False),
    ("Read book", False),
]

d.text((8, 2), "TODO TODAY", font=f_title, fill=0)
d.line([(8, 19), (CW - 8, 19)], fill=0, width=1)

y = 24
for text, done in TASKS:
    d.rectangle([(10, y + 1), (19, y + 10)], outline=0, width=1)
    if done:
        d.line([(11, y + 2), (18, y + 9)], fill=0, width=1)
        d.line([(11, y + 9), (18, y + 2)], fill=0, width=1)
    d.text((24, y - 1), text, font=f_task, fill=0)
    if done:
        tw = d.textlength(text, font=f_task)
        d.line([(24, y + 5), (24 + tw, y + 5)], fill=0, width=1)
    y += 16

img.save(os.path.join(HERE, "todo_preview.png"))

px = img.load()
fb = bytearray([0xFF]) * (((CW + 7) // 8) * CH)
for yy in range(CH):
    for xx in range(CW):
        if px[xx, yy] == 0:
            fb[yy * 27 + (xx >> 3)] &= ~(0x80 >> (xx & 7))

pn = bytearray(PLANE)
for py in range(PH):
    for pxx in range(PW):
        cx, cy = CW - 1 - py, pxx
        if not (fb[cy * 27 + (cx >> 3)] >> (7 - (cx & 7))) & 1:
            pn[py * 13 + (pxx >> 3)] &= ~(0x80 >> (pxx & 7))
black = bytes(pn)
assert len(black) == PLANE, len(black)
assert any(b != 0xFF for b in black), "nothing drawn"

with open(os.path.join(HERE, "todo_panel.pbm"), "wb") as f:
    f.write(f"P4\n{PW} {PH}\n".encode() + black)
with open(os.path.join(HERE, "todo_image.bin"), "wb") as f:
    f.write(black + bytes([0xFF]) * PLANE)
print(f"OK tasks={len(TASKS)} done={sum(1 for _, t in TASKS if t)} plane={PLANE}B total={2*PLANE}B")
