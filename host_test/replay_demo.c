/* Replay seed demo through the REAL firmware renderer.
 * Build: cc -Wall -Wextra -I ../firmware/bwr_hello/src \
 *   ../firmware/bwr_hello/src/epd_bwr.c ../firmware/bwr_hello/src/gfx.c \
 *   ../firmware/bwr_hello/src/clock_nav.c \
 *   ../firmware/bwr_hello/src/demo.c replay_demo.c -o /tmp/replay
 * Run from host_test/: /tmp/replay  -> demo_frame_<i>_<name>.pbm (212x104)
 * Fails loudly on: rejected payload, empty black plane, missing red ink
 * on nav steps, or red ink leaking onto idle/done steps.
 * Uses firmware demo.h, the same loop main.c runs at boot.
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "epd_bwr.h"
#include "gfx.h"
#include "clock_nav.h"
#include "demo.h"
#include "demo_steps.h"  // demo_name/demo_red oracles (generated)

void epd_write_cmd(uint8_t c) { (void)c; }
void epd_write_data(uint8_t v) { (void)v; }
void epd_write_buf(const uint8_t *p, int n) { (void)p; (void)n; }
void epd_wait_idle(unsigned m) { (void)m; }
void epd_reset(void) { }

static void dump_pbm(const char *path) {
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P4\n%d %d\n", EPD_W, EPD_H);
    for (int y = 0; y < EPD_H; y++)
        fwrite(&epd_black[y * EPD_ROW_BYTES], 1, EPD_ROW_BYTES, f);
    fclose(f);
}

int main(void) {
    cnv_clock_t clk = {0};
    cnv_weather_t wx = {0};
    cnv_nav_t nav = {0};
    for (int s = 0; s < DEMO_STEPS; s++) {
        assert(demo_apply(s, &clk, &wx, &nav));
        cnv_render(&clk, &wx, &nav);
        int nb = 0, nr = 0;
        for (int i = 0; i < EPD_PLANE_SIZE; i++) {
            if (epd_black[i] != 0xFF) nb++;
            if (epd_red[i] != 0xFF) nr++;
        }
        assert(nb > 0);
        if (demo_red[s]) assert(nr > 0);
        else assert(nr == 0);
        char path[64];
        snprintf(path, sizeof path, "demo_frame_%d_%s.pbm", s, demo_name[s]);
        dump_pbm(path);
        printf("frame %d %-11s black=%d red=%d nav=%d\n",
               s, demo_name[s], nb, nr, nav.active);
    }
    printf("DEMO-OK frames=%d\n", DEMO_STEPS);
    return 0;
}
