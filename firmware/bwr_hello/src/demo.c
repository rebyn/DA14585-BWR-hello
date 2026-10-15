#include "demo.h"
#include "demo_steps.h"

bool demo_apply(int s, cnv_clock_t *clk, cnv_weather_t *wx, cnv_nav_t *nav) {
    if (s < 0 || s >= DEMO_STEPS) return false;
    for (int k = 0; k < demo_k[s]; k++)
        if (!cnv_parse(demo_p[s][k], demo_n[s][k], clk, wx, nav)) return false;
    return true;
}

int demo_count(void) { return DEMO_STEPS; }
