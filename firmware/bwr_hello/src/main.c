// bwr_hello main: boot -> board record -> panel init -> demo loop.
// Demo replays the seed trip (tools/mkdemo.py) via demo_apply(), i.e. the
// same cnv_parse() path live phone BLE writes take, re-rendering the panel
// step by step with no phone attached.
// SDK glue (GPIO/SPI/flash/BLE) is marked // SDK: below; pure logic (gfx,
// board, ota header, clock/nav, demo) is host-tested. BLE OTA + notify
// surface stays vendor-compatible (see ble_ota.h) so the existing webtool
// (js/2_13c) flashes this image.
#include "epd_bwr.h"
#include "gfx.h"
#include "board.h"
#include "ble_ota.h"
#include "clock_nav.h"  // minimal-data port of esp32-c3-mini (time/wx/nav)
#include "demo.h"       // boot demo loop (seed trip, no phone needed)

// SDK: SysRAM vector table, app entry, watchdog, retention sleep go here
// (mirror firmware/hema_epd_clock/src/user_empty_peripheral_template.c).

// SDK: transport over SPI+GPIO at the pins from the board record.
void epd_write_cmd(uint8_t cmd) { (void)cmd; /* SDK: DC low + SPI byte */ }
void epd_write_data(uint8_t v) { (void)v; /* SDK: DC high + SPI byte */ }
void epd_write_buf(const uint8_t *p, int n) { (void)p; (void)n; /* SDK: DC high + burst */ }
void epd_wait_idle(unsigned ms) { (void)ms; /* SDK: poll BUSY low with timeout */ }
void epd_reset(void) { /* SDK: RST pulse 10ms/10ms/10ms */ }
// SDK: wall-clock pause between demo frames.
void demo_sleep_s(unsigned s) { (void)s; /* SDK: low-power sleep, retention on */ }

// SDK: BLE handlers (CUSTS1 on ...0002):
//   on A0: erase inactive bank (size=u32LE), reset page cursor.
//   on A2/A3: append 128B halves; first page = 64B hdr + 192B fw.
//   on A4: verify crc32==header, write header with new imageid, reboot.
//   on data (clock_nav.h, ported from esp32-c3-mini time/wx/nav):
//     'T'+h+m, 'W'+temp+icon, 'N'+dist0+instr0, 'C' -> cnv_parse(),
//     then cnv_throttle(now,last,force) gate -> cnv_render() + epd_display().
//     Phone collapses Chronos title/duration/distance/eta/directions to the
//     two N strings (ASCII, no diacritics); 48x48 icon + LVGL + WiFi dropped.
//   notifies: "fw=v0.1" "mac=.." "asset=none" "act=on" "mtu=%d" "t=%lu"
//   (same strings the webtool parses; see analysis/ota-flow.md).

int app_main(void) {
    // SDK: board_read() flash 0x039000 -> pins; on fail: drive nothing.
    static cnv_clock_t clk;
    static cnv_weather_t wx;
    static cnv_nav_t nav;
    epd_init();  // OTP waveform: slow, works on every lot
    // Attract loop: each step parses seed BLE bytes, repaints, pauses.
    // SDK: a live ...0002 write should preempt the loop (apply cnv_parse(),
    // repaint, resume demo after idle timeout) — future, needs BLE events.
    for (;;) {
        for (int s = 0; s < demo_count(); s++) {
            if (!demo_apply(s, &clk, &wx, &nav)) continue;  // SDK: log + skip
            cnv_render(&clk, &wx, &nav);
            epd_display();
            demo_sleep_s(DEMO_DWELL_S);
        }
    }
    return 0;
}
