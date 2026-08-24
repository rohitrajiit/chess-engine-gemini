#pragma once

#include "types.h"
#include "board.h"

namespace zenith {

int evaluate(const Board& board);

// Piece values for MVV-LVA and heuristics
constexpr int PIECE_VALUES[PIECE_TYPE_NB] = {
    100, // PAWN
    320, // KNIGHT
    330, // BISHOP
    500, // ROOK
    900, // QUEEN
    20000 // KING
};

} // namespace zenith
