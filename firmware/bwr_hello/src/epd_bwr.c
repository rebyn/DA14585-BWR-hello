#include "epd_bwr.h"
#include <string.h>

uint8_t epd_black[EPD_PLANE_SIZE];
uint8_t epd_red[EPD_PLANE_SIZE];

void epd_pack_wire(const uint8_t *plane, uint8_t *wire) {
    memset(wire, 0, EPD_WIRE_PLANE_SIZE);
    for (int py = 0; py < EPD_WIRE_H; py++) {
        for (int px = 0; px < EPD_WIRE_W; px++) {
            int cx = EPD_W - 1 - py, cy = px;
            uint8_t bit = (plane[cy * EPD_ROW_BYTES + (cx >> 3)] >> (7 - (cx & 7))) & 1;
            if (bit) wire[py * EPD_WIRE_ROW_BYTES + (px >> 3)] |= (uint8_t)(0x80 >> (px & 7));
        }
    }
}

static void cmd1(uint8_t c, uint8_t d) { epd_write_cmd(c); epd_write_data(d); }

void epd_init(void) {
    epd_reset();
    cmd1(0x12, 0x00);            // SWRESET (data byte ignored, keeps fn shape)
    epd_wait_idle(500);
    // Driver output control: 212 rows-1 = 0xD3, GD=0, SM=0
    epd_write_cmd(0x01);
    epd_write_data(0xD3); epd_write_data(0x00); epd_write_data(0x00);
    epd_write_cmd(0x11); epd_write_data(0x01);   // data entry X+1/Y+1
    epd_write_cmd(0x44); epd_write_data(0x00); epd_write_data(0x0D); // RAM-X 0..104/8
    epd_write_cmd(0x45); epd_write_data(0x00); epd_write_data(0x00); // RAM-Y 0..
    epd_write_data(0xD3); epd_write_data(0x00);
    epd_write_cmd(0x3C); epd_write_data(0x05);   // border: follow LUT
    epd_write_cmd(0x18); epd_write_data(0x80);   // temp sensor: internal
    // NOTE: no 0x32 LUT here: panel OTP waveform (safe default for unknown lot).
    epd_write_cmd(0x22); epd_write_data(0xF0);   // power on
    epd_write_cmd(0x20);
    epd_wait_idle(500);
}

void epd_display(void) {
    // RAM window + cursor home (104x212 wire geometry)
    epd_write_cmd(0x44); epd_write_data(0x00); epd_write_data(0x0D); // RAM-X 0..13
    epd_write_cmd(0x45); epd_write_data(0x00); epd_write_data(0x00); // RAM-Y 0..
    epd_write_data(0xD3); epd_write_data(0x00);                      // ..211
    epd_write_cmd(0x4E); epd_write_data(0x00);
    epd_write_cmd(0x4F); epd_write_data(0x00); epd_write_data(0x00);
    static uint8_t wire[EPD_WIRE_PLANE_SIZE];
    epd_write_cmd(0x24);
    epd_pack_wire(epd_black, wire);
    epd_write_buf(wire, EPD_WIRE_PLANE_SIZE);
    epd_write_cmd(0x26);
    epd_pack_wire(epd_red, wire);
    epd_write_buf(wire, EPD_WIRE_PLANE_SIZE);
    epd_write_cmd(0x22); epd_write_data(0xF7);   // display update control
    epd_write_cmd(0x20);                          // master activation
    epd_wait_idle(5000);
}

void epd_sleep(void) {
    epd_write_cmd(0x10); epd_write_data(0x01);    // deep sleep
}
