// bwr_hello main: boot -> board record -> panel init -> "Hello world!" -> sleep.
// SDK glue (GPIO/SPI/flash/BLE) is marked // SDK: below; pure logic (gfx, board,
// ota header) is host-tested. BLE OTA + notify surface stays vendor-compatible
// (see ble_ota.h) so the existing webtool (js/2_13c) flashes this image.
#include "epd_bwr.h"
#include "gfx.h"
#include "board.h"
#include "ble_ota.h"

// SDK: SysRAM vector table, app entry, watchdog, retention sleep go here
// (mirror firmware/hema_epd_clock/src/user_empty_peripheral_template.c).

// SDK: transport over SPI+GPIO at the pins from the board record.
void epd_write_cmd(uint8_t cmd) { (void)cmd; /* SDK: DC low + SPI byte */ }
void epd_write_data(uint8_t v) { (void)v; /* SDK: DC high + SPI byte */ }
void epd_write_buf(const uint8_t *p, int n) { (void)p; (void)n; /* SDK: DC high + burst */ }
void epd_wait_idle(unsigned ms) { (void)ms; /* SDK: poll BUSY low with timeout */ }
void epd_reset(void) { /* SDK: RST pulse 10ms/10ms/10ms */ }

// SDK: BLE handlers (CUSTS1 on ...0002):
//   on A0: erase inactive bank (size=u32LE), reset page cursor.
//   on A2/A3: append 128B halves; first page = 64B hdr + 192B fw.
//   on A4: verify crc32==header, write header with new imageid, reboot.
//   notifies: "fw=v0.1" "mac=.." "asset=none" "act=on" "mtu=%d" "t=%lu"
//   (same strings the webtool parses; see analysis/ota-flow.md).

int app_main(void) {
    // SDK: board_read() flash 0x039000 -> pins; on fail: drive nothing.
    gfx_hello();       // black "Hello world!", red plane stays white
    epd_init();        // OTP waveform: slow, works on every lot
    epd_display();
    epd_sleep();
    // SDK: advertise as DIY-2_13C-xxxx + EPD service, then sleep; repaint only
    // on BLE image write (0x30) or OTA commit (A4).
    for (;;) { /* SDK: __WFI(); */ }
    return 0;
}
