#ifndef CLOCK_NAV_H_
#define CLOCK_NAV_H_
// Minimal-data port of esp32-c3-mini -> 212x104 BWR tag.
//
// Source (esp32): LVGL Navigation V2 + ChronosESP32 time/weather/nav.
//   docs/CHRONOS_TECHNICAL_KEYNOTE.md sections 4.2, 5.1-5.4,
//   src/apps/navigation/navigation.c (navigateInfo/title/directions,
//   nav_ws35_format_eta_time, nav_split_ve_huong),
//   hal/esp32/app_hal.cpp (navChanged/navIcChanged loop, getTime/weather).
// Target: no LVGL, no WiFi, no 48x48 icon, no Vietnamese TTF.
//   Static buffers, ASCII (diacritics stripped phone-side), e-paper
//   full refresh is seconds -> throttle repaints (see cnv_throttle).
//
// Wire (reuses vendor ...0002 char, opcodes disjoint from OTA A0/A2/A3/A4):
//   'T' + u8 hour + u8 min              time sync (CF_TIME equivalent)
//   'W' + i8 tempC + u8 icon            weather (icon 0=sun 1=cloud 2=rain 3=snow 4=fog)
//   'N' + dist0 + instr0                nav live update, two C-strings, total<=120B
//                                       (maps to Chronos title+directions; duration/
//                                       distance/eta collapsed phone-side to dist)
//   'C'                                nav clear (Chronos data[5]==0x00 equivalent)
// All parsers are pure + bounds-checked for host test.
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CNV_DIST_LEN 24
#define CNV_INSTR_LEN 64

typedef struct { uint8_t h, m; bool valid; } cnv_clock_t;
typedef struct { int8_t temp_c; uint8_t icon; bool valid; } cnv_weather_t;
typedef struct {
    bool active;
    char dist[CNV_DIST_LEN];
    char instr[CNV_INSTR_LEN];
} cnv_nav_t;

// Parse one BLE write payload. Returns true if opcode recognised + applied.
bool cnv_parse(const uint8_t *buf, size_t n,
               cnv_clock_t *clk, cnv_weather_t *wx, cnv_nav_t *nav);

// Render current state into epd_black/epd_red (logical order). Pure.
void cnv_render(const cnv_clock_t *clk, const cnv_weather_t *wx,
                const cnv_nav_t *nav);

// E-paper throttle: full update costs seconds + power. Repaint at most
// every CNV_MIN_MS unless force (nav start/clear). Pure.
#define CNV_MIN_MS 5000u
bool cnv_throttle(uint32_t now_ms, uint32_t last_ms, bool force);

// Test helper: ASCII-safe copy (non-printable -> '?', truncate, NUL-term).
void cnv_sanitize(char *dst, size_t dstn, const char *src, size_t srclen);
#endif
