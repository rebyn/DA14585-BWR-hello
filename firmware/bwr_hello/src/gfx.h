#ifndef GFX_H_
#define GFX_H_
// 1bpp text onto epd_black; red plane left white (hello is black-only).
// 5x7 subset for "Hello world!". Host test mirrors this file exactly.
#include <stdint.h>
#include "epd_bwr.h"

void gfx_clear(uint8_t color);   // 0=black, 1=white
void gfx_pixel(int x, int y, uint8_t color);
void gfx_text(int x, int y, const char *s, uint8_t scale);
int gfx_text_width(const char *s, uint8_t scale);
void gfx_hello(void);            // clear white + centered "Hello world!"

// Glyph: 7 rows of 5 bits, MSB=row left. Only chars in "Helo wrd!".
const uint8_t *gfx_glyph(char c);  // NULL if not in subset
#endif
