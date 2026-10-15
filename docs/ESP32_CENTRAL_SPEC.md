# ESP32 BLE-central spec: drive the DCLK e-tag from ESP32 firmware

Goal: move everything the webtool does (status, time sync, image upload
incl. markdown pages, OTA, test/mode commands) into our ESP32 controller
(Waveshare boards, `esp32-c3-mini` tree), which acts as BLE **central** and
data source; the tag stays a BLE **peripheral**. Reference implementation
behavior is the web app (`ancestor/WebApp/WebApp_EN/index.html`); byte
offsets below were read from it and the tag firmware, not re-derived.

Suggested topology: ESP32 as gateway — markdown over WiFi (existing
`wifi_transport`), render locally, push over BLE. Tag stays coin-cell;
ESP32 on mains.

## 1. Normative GATT contract (tag side)

Service `0xFF00`. All 16-bit UUIDs (`ancestor/src/custom_profile/user_custs1_def.c:62-65,83-116`):

| Char | Props | Len | Use |
|---|---|---|---|
| `0xFF03` control | write-with-response | 1 | unused by webtool (all commands go via `FF01`) |
| `0xFF02` ADC | read | 2 | `u16LE` millivolts (`ancestor/.../user_custs1_impl.c:104-112`: `volt=(adcval*225)>>7`) |
| `0xFF01` long-value | read + write (+write-with-response) | 160 max (`DEF_SVC1_LONG_VALUE_CHAR_LEN`) | every command + read-backs |
| `0xFF04` diag | read | 32 | boot/flash state (`spi_flash.c:370`, FF04 layout comment) |

No characteristic has notify/indicate (no CCCD in the att db). **The tag
cannot push.** The central must poll: status after every write that changes
it, version/voltage at connect. Anything event-like (OTA progress, demo
frames) is inferred client-side.

Transport rules for the central (NimBLE):

- Scan filter: name prefix `DCLK` (web app uses `namePrefix: "DCLK"`, exact
  name for `?n=` targeting).
- **MTU exchange is mandatory.** OTA/image writes are 136 B with response;
  request MTU ≥ 139 (web app relies on platform default ≥ that; ask 185+).
- One outstanding write-with-response at a time; await the response before
  the next write (the web app is strictly sequential).
- Long-value reads right after a write return the *echo of the command just
  written* until the device refreshes it: poll up to 8×300 ms for the
  expected shape (`readStatus()` pattern).

## 2. Connect sequence (mirror the web app, `index.html:730-800`)

1. Connect → discover `0xFF00` → MTU exchange.
2. Read `FF02` → volts = `u16LE/1000`. Gate: refuse OTA below 2.6 V
   (web app blocks the same way; brownout mid-slot-write is unrecoverable).
3. Read `FF04` → running version = `u16LE @20`. Expect `v54+` on current
   builds (`FF04[20..22]` = running `EPD_VERSION`; bytes 28..31 = slot
   addrs — nonstandard on this tag: `0x2000`/`0x1D000`, never assume).
4. Read `FF01` (≥16 B status): time Y(`u16LE`@0)/M/D/h/m/s (@2..6),
   panel W/H (`u16LE`@11/@13), display mode (@15), size override (@16).
5. Optional: time sync `0x91` (web app auto-syncs on connect when enabled).

## 3. Command catalog (all on `FF01`, write-with-response unless noted)

- `0x91 + Y_lo Y_hi M D h m s wday` (9 B): set clock. Verify by re-reading
  `FF01`: minutes-since-sync (`i32LE`@7) ≥ 0, else rejected (out of range).
- `0x92 + lo hi`: time calibration (no-op before first sync).
- `0x93 + mode`: 0 = clock, 1 = image (needs stored image). Then
  `readStatus()` poll; `@15` echoes the effective mode.
- `0x94 + w16 h16`: begin image upload (`w*h` must equal panel px).
- `0x95 + seq16 + ≤128 B`: chunk at `seq*128`, in order.
- `0x96 + crc32`: finish; device switches to image mode only on CRC match;
  confirm via `readStatus()` `@15 == 1`.
- Image encoding: logical row-major 1bpp, MSB first, **1 = white**, rows
  padded to whole bytes; stride = `(w+7)>>3`; 212×104 =
  27×104 = 2808 B. Drawn through `draw_pixel`, so no rotation needed.
- `0x97 + size`: panel-size override (0 auto, 1..3 = `layouts[]`+1).
- `0x98` (bare): test pattern (demo builds: steps demo frames instead).
- OTA `0xA0 + size16LE`: start; device picks inactive slot, lazy-erases.
  **Size is u16: refuse files > 65535 B** (both sides; BWR bins ~63 KB fit).
- `0xA2 + 128 B` / `0xA3 + 128 B`: page halves. Page 0 = 64 B prologue
  (`00AA5170` LE@0, size@4, crc32@8, version `0xA50F0000+filever`@28,
  `0x00`@32+... `@40=0`) + 192 B firmware; later pages pure 256 B data.
- `0xA4`: commit (device verifies CRC over received bytes, programs
  held-back magic/flag, SW resets). **Link drop = success** (reset);
  a *reply* = CRC reject, nothing changed.
- `0xA5`: abort in-flight update (also aborts on disconnect).
- After reset: reconnect (allow BLE supervision timeout), re-read `FF04`;
  success iff `running == flashed version`.

## 4. Markdown render on ESP32 (no canvas here)

The web app rasterizes with canvas fillText; ESP32 has no canvas, so port
the *algorithm*, not the code (`index.html` md section):

- Subset: `#`–`###` headings, `-`/`N.` lists, fences, `>` quotes, `---`,
  `|a|b|` rows (cells joined), paragraphs; inline `**bold**`, `` `code` ``,
  links→text; collapse whitespace runs to one space.
- Fonts: use the repo's LVGL bitmap fonts (`src/apps/navigation/
  lv_font_nav_vn_{16,20,30}.c` — Vietnamese coverage included; Montserrat
  for headings). Measure with LVGL glyph advances, wrap greedily.
- Geometry (212×104 tag): S/M/L ≈ 0.68/0.8/0.96 × base; **2 px left inset**
  (this unit's left edge is broken); no other page padding; ~1.15 line
  pitch; threshold to pure B/W (mid-gray → black).
- Paginate by measured line heights (keep headings with next line), then
  feed each page through the §3 image upload. UI: prev/next + numeric jump
  + auto-send current page when connected (guard one outstanding upload).

## 5. ESP32 implementation notes

- NimBLE central role; the tag advertises slow-ish — scan with a generous
  window, then `connect()` by name prefix match.
- Keep the web app's strict request/response pacing; add per-write timeout
  + retry (1×) + `0xA5`/abort on disconnect mid-OTA.
- Concurrency with LVGL UI: run BLE on its task, marshal results to the UI
  (same rule as `hal/esp32/app_hal.cpp`: never LVGL from NimBLE callbacks).
- Power discipline (coin-cell tag): pace full updates ≥ 60 s, never OTA
  under 2.6 V, never overlap an update with another write (no busy signal
  exists — the central must self-serialize, one outstanding op globally).
- Useful central-side displays: tag voltage/mode/version polled into the
  ESP32 UI; tag `DIAG` frame content is also readable as raw `FF04` bytes.

## 6. Acceptance (interop matrix)

For each command: send → expected tag behavior → verifying read (e.g.
`0x93 01` → image shows → `FF01@15==1`; OTA file → reset → `FF04==file
ver`; md page → pixels match host-rendered reference). Markdown layout
conformance: host Node harness procedure from the web-app commit history
(stub measure, assert fits + 2808 B + pager) ported to ESP32 font metrics.
