#include "gfx.h"
#include <string.h>

// Rows as 5-char strings, '#' = ink. Copied from HEMA tools/font5.py subset.
typedef struct { char c; const char *rows[7]; } G;
static const G GLYPHS[] = {
 {' ', {".....",".....",".....",".....",".....",".....","....."}},
 {'!', {"..#..","..#..","..#..","..#..","..#..",".....","..#.."}},
 {'H', {"#...#","#...#","#...#","#####","#...#","#...#","#...#"}},
 {'d', {"....#","....#",".####","#...#","#...#","#...#",".####"}},
 {'e', {".....",".....",".###.","#...#","#####","#....",".###."}},
 {'l', {".##..","..#..","..#..","..#..","..#..","..#..",".###."}},
 {'o', {".....",".....",".###.","#...#","#...#","#...#",".###."}},
 {'r', {".....",".....","#.##.","##...","#....","#....","#...."}},
 {'w', {".....",".....","#...#","#...#","#.#.#","#.#.#",".#.#."}},
};
#define NGLYPH (sizeof(GLYPHS)/sizeof(GLYPHS[0]))

const uint8_t *gfx_glyph(char c) {
    // Returns packed rows (5 bits in low bits) via static buffer per call.
    // Simple + testable; firmware scale is small.
    static uint8_t packed[7];
    for (unsigned i = 0; i < NGLYPH; i++) {
        if (GLYPHS[i].c != c) continue;
        for (int r = 0; r < 7; r++) {
            uint8_t v = 0;
            for (int col = 0; col < 5; col++)
                if (GLYPHS[i].rows[r][col] == '#') v |= (uint8_t)(0x10 >> col);
            packed[r] = v;
        }
        return packed;
    }
    return 0;
}

void gfx_clear(uint8_t color) {
    memset(epd_black, color ? 0xFF : 0x00, EPD_PLANE_SIZE);
    memset(epd_red, 0xFF, EPD_PLANE_SIZE);  // no red in hello
}

void gfx_pixel(int x, int y, uint8_t color) {
    if (x < 0 || y < 0 || x >= EPD_W || y >= EPD_H) return;
    uint8_t *b = &epd_black[y * EPD_ROW_BYTES + (x >> 3)];
    uint8_t m = (uint8_t)(0x80 >> (x & 7));
    if (color) *b |= m; else *b &= (uint8_t)~m;
}

static void glyph_draw(int x, int y, const uint8_t *g, uint8_t scale) {
    for (int r = 0; r < 7; r++)
        for (int col = 0; col < 5; col++) {
            uint8_t ink = (g[r] >> (4 - col)) & 1;
            for (uint8_t dy = 0; dy < scale; dy++)
                for (uint8_t dx = 0; dx < scale; dx++)
                    gfx_pixel(x + col * scale + dx, y + r * scale + dy,
                              ink ? 0 : 1);
        }
}

void gfx_text(int x, int y, const char *s, uint8_t scale) {
    if (!scale) scale = 1;
    for (; *s; s++) {
        const uint8_t *g = gfx_glyph(*s);
        if (g) glyph_draw(x, y, g, scale);
        x += (int)(6 * scale);  // 5px cell + 1px gap
    }
}

int gfx_text_width(const char *s, uint8_t scale) {
    int n = 0;
    while (*s++) n++;
    return n ? (n * 6 - 1) * scale : 0;
}

void gfx_hello(void) {
    const char *msg = "Hello world!";
    uint8_t scale = 4;
    gfx_clear(1);
    int w = gfx_text_width(msg, scale);
    int x = (EPD_W - w) / 2;
    int y = (EPD_H - 7 * scale) / 2;
    gfx_text(x, y, msg, scale);
}
