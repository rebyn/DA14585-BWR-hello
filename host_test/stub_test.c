/* Host test for the portable modules (gfx, board, ble_ota).
 * Build: cc -Wall -Wextra -I ../firmware/bwr_hello/src \
 *   ../firmware/bwr_hello/src/epd_bwr.c ../firmware/bwr_hello/src/gfx.c \
 *   ../firmware/bwr_hello/src/board.c ../firmware/bwr_hello/src/ble_ota.c \
 *   stub_test.c -o /tmp/bwr_test && /tmp/bwr_test
 * (Transport stubs: real SPI/GPIO/BLE live in main.c against SDK6.)
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "epd_bwr.h"
#include "gfx.h"
#include "board.h"
#include "ble_ota.h"
#include "factory_reset.h"
#include "clock_nav.h"
#include "demo.h"

void epd_write_cmd(uint8_t c) { (void)c; }
void epd_write_data(uint8_t v) { (void)v; }
void epd_write_buf(const uint8_t *p, int n) { (void)p; (void)n; }
void epd_wait_idle(unsigned m) { (void)m; }
void epd_reset(void) { }

int main(void) {
    board_t b;
    uint8_t rec[16];
    memset(rec, 0xFF, sizeof rec); rec[0] = 0x09;
    assert(board_decode(rec, &b) == 0 && b.have_pinmap == 0 && b.panel == 0x09);
    rec[1] = 0x01;
    uint8_t m[8] = {0x21,0x22,0x10,0x01,0x20,0x07,0x11,0x23};
    memcpy(rec + 8, m, 8);
    assert(board_decode(rec, &b) == 1 && b.port[0] == 2 && b.pin[0] == 1);
    gfx_hello();
    int n = 0;
    for (int i = 0; i < EPD_PLANE_SIZE; i++) if (epd_black[i] != 0xFF) n++;
    assert(n > 0);
    for (int i = 0; i < EPD_PLANE_SIZE; i++) assert(epd_red[i] == 0xFF);
    static uint8_t wire[EPD_WIRE_PLANE_SIZE];
    int w = 0;
    epd_pack_wire(epd_black, wire);
    for (int i = 0; i < EPD_WIRE_PLANE_SIZE; i++) if (wire[i] != 0x00) w++;
    assert(w > 0);   // rotation preserves ink (white=1 -> wire has set bits)
    epd_pack_wire(epd_red, wire);
    for (int i = 0; i < EPD_WIRE_PLANE_SIZE; i++) assert(wire[i] == 0xFF);
    assert(gfx_glyph('H') && gfx_glyph('Z') && !gfx_glyph('{'));
    uint8_t h[64];
    ota_header(h, 1000, 0x12345678, 1);
    assert(h[0] == 0x70 && h[1] == 0x51 && h[2] == 0xAA && h[32] == 0);
    uint8_t f[16] = {0,0,0x79,0x13,0xA5,0xF9,0x86,0xEC,0x5A,0x06,0x09,0,0,0,0,0};
    assert(ota_magic_find(f, 16) == 2 && ota_magic_find(f, 8) == -1);
    assert(ota_crc((uint8_t *)"abc", 3) == 0x352441C2);
    printf("C-OK plane=%d black_bytes=%d\n", EPD_PLANE_SIZE, n);

    /* ---- factory-reset contract (docs/FACTORY_RESET.md) ---- */
    static uint8_t flash[0x40000];
    memset(flash, 0xA5, sizeof flash);
    // exempt sentinels: product header magic, board + panel records, banks
    flash[0x38000] = 0x70; flash[0x38001] = 0x52;
    flash[0x39000] = 0x09; flash[0x3A000] = 0x00;
    const fr_span_t banks[] = { { 0x04000, 0x10000 }, { 0x1F000, 0x10000 } };
    fr_span_t all_exempt[8];
    memcpy(all_exempt, fr_exempt_fixed, sizeof fr_exempt_fixed);
    memcpy(all_exempt + FR_NEXEMPT_FIXED, banks, sizeof banks);
    int nex = FR_NEXEMPT_FIXED + 2;
    // consequence 2, enforced: wipe list must not reach identity
    assert(fr_disjoint(fr_covered_ref, FR_NCOVERED_REF, all_exempt, nex));
    // a hostile table must trip the same check
    assert(!fr_disjoint((const fr_span_t[]){{0x38FFF, 0x200}}, 1, all_exempt, nex));
    // consequence 1, enforced by API shape: no auth token exists; a reset
    // works with zero prior state. Magic-less / wrong-opcode packets: ignored.
    uint8_t snap;
    snap = flash[0x3B000];
    assert(factory_reset_apply(flash, sizeof flash, fr_covered_ref, FR_NCOVERED_REF,
                               (uint8_t[]){0x2F, 0, 0, 0, 0}, 5) == 0);
    assert(factory_reset_apply(flash, sizeof flash, fr_covered_ref, FR_NCOVERED_REF,
                               (uint8_t[]){0x30, 0x52, 0x53, 0x54, 0x5A}, 5) == 0);
    assert(flash[0x3B000] == snap);
    // real reset: covered wiped, exempt byte-identical
    assert(factory_reset_apply(flash, sizeof flash, fr_covered_ref, FR_NCOVERED_REF,
                               (uint8_t[]){0x2F, 0x52, 0x53, 0x54, 0x5A}, 5) == 1);
    for (uint32_t i = 0; i < (uint32_t)FR_NCOVERED_REF; i++)
        for (uint32_t k = 0; k < fr_covered_ref[i].len; k++)
            assert(flash[fr_covered_ref[i].addr + k] == 0xFF);
    assert(flash[0x38000] == 0x70 && flash[0x38001] == 0x52);
    assert(flash[0x39000] == 0x09 && flash[0x3A000] == 0x00);
    for (uint32_t i = 0; i < 2; i++)
        for (uint32_t k = 0; k < banks[i].len; k++)
            assert(flash[banks[i].addr + k] == 0xA5);
    printf("C-OK factory-reset contract\n");

    /* ---- clock/nav minimal port (esp32-c3-mini) ---- */
    cnv_clock_t clk = {0};
    cnv_weather_t wx = {0};
    cnv_nav_t nav = {0};
    assert(!cnv_parse(NULL, 0, &clk, &wx, &nav));
    assert(!cnv_parse((uint8_t[]){0xA0, 1}, 2, &clk, &wx, &nav));  // OTA ignored
    assert(cnv_parse((uint8_t[]){'T', 22, 31}, 3, &clk, &wx, &nav));
    assert(clk.valid && clk.h == 22 && clk.m == 31);
    assert(!cnv_parse((uint8_t[]){'T', 24, 0}, 3, &clk, &wx, &nav));
    assert(cnv_parse((uint8_t[]){'W', 25, 2}, 3, &clk, &wx, &nav));
    assert(wx.valid && wx.temp_c == 25 && wx.icon == 2);
    uint8_t nbuf[] = {'N','8','4',' ','k','m',0,'R','e',' ','p','h','a','i',0};
    assert(cnv_parse(nbuf, sizeof nbuf, &clk, &wx, &nav) && nav.active);
    cnv_render(&clk, &wx, &nav);
    int nb = 0, nr = 0;
    for (int i = 0; i < EPD_PLANE_SIZE; i++) {
        if (epd_black[i] != 0xFF) nb++;
        if (epd_red[i] != 0xFF) nr++;
    }
    assert(nb > 0 && nr > 0);  // nav dist paints RED plane
    assert(cnv_parse((uint8_t[]){'C'}, 1, &clk, &wx, &nav) && !nav.active);
    assert(cnv_throttle(0, 0, true) && !cnv_throttle(1000, 0, false));
    assert(cnv_throttle(5000, 0, false));
    printf("C-OK clock-nav port\n");

    /* ---- boot demo loop (same steps main.c replays) ---- */
    assert(demo_count() > 0);
    assert(!demo_apply(-1, &clk, &wx, &nav) && !demo_apply(demo_count(), &clk, &wx, &nav));
    for (int s = 0; s < demo_count(); s++) assert(demo_apply(s, &clk, &wx, &nav));
    assert(!nav.active);  // last seed step clears nav
    printf("C-OK boot demo steps=%d\n", demo_count());
    return 0;
}
