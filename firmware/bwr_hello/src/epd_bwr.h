#ifndef EPD_BWR_H_
#define EPD_BWR_H_
// Tri-color 2.13" driver, 212x104 logical (landscape) = 104x212 on the wire.
// Two planes: black (cmd 0x24) + red (cmd 0x26). 1=white in both.
//- logical framebuffer: row-major, MSB-first, 1=white (what gfx draws).
//- wire order: panel(px,py) = logical(W-1-py, px), rows padded to whole
//- bytes (13 B/row x 212 rows = 2756 B/plane). Same rotation the vendor
//- webtool applies to its landscape canvas.
//- full update: 0x22 0xF7, 0x20, wait BUSY low (active-high-while-busy).
//- init: SW reset 0x12, driver control 0x01 (212x104), data entry 0x11,
//-        border 0x3C, OTP waveform (no hand LUT yet), power on 0x22 0xF0+0x20.
#include <stdint.h>
#include <stdbool.h>

#define EPD_W 212
#define EPD_H 104
#define EPD_ROW_BYTES ((EPD_W + 7) / 8)          // 27
#define EPD_PLANE_SIZE (EPD_ROW_BYTES * EPD_H)   // 2808, logical

#define EPD_WIRE_W 104
#define EPD_WIRE_H 212
#define EPD_WIRE_ROW_BYTES ((EPD_WIRE_W + 7) / 8)        // 13
#define EPD_WIRE_PLANE_SIZE (EPD_WIRE_ROW_BYTES * EPD_WIRE_H)  // 2756

extern uint8_t epd_black[EPD_PLANE_SIZE];  // 0x24, logical order
extern uint8_t epd_red[EPD_PLANE_SIZE];    // 0x26, logical order

// --- transport: implemented in main.c against SDK6 GPIO/SPI. ---
void epd_write_cmd(uint8_t cmd);
void epd_write_data(uint8_t v);
void epd_write_buf(const uint8_t *p, int n);
void epd_wait_idle(unsigned timeout_ms);
void epd_reset(void);

// --- driver: pure sequence, calls transport above. ---
void epd_init(void);          // reset + init seq (OTP waveform)
void epd_display(void);       // pack both planes to wire order + full update
void epd_sleep(void);         // 0x10 deep sleep after paint

// Pack a logical plane into wire order (1=white, MSB-first, 13 B rows).
// Pure: used by epd_display() and the host test.
void epd_pack_wire(const uint8_t *plane, uint8_t *wire);
#endif
