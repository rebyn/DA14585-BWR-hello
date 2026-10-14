#include "epd_bwr.h"

uint8_t epd_black[EPD_PLANE_SIZE];
uint8_t epd_red[EPD_PLANE_SIZE];

static void cmd1(uint8_t c, uint8_t d) { epd_write_cmd(c); epd_write_data(d); }

void epd_init(void) {
    epd_reset();
    cmd1(0x12, 0x00);            // SWRESET (data byte ignored, keeps fn shape)
    epd_wait_idle(500);
    // Driver output control: 122 rows-1 = 0x79, GD=0, SM=0
    epd_write_cmd(0x01);
    epd_write_data(0x79); epd_write_data(0x00); epd_write_data(0x00);
    epd_write_cmd(0x11); epd_write_data(0x01);   // data entry X+1/Y+1
    epd_write_cmd(0x44); epd_write_data(0x00); epd_write_data(0xF9); // RAM-X 0..250/8
    epd_write_cmd(0x45); epd_write_data(0x00); epd_write_data(0x00); // RAM-Y 0..
    epd_write_data(0x79); epd_write_data(0x00);
    epd_write_cmd(0x3C); epd_write_data(0x05);   // border: follow LUT
    epd_write_cmd(0x18); epd_write_data(0x80);   // temp sensor: internal
    // NOTE: no 0x32 LUT here: panel OTP waveform (safe default for unknown lot).
    epd_write_cmd(0x22); epd_write_data(0xF0);   // power on
    epd_write_cmd(0x20);
    epd_wait_idle(500);
}

void epd_display(void) {
    // RAM window + cursor home
    epd_write_cmd(0x44); epd_write_data(0x00); epd_write_data(0xF9);
    epd_write_cmd(0x45); epd_write_data(0x00); epd_write_data(0x00);
    epd_write_data(0x79); epd_write_data(0x00);
    epd_write_cmd(0x4E); epd_write_data(0x00);
    epd_write_cmd(0x4F); epd_write_data(0x00); epd_write_data(0x00);
    epd_write_cmd(0x24);
    epd_write_buf(epd_black, EPD_PLANE_SIZE);
    epd_write_cmd(0x26);
    epd_write_buf(epd_red, EPD_PLANE_SIZE);
    epd_write_cmd(0x22); epd_write_data(0xF7);   // display update control
    epd_write_cmd(0x20);                          // master activation
    epd_wait_idle(5000);
}

void epd_sleep(void) {
    epd_write_cmd(0x10); epd_write_data(0x01);    // deep sleep
}
