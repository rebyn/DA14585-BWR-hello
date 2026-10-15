# DA14585-BWR-hello

Minimal "Hello world" firmware for the 2.13" tri-color (B/W/R) tag
`DCLK-xxxx` (212×104, measured on-device), side-by-side with HEMA (`../DA14585-ESL-213`).

HEMA is the B/W reference (SSD1680, DSL, web UI). This repo is the BWR
starting point: boot, init the tri-color panel, paint "Hello world", stay
OTA-compatible with the vendor webtool (`62750001-...`, opcodes `A0/A2/A3/A4`).

## Layout

```
firmware/bwr_hello/src/
  main.c        boot: board record -> panel init -> hello -> sleep
  epd_bwr.h/.c  tri-color driver: 0x24 black plane, 0x26 red plane, 0x22/0x20 update
  gfx.h/.c      1bpp framebuffer + 5x7 text (subset: "Helo wrd!")
  board.h/.c    16B record at 0x039000 (panel + pin map), pure + HW split
  ble_ota.h/.c  OTA protocol: header 00AA5170, A0/A2/A3/A4 reassembly (pure parts)
tools/mkota.py  stamp payload magic + print A0/header params for the webtool
host_test/render_hello.py  host mirror of gfx: renders hello.pbm, checks sizes
```

## Panel (BWR vs HEMA B/W)

* Two RAM planes: black `0x24`, red `0x26`. White = both 1. Full update:
  `0x22 0xF7` + `0x20`, wait BUSY low. HEMA B/W uses one plane; do not reuse
  its driver verbatim.
* Geometry 212×104 logical (landscape) = 104×212 on the wire: 13 B/row ×
  212 rows = 2756 B per plane, black + red. `epd_display()` rotates at push
  time (`epd_pack_wire`), same mapping the vendor webtool applies.
* Waveform: use panel OTP (slow, safe) first. Hand-written LUT comes later
  after measuring the lot (see HEMA `tag_types.h` for why).

## BLE / OTA compat (so the existing webtool flashes it)

* Service `62750001-d828-918d-fb46-b6c11c675aec`, write+notify `...0002`,
  version `...0003`, name `DCLK-xxxx`. See `ble_ota.h`.
* File magic `79 13 A5 F9 86 EC 5A 06 + u32LE firmVer` — webtool refuses files
  without it. `mkota.py` stamps it.
* On-air: `A0 + u32LE(size)` -> 256B pages as `A2+128 / A3+128` (first page
  carries 64B header `00AA5170/size/crc/ver@28`) -> `A4` reboot into inactive bank.
* `asset.bin` (`EPDA...`) is separate, pushed via `0x2C` after OTA. Hello does
  not need it.

## Build (needs what HEMA needs)

Renesas SDK 6.0.22.1401 (account-gated, not vendored) + e2studio + LLVM ARM +
J-Link. Same flow as HEMA `tools/build.sh`: mirror `src/` into
`$SDK/.../template/bwr_hello`, build DA14585 config, wrap with `mksuota.py`
(HEMA) or `tools/mkota.py` here for OTA params. No SDK on this Mac, so this
repo compiles the *pure* parts (`gfx`, `ble_ota` header/page logic) on host;
the SDK glue (`main.c` GPIO/SPI/BLE calls) is marked `// SDK:` and reviewed,
not compiled, until an SDK machine builds it.

## Hello without hardware

```
python3 host_test/render_hello.py   # -> host_test/hello.pbm + size checks
```

## Safety

Dump flash twice before any SWD write, keep the dump: it holds the board
record at `0x039000` and the fallback bank. Full-erase without it = dead panel,
same as HEMA.
