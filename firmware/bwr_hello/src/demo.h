#ifndef DEMO_H_
#define DEMO_H_
// Boot demo loop: replays the seed trip (tools/mkdemo.py) through the same
// cnv_parse() path the phone's BLE writes take, so the panel re-renders
// step by step without a phone. Tables live in generated demo_steps.h.
#include <stdbool.h>
#include "clock_nav.h"

#define DEMO_DWELL_S 8  // per-frame pause; full BWR refresh takes seconds

// Apply step s (0..demo_count()-1) to state. Returns false on bad index or
// rejected payload (both are firmware bugs: tables are generated valid).
bool demo_apply(int s, cnv_clock_t *clk, cnv_weather_t *wx, cnv_nav_t *nav);
int demo_count(void);
#endif
