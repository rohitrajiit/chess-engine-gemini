#include "zobrist.h"

namespace zenith {
namespace zobrist {

U64 PIECE_KEYS[PIECE_NB][64];
U64 CASTLING_KEYS[16];
U64 EP_KEYS[64];
U64 SIDE_KEY;

// Deterministic PRNG for reproducible Zobrist keys
static uint64_t zobrist_prng_state = 1070372ULL;

static uint64_t rand64() {
    uint64_t x = zobrist_prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    zobrist_prng_state = x;
    return x;
}

void init_zobrist() {
    for (int p = 0; p < PIECE_NB; ++p) {
        for (int sq = 0; sq < 64; ++sq) {
            PIECE_KEYS[p][sq] = rand64();
        }
    }

    for (int i = 0; i < 16; ++i) {
        CASTLING_KEYS[i] = rand64();
    }

    for (int sq = 0; sq < 64; ++sq) {
        EP_KEYS[sq] = rand64();
    }

    SIDE_KEY = rand64();
}

} // namespace zobrist
} // namespace zenith
