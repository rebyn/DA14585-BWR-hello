# Port checklist: HMCLOCK-EN → 2.13" BWR tag (DIY-2_13C)

Source: `../HMCLOCK-EN` (terenceang fork of tpunix/HMCLOCK), paths below are
relative to it. Vendor protocol = our RE of `DIY-2_13C` (`ble_ota.h` here).

## A. Boot/flash — MATCHES, reuse as-is

- Product header `0x38000` magic `7052` + 2 slot addrs; defaults
  `0x4000`/`0x1F000` (`src/epd/spi_flash.c:415`).
- Slot header `7051AA` + imageid + size + CRC + version@28 + enc@32=0.
- Wrap-safe newest-wins (`bump_image_flag`, `pick_active_image`).
- `selflash()`: first RAM-boot installs itself into the primary slot; demands
  `EPD_VERSION` bump per build (`src/config/user_config.h:58`, now `0xA50f0035`).
- Records: `0x39000` 16B pin/panel (same decode as vendor/HEMA),
  `0x3A000` resolution (`xres@+10`, `yres@+12`, sanity `<512`).

## B. BLE — DIFFERS, one decision

- HMCLOCK: 16-bit svc `0xFF00`, chars control(1B)/ADC(2B)/long-value(160B)/
  diag(32B); commands on long-value (`src/user_custs1_impl.c:1004`):
  `0x91` clock, `0x92` cal, `0x93-96` image/mode, `0x97` size override,
  `0x98` test, `0xA0+` OTA. Web app: `WebApp/WebApp_EN/index.html`.
- Vendor: 128-bit `62750001-…`, ASCII notifies, slots + asset protocol.
- DECISION: keep HMCLOCK's stack + web app (proven OTA, simpler). Consequence:
  vendor webtool stops working after cutover — planned, not a bug.

## C. OTA — same shape, 3 dialect diffs to reconcile

1. **Size field width.** HMCLOCK: u16 @+2 (`ota_handle`, `spi_flash.c:669`).
   Vendor 2_13c: u32 @+2 (4.2" images exceed 64K). Our BWR images (~65K) sit
   just under the u16 limit — widen to u32 now, don't rediscover this later.
2. **Framing identical:** `0xA2/0xA3` halves at `buf+8`, page 0 = 64B header
   prologue + 192B data. Magic/flag held back until CRC passes at `0xA4`
   (both sides) — keep this property, it makes interrupted flashes unbootable
   rather than bricked.
3. **Keep HMCLOCK extras:** `0xA5` abort, lazy per-page 4K erase (vendor's
   erase-at-A0 risks dropping the BLE link), fit-check + header sanity.
   Drop the vendor `7913…` magic trailer unless the vendor webtool must send.

## D. Panel — port required (the actual work)

- HMCLOCK defaults 104×212, B/W only, red plane never driven (`src/epd/epd.c:423`).
- Our panel: 120×252 wire order (its `250×122` logical, cf. A22 row in Readme).
  `detect_w/h` from `0x3A000`, override via `0x97` — reuse, verify against OUR
  tag's dump before driving (P1_1 contention trap: read flash, never probe).
- Red plane: add second plane buffer + `0x26` writes + BWR init
  (OTP waveform first). Hello v1 ships B/W-only like HMCLOCK; red = phase 2.
- Note its A22 caveat: weak contrast on 250×122 (boost circuit sized for A41).

## E. Display/image — pick HMCLOCK's, drop vendor's

- HMCLOCK: `0x3B000-0x3EFFF` region (`IMG1` header, mode rec, pixels) via
  `0x93-96`. Vendor slots/`0x30` + `0x2C` asset: do not port.
- No `asset.bin` equivalent (lunar/fonts blob) — vendor calendar faces that
  need it don't transfer; our faces are self-contained scripts/glyphs.

## F. Bring-up order (safety)

1. Dump ×2, verify (`7052`/`7051`/record) — unchanged.
2. **OpenOCD RAM-run** (`Keil_5/openocd_da14585.cfg`): zero flash writes,
   validates pins + panel + BLE live on our tag. New capability vs HEMA flow.
3. Selflash persist → BLE OTA iteration via HMCLOCK web app.
4. Red plane → faces → cutover. Vendor bank kept as fallback until step 3 is
   boring (several clean OTA cycles).

## License note

Fork changes MIT (Terence Ang); original HMCLOCK states no license — credit
tpunix, don't relicense their files. Our files stay separate (`firmware/`).
