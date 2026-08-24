#pragma once

#include "types.h"
#include "board.h"
#include "movegen.h"
#include "tt.h"
#include <chrono>
#include <atomic>
#include <vector>

namespace zenith {

struct SearchLimits {
    int depth{64};
    uint64_t nodes{0};
    int time[COLOR_NB]{0, 0};
    int inc[COLOR_NB]{0, 0};
    int movestogo{0};
    int movetime{0};
    bool infinite{false};
};

struct SearchInfo {
    std::chrono::steady_clock::time_point start_time;
    int allocated_time_ms{0};
    uint64_t nodes{0};
    int seldepth{0};
    std::atomic<bool> stop{false};
};

struct PVLine {
    int count{0};
    std::array<Move, MAX_PLY> moves{};
};

class Search {
public:
    Search();

    void prepare_search();
    void start(Board& board, const SearchLimits& limits);
    void stop();
    bool is_searching() const { return searching_; }

    void clear_history();

private:
    int negamax(Board& board, int depth, int alpha, int beta, int ply, bool is_pv, PVLine& pv);
    int quiescence(Board& board, int alpha, int beta, int ply);

    void score_moves(const Board& board, MoveList& moves, Move tt_move, int ply);
    void pick_move(MoveList& moves, int start_index);
    int score_move(const Board& board, Move m, Move tt_move, int ply);

    void check_time();

    SearchInfo info_;
    std::atomic<bool> searching_{false};

    // Move ordering tables
    std::array<std::array<Move, 2>, MAX_PLY> killers_{};
    std::array<std::array<std::array<int, 64>, 64>, COLOR_NB> history_{};
    std::array<int, 256> move_scores_{};
};

extern Search g_search;

} // namespace zenith
