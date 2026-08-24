#pragma once

#include "types.h"
#include <vector>

namespace zenith {

enum TTFlag : uint8_t {
    TT_NONE        = 0,
    TT_EXACT       = 1,
    TT_LOWER_BOUND = 2, // Beta cutoff (fail-high)
    TT_UPPER_BOUND = 3  // Alpha (fail-low)
};

struct TTEntry {
    U64 key{0ULL};
    Move best_move{MOVE_NONE};
    int16_t score{0};
    int8_t depth{0};
    uint8_t flag{TT_NONE};
    uint8_t age{0};
};

class TranspositionTable {
public:
    TranspositionTable() = default;
    explicit TranspositionTable(size_t size_mb) {
        resize(size_mb);
    }

    void resize(size_t size_mb);
    void clear();
    void new_search() { ++current_age_; }

    bool probe(U64 key, TTEntry& entry) const;
    void store(U64 key, Move best_move, int score, int depth, TTFlag flag, int ply);

    static int score_to_tt(int score, int ply) {
        if (score > SCORE_MATE_BOUND) return score + ply;
        if (score < -SCORE_MATE_BOUND) return score - ply;
        return score;
    }

    static int score_from_tt(int score, int ply) {
        if (score > SCORE_MATE_BOUND) return score - ply;
        if (score < -SCORE_MATE_BOUND) return score + ply;
        return score;
    }

    size_t size() const { return table_.size(); }

private:
    std::vector<TTEntry> table_;
    uint8_t current_age_{0};
};

extern TranspositionTable g_tt;

} // namespace zenith
