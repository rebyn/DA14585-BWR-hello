#include "clock_nav.h"
#include "epd_bwr.h"
#include "gfx.h"
#include <string.h>

void cnv_sanitize(char *dst, size_t dstn, const char *src, size_t srclen) {
    if (dstn == 0) return;
    size_t j = 0;
    for (size_t i = 0; i < srclen && j + 1 < dstn; i++) {
        char c = src[i];
        if (c == '\0') break;
        // Strip UTF-8 continuation/lead bytes (phone sends ASCII fallback
        // like "gio"/"phut" per esp32 keynote 5.6; diacritics never reach us).
        if ((unsigned char)c < 0x20 || (unsigned char)c > 0x7E) {
            if ((unsigned char)c < 0x80) continue;  // control: drop
            continue;                               // non-ASCII: drop
        }
        dst[j++] = c;
    }
    dst[j] = '\0';
}

static bool parse_nav(const uint8_t *buf, size_t n, cnv_nav_t *nav) {
    // buf[0]=='N', rest = dist0 instr0
    size_t i = 1, dlen = 0;
    while (i + dlen < n && buf[i + dlen] != 0) dlen++;
    if (i + dlen >= n) return false;  // no NUL after dist
    size_t j0 = i + dlen + 1, ilen = 0;
    while (j0 + ilen < n && buf[j0 + ilen] != 0) ilen++;
    // instr may be unterminated at end of write: accept, clamp.
    cnv_sanitize(nav->dist, sizeof nav->dist, (const char *)(buf + i), dlen);
    cnv_sanitize(nav->instr, sizeof nav->instr, (const char *)(buf + j0), ilen);
    nav->active = true;
    return true;
}

bool cnv_parse(const uint8_t *buf, size_t n,
               cnv_clock_t *clk, cnv_weather_t *wx, cnv_nav_t *nav) {
    if (!buf || n < 1) return false;
    switch (buf[0]) {
    case 'T':
        if (n < 3 || buf[1] > 23 || buf[2] > 59) return false;
        clk->h = buf[1]; clk->m = buf[2]; clk->valid = true;
        return true;
    case 'W':
        if (n < 3 || buf[2] > 4) return false;
        wx->temp_c = (int8_t)buf[1]; wx->icon = buf[2]; wx->valid = true;
        return true;
    case 'N':
        return parse_nav(buf, n, nav);
    case 'C':
        nav->active = false;
        nav->dist[0] = '\0'; nav->instr[0] = '\0';
        return true;
    default:
        return false;  // OTA A0/A2/A3/A4 + unknown: ignore
    }
}

bool cnv_throttle(uint32_t now_ms, uint32_t last_ms, bool force) {
    if (force) return true;
    return (now_ms - last_ms) >= CNV_MIN_MS;
}

// --- layout (212x104 logical) ---
// y=4:  clock "HH:MM" scale 3 left; weather "23C S" scale 2 right.
// y=40: hairline.
// y=46: nav dist scale 2 (RED ink when active, esp32 green-label equiv).
// y=68,84: nav instr wrapped to 2x35ch scale 1 (ve-huong split equiv: hard
//           wrap at space; esp32 nav_split_ve_huong kept phone-side).
static void hline(int y) {
    for (int x = 0; x < EPD_W; x++) gfx_pixel(x, y, 0);
}

static void gfx_red_text(int x, int y, const char *s, uint8_t scale) {
    // Draw into red plane: same cell metrics as gfx_text, 0=red ink.
    if (!scale) scale = 1;
    for (; *s; s++) {
        const uint8_t *g = gfx_glyph(*s);
        if (!g && *s >= 'a' && *s <= 'z')
            g = gfx_glyph((char)(*s - 'a' + 'A'));
        if (g) {
            for (int r = 0; r < 7; r++)
                for (int col = 0; col < 5; col++) {
                    uint8_t ink = (g[r] >> (4 - col)) & 1;
                    if (!ink) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                            for (uint8_t dx = 0; dx < scale; dx++) {
                                int px = x + col * scale + dx, py = y + r * scale + dy;
                                if (px < 0 || py < 0 || px >= EPD_W || py >= EPD_H) continue;
                                epd_red[py * EPD_ROW_BYTES + (px >> 3)] &=
                                    (uint8_t)~(0x80 >> (px & 7));
                            }
                    }
                }
        }
        x += 6 * scale;
    }
}

static const char WICON[5] = {'S', 'C', 'R', 'W', 'F'};  // sun cloud rain snow fog

void cnv_render(const cnv_clock_t *clk, const cnv_weather_t *wx,
                const cnv_nav_t *nav) {
    gfx_clear(1);
    char line[32];
    // clock
    if (clk && clk->valid) {
        char c0 = '0' + clk->h / 10, c1 = '0' + clk->h % 10;
        char c2 = '0' + clk->m / 10, c3 = '0' + clk->m % 10;
        line[0]=c0; line[1]=c1; line[2]=':'; line[3]=c2; line[4]=c3; line[5]='\0';
    } else {
        line[0]='-'; line[1]='-'; line[2]=':'; line[3]='-'; line[4]='-'; line[5]='\0';
    }
    gfx_text(4, 4, line, 3);
    // weather top-right
    if (wx && wx->valid) {
        int t = wx->temp_c;
        int k = 0;
        if (t < 0) { line[k++] = '-'; t = -t; }
        if (t >= 100) { line[k++] = '0' + t / 100; t %= 100; }
        if (t >= 10 || k > 0) line[k++] = '0' + t / 10;
        line[k++] = '0' + t % 10;
        line[k++] = 'C'; line[k++] = ' ';
        line[k++] = WICON[wx->icon > 4 ? 0 : wx->icon];
        line[k] = '\0';
        int w = gfx_text_width(line, 2);
        gfx_text(EPD_W - 4 - w, 8, line, 2);
    }
    hline(40);
    // nav
    if (nav && nav->active) {
        const char *d = nav->dist[0] ? nav->dist : "-";
        gfx_red_text(4, 46, d, 2);   // red ~= esp32 green maneuver label
        // wrap instr at 35 cols x2 lines
        char w1[36], w2[36];
        size_t L = strlen(nav->instr);
        if (L <= 35) {
            gfx_text(4, 68, nav->instr, 1);
        } else {
            size_t cut = 35;
            while (cut > 20 && nav->instr[cut] != ' ') cut--;
            if (nav->instr[cut] != ' ') cut = 35;
            memcpy(w1, nav->instr, cut); w1[cut] = '\0';
            size_t r = L - cut - (nav->instr[cut] == ' ' ? 1 : 0);
            if (r > 35) r = 35;
            memcpy(w2, nav->instr + cut + (nav->instr[cut] == ' ' ? 1 : 0), r);
            w2[r] = '\0';
            gfx_text(4, 66, w1, 1);
            gfx_text(4, 80, w2, 1);
        }
    } else {
        gfx_text(4, 46, "Ready", 2);
        gfx_text(4, 70, "Start nav on phone", 1);
    }
}
