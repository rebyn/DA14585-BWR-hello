# Factory reset (`0x2F`) — DO NOT BREAK. The last fallback.

Source: vendor webtool `js/2_13c/main.js:54-77,175` (adapted copy of the
original 4.2" tool; `EPD_CMD_FACTORY_RESET`). This is the recovery path for a
user with no probe and a broken config — it must work on OUR firmware with
identical semantics, or a bad face becomes a mail-in repair.

## Wire contract (exact)

- Opcode `0x2F`, payload 4 bytes: `52 53 54 5A` (`'R','S','T',0x5A`).
- Firmware **ignores packets without the magic** (stray-write guard).
- No reply is read: device reboots + drops the link; webtool only logs
  «reconnect after a few seconds». Our implementation must reboot promptly —
  a delayed reboot looks like a hang and invites a battery pull mid-erase.
- Available without privilege: our firmware has no activation lock, so `0x2F`
  always works. (On vendor firmware every command except `0x26` is refused
  until RSA activation — ours must NOT reproduce that gate on this opcode.)

## What it wipes (follow the confirm dialog — the user's contract)

- Config: mode, pin style, 12/24h, bold, sleep windows, intervals.
- Both «Tự thiết kế» layouts + icons.
- Timetable.
- **All image slots** (dialog: «tất cả ảnh đã gửi vào khe», «Không hoàn tác
  được»). NOTE: a code comment claims uploaded images are kept — the dialog
  says deleted. The dialog is what the user consented to; «về như lúc mới»
  (like-new) means empty slots. Implement DELETE, revisit only with HW proof.

## What it MUST NEVER touch (reset-exempt)

- Firmware banks + product header `0x38000` (reset is settings-only; touching
  these turns a config problem into a brick).
- Hardware records `0x39000` (panel/pins) and `0x3A000` (resolution) — wiping
  identity = dead panel with no error message (cf. PORT_CHECKLIST §D).
- Activation: vendor keeps the RSA signature; we have no lock, so the
  «keeps activation» clause is trivially satisfied. Do NOT add any lock that
  reset would need to preserve.

## Design rule for all future settings

Every mutable state we add lives in reset-covered sectors; hardware identity
lives in reset-exempt sectors. A setting reset cannot clear is a future
mail-in. When adding a record, extend the `0x2F` wipe list in the same commit
— never separately.

## Test (no HW needed for the contract, HW needed for the proof)

- Unit: wipe routine against a fake flash image — assert settings sectors are
  `0xFF`, banks/headers/records byte-identical, magic-less packet ignored.
- HW: break a face on purpose, run «Khôi phục cài đặt gốc» from the vendor
  webtool against OUR firmware, confirm: reboot, disconnect, reconnect shows
  the built-in default, slots empty.
