#include "board.h"
bool board_decode(const uint8_t *rec, board_t *out) {
    out->panel = rec[0];
    out->have_pinmap = (rec[1] == 0x01);
    for (int i = 0; i < 8; i++) {
        uint8_t v = rec[8 + i];
        out->port[i] = (uint8_t)(v >> 4);
        out->pin[i] = (uint8_t)(v & 0x0F);
    }
    // Erased flash (all FF) = built-in map, valid state, not an error.
    return out->have_pinmap;
}
