#ifndef FACTORY_RESET_H_
#define FACTORY_RESET_H_
// 0x2F factory reset — the binding fallback contract (docs/FACTORY_RESET.md).
//
// Consequence 1 (no activation gate): this API takes NO auth state. Reset
// works unconditionally; a lock that reset must preserve is a fallback
// weakened. There is deliberately no parameter for it.
//
// Consequence 2 (settings/reset co-evolve): every mutable record must appear
// in the covered table; hardware identity in the exempt table. The tables are
// the single place to extend, and fr_disjoint() lets tests fail a build that
// lets the wipe reach identity.
#include <stdint.h>

typedef struct { uint32_t addr; uint32_t len; } fr_span_t;

// Wire guard: opcode 0x2F + magic 'R','S','T',0x5A. Anything else: ignore.
#define FR_OP      0x2F
#define FR_MAGIC0  0x52
#define FR_MAGIC1  0x53
#define FR_MAGIC2  0x54
#define FR_MAGIC3  0x5A

// Wipe covered spans (erase semantics: fill 0xFF), bounds-checked.
// Returns 1 = wiped, reboot now; 0 = packet ignored, flash untouched.
int factory_reset_apply(uint8_t *flash, uint32_t flash_size,
                        const fr_span_t *covered, int ncovered,
                        const uint8_t *pkt, int pktlen);

// True iff no covered span overlaps any exempt span. Run it over the shipped
// tables in every test build: overlap = reset can kill identity = brick path.
int fr_disjoint(const fr_span_t *covered, int ncovered,
                const fr_span_t *exempt, int nexempt);

// Reference tables. Covered mirrors the HMCLOCK-EN settings region
// (0x3B000 image/mode/data); vendor config/timetable/slot offsets are
// VENDOR-TBD and MUST be added here (same commit as the record) once the
// dump confirms them. Exempt is non-negotiable: banks travel with the
// product header, identity records are fixed.
// Tables are sized HERE, in the header, on purpose: adding a record without
// growing FR_NCOVERED_REF (same commit, per docs/FACTORY_RESET.md) breaks the
// _Static_assert in factory_reset.c. The wipe list cannot silently lag.
#define FR_NCOVERED_REF 1
#define FR_NEXEMPT_FIXED 3
extern const fr_span_t fr_covered_ref[FR_NCOVERED_REF];
extern const fr_span_t fr_exempt_fixed[FR_NEXEMPT_FIXED];
#endif
