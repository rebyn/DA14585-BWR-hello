#ifndef GFX_H_
#define GFX_H_
// 1bpp text onto epd_black; red plane left white (hello is black-only).
// 5x7 ASCII subset: original "Hello world!" + clock/nav set
// (0-9 : / - + . % C S R W F T N E A D P M Y and A-Z via uppercasing).
// Host test mirrors this file exactly.
#include <stdint.h>
#include "epd_bwr.h"

void gfx_clear(uint8_t color);   // 0=black, 1=white
void gfx_pixel(int x, int y, uint8_t color);
void gfx_text(int x, int y, const char *s, uint8_t scale);
int gfx_text_width(const char *s, uint8_t scale);
void gfx_hello(void);            // clear white + centered "Hello world!"

// Glyph: 7 rows of 5 bits, MSB=row left. ASCII subset (see above);
// gfx_text uppercases a-z so nav text stays readable without a
// full lowercase table.
const uint8_t *gfx_glyph(char c);  // NULL if not in subset
#endif
