#ifndef EPD_BWR_H_
#define EPD_BWR_H_
// Tri-color 2.13" driver (E0213A192-class, UC8151C/SSD1680-BWR).
// Two planes: black (cmd 0x24) + red (cmd 0x26). 1=white in both.
//- row-major, MSB-first, 1=white, like HEMA gfx.
//- full update: 0x22 0xF7, 0x20, wait BUSY low (active-high-while-busy).
//- init: SW reset 0x12, driver control 0x01 (250x122), data entry 0x11,
//-        border 0x3C, OTP waveform (no hand LUT yet), power on 0x22 0xF0+0x20.
#include <stdint.h>
#include <stdbool.h>

#define EPD_W 250
#define EPD_H 122
#define EPD_ROW_BYTES ((EPD_W + 7) / 8)          // 32
#define EPD_PLANE_SIZE (EPD_ROW_BYTES * EPD_H)   // 3904

extern uint8_t epd_black[EPD_PLANE_SIZE];  // 0x24
extern uint8_t epd_red[EPD_PLANE_SIZE];    // 0x26

// --- transport: implemented in main.c against SDK6 GPIO/SPI. ---
void epd_write_cmd(uint8_t cmd);
void epd_write_data(uint8_t v);
void epd_write_buf(const uint8_t *p, int n);
void epd_wait_idle(unsigned timeout_ms);
void epd_reset(void);

// --- driver: pure sequence, calls transport above. ---
void epd_init(void);          // reset + init seq (OTP waveform)
void epd_display(void);       // push both planes + full update
void epd_sleep(void);         // 0x10 deep sleep after paint

// Build the on-wire plane bytes for one row (MSB-first, 1=white).
// Pure: used by epd_display() and the host test.
static inline uint8_t epd_row_byte(const uint8_t *plane, int row, int colbyte) {
    return plane[row * EPD_ROW_BYTES + colbyte];
}
#endif
