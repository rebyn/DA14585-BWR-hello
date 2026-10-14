#include "factory_reset.h"
#include <string.h>

const fr_span_t fr_covered_ref[FR_NCOVERED_REF] = {
    { 0x3B000, 0x4000 },   // HMCLOCK-EN settings region: IMG1 hdr + mode + pixels
    // VENDOR-TBD: config / timetable / image-slot sectors go here, same
    // commit as the record that defines them. See docs/FACTORY_RESET.md.
};

const fr_span_t fr_exempt_fixed[FR_NEXEMPT_FIXED] = {
    { 0x38000, 0x1000 },   // product header sector
    { 0x39000, 0x1000 },   // board record (panel/pins) sector
    { 0x3A000, 0x1000 },   // panel resolution record sector
    // NOTE: both firmware bank slots are exempt too, but their addresses
    // come from the product header at runtime — the caller passes them.
    // A wipe table that names a bank address is a defect, not a feature.
};
_Static_assert(sizeof(fr_covered_ref) / sizeof(fr_covered_ref[0]) == FR_NCOVERED_REF,
               "grow FR_NCOVERED_REF in factory_reset.h with the new record");
_Static_assert(sizeof(fr_exempt_fixed) / sizeof(fr_exempt_fixed[0]) == FR_NEXEMPT_FIXED,
               "grow FR_NEXEMPT_FIXED in factory_reset.h with the new record");

int fr_disjoint(const fr_span_t *covered, int ncovered,
                const fr_span_t *exempt, int nexempt) {
    for (int i = 0; i < ncovered; i++) {
        uint32_t a0 = covered[i].addr, a1 = a0 + covered[i].len;
        for (int j = 0; j < nexempt; j++) {
            uint32_t b0 = exempt[j].addr, b1 = b0 + exempt[j].len;
            if (a0 < b1 && b0 < a1) return 0;
        }
    }
    return 1;
}

int factory_reset_apply(uint8_t *flash, uint32_t flash_size,
                        const fr_span_t *covered, int ncovered,
                        const uint8_t *pkt, int pktlen) {
    if (pktlen < 5 || pkt[0] != FR_OP ||
        pkt[1] != FR_MAGIC0 || pkt[2] != FR_MAGIC1 ||
        pkt[3] != FR_MAGIC2 || pkt[4] != FR_MAGIC3)
        return 0;   // stray write: touch nothing
    for (int i = 0; i < ncovered; i++) {
        uint32_t a = covered[i].addr, n = covered[i].len;
        if (a >= flash_size) continue;
        if (a + n > flash_size) n = flash_size - a;
        memset(flash + a, 0xFF, n);
    }
    return 1;
}
