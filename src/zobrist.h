#pragma once

#include "types.h"

namespace zenith {
namespace zobrist {

extern U64 PIECE_KEYS[PIECE_NB][64];
extern U64 CASTLING_KEYS[16];
extern U64 EP_KEYS[64];
extern U64 SIDE_KEY;

void init_zobrist();

} // namespace zobrist
} // namespace zenith
