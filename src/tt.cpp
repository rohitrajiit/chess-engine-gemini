#include "tt.h"
#include <cstring>

namespace zenith {

TranspositionTable g_tt(64); // Default 64 MB

void TranspositionTable::resize(size_t size_mb) {
    size_t num_entries = (size_mb * 1024 * 1024) / sizeof(TTEntry);
    table_.clear();
    table_.resize(num_entries);
    clear();
}

void TranspositionTable::clear() {
    std::fill(table_.begin(), table_.end(), TTEntry{});
}

bool TranspositionTable::probe(U64 key, TTEntry& entry) const {
    if (table_.empty()) return false;
    size_t index = key % table_.size();
    const TTEntry& target = table_[index];

    if (target.key == key && target.flag != TT_NONE) {
        entry = target;
        return true;
    }
    return false;
}

void TranspositionTable::store(U64 key, Move best_move, int score, int depth, TTFlag flag, int ply) {
    if (table_.empty()) return;
    size_t index = key % table_.size();
    TTEntry& target = table_[index];

    // Replacement condition:
    // 1. Empty slot or same position
    // 2. Older search entry
    // 3. Deeper search depth
    bool replace = (target.key == 0ULL) ||
                   (target.key == key) ||
                   (target.age != current_age_) ||
                   (depth >= target.depth);

    if (replace) {
        target.key = key;
        // Keep previous best move if new move is NONE
        if (!best_move.is_none() || target.key != key) {
            target.best_move = best_move;
        }
        target.score = static_cast<int16_t>(score_to_tt(score, ply));
        target.depth = static_cast<int8_t>(depth);
        target.flag = flag;
        target.age = current_age_;
    }
}

} // namespace zenith
