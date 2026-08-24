#pragma once

#include "types.h"
#include "board.h"
#include <array>

namespace zenith {

struct MoveList {
    std::array<Move, 256> moves{};
    int count{0};

    void add(Move m) {
        moves[count++] = m;
    }

    int size() const { return count; }
    void clear() { count = 0; }

    Move operator[](int idx) const { return moves[idx]; }
    Move& operator[](int idx) { return moves[idx]; }

    Move* begin() { return moves.data(); }
    Move* end() { return moves.data() + count; }
    const Move* begin() const { return moves.data(); }
    const Move* end() const { return moves.data() + count; }
};

void generate_pseudo_legal_moves(const Board& board, MoveList& moves);
void generate_legal_moves(Board& board, MoveList& moves);
void generate_captures(const Board& board, MoveList& moves);

Move parse_move(Board& board, std::string_view uci_str);

} // namespace zenith
