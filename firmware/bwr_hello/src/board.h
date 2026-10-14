#ifndef BOARD_H_
#define BOARD_H_
// Board record at flash 0x039000 (same mechanism as HEMA/vendor):
// +0x00 panel byte (0x14=A53 122x250, 0x09=A41 104x212; BWR tag reports its own),
// +0x01 map select (0x01=override map, 0xFF=built-in), +0x08..0x0F packed pins.
// Hello reads it at boot for pins; geometry is fixed BWR 250x122 here.
#include <stdint.h>
#include <stdbool.h>
#define BOARD_REC_ADDR 0x039000
#define BOARD_REC_LEN 16
typedef struct {
    uint8_t panel;
    bool have_pinmap;
    uint8_t port[8], pin[8];
} board_t;
bool board_decode(const uint8_t *rec, board_t *out);  // pure, host-tested
// bool board_read(board_t *out);  // SDK: flash read + decode (main.c)
#endif
