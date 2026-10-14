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
    assert(gfx_glyph('H') && !gfx_glyph('Z'));
    uint8_t h[64];
    ota_header(h, 1000, 0x12345678, 1);
    assert(h[0] == 0x70 && h[1] == 0x51 && h[2] == 0xAA && h[32] == 0);
    uint8_t f[16] = {0,0,0x79,0x13,0xA5,0xF9,0x86,0xEC,0x5A,0x06,0x09,0,0,0,0,0};
    assert(ota_magic_find(f, 16) == 2 && ota_magic_find(f, 8) == -1);
    assert(ota_crc((uint8_t *)"abc", 3) == 0x352441C2);
    printf("C-OK plane=%d black_bytes=%d\n", EPD_PLANE_SIZE, n);
    return 0;
}
