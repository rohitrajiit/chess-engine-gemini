#include "search.h"
#include "evaluate.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>

namespace zenith {

Search g_search;

Search::Search() {
    clear_history();
}

void Search::clear_history() {
    for (int p = 0; p < MAX_PLY; ++p) {
        killers_[p][0] = MOVE_NONE;
        killers_[p][1] = MOVE_NONE;
    }
    for (int c = 0; c < COLOR_NB; ++c) {
        for (int from = 0; from < 64; ++from) {
            for (int to = 0; to < 64; ++to) {
                history_[c][from][to] = 0;
            }
        }
    }
}

void Search::stop() {
    info_.stop = true;
}

void Search::check_time() {
    if (info_.allocated_time_ms > 0) {
        auto now = std::chrono::steady_clock::now();
        int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - info_.start_time).count());
        if (elapsed >= info_.allocated_time_ms) {
            info_.stop = true;
        }
    }
}

int Search::score_move(const Board& board, Move m, Move tt_move, int ply) {
    if (m == tt_move) return 20000000;

    // Captures & Promotions
    if (m.type() == PROMOTION) {
        return 10000000 + PIECE_VALUES[m.promo_piece()];
    }

    if (m.type() == EN_PASSANT) {
        return 1000000 + (PIECE_VALUES[PAWN] * 10) - PIECE_VALUES[PAWN];
    }

    Piece captured = board.piece_at(m.to());
    if (captured != NO_PIECE) {
        PieceType victim = type_of(captured);
        PieceType attacker = type_of(board.piece_at(m.from()));
        return 1000000 + (PIECE_VALUES[victim] * 10) - PIECE_VALUES[attacker];
    }

    // Killer moves
    if (ply < MAX_PLY) {
        if (m == killers_[ply][0]) return 900000;
        if (m == killers_[ply][1]) return 800000;
    }

    // History heuristic
    int hist = history_[board.side_to_move()][m.from()][m.to()];
    return std::min(hist, 700000);
}

void Search::score_moves(const Board& board, MoveList& moves, Move tt_move, int ply) {
    for (int i = 0; i < moves.size(); ++i) {
        move_scores_[i] = score_move(board, moves[i], tt_move, ply);
    }
}

void Search::pick_move(MoveList& moves, int start_index) {
    int best_index = start_index;
    int best_score = move_scores_[start_index];

    for (int i = start_index + 1; i < moves.size(); ++i) {
        if (move_scores_[i] > best_score) {
            best_score = move_scores_[i];
            best_index = i;
        }
    }

    std::swap(moves[start_index], moves[best_index]);
    std::swap(move_scores_[start_index], move_scores_[best_index]);
}

int Search::quiescence(Board& board, int alpha, int beta, int ply) {
    if ((info_.nodes & 2047) == 0) {
        check_time();
    }
    if (info_.stop) return 0;

    ++info_.nodes;
    if (ply > info_.seldepth) {
        info_.seldepth = ply;
    }

    if (ply >= MAX_PLY - 1) {
        return evaluate(board);
    }

    int stand_pat = evaluate(board);
    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // Delta pruning
    constexpr int BIG_DELTA = 950; // Queen value
    if (stand_pat < alpha - BIG_DELTA) {
        return alpha;
    }

    MoveList captures;
    generate_captures(board, captures);
    score_moves(board, captures, MOVE_NONE, ply);

    for (int i = 0; i < captures.size(); ++i) {
        pick_move(captures, i);
        Move m = captures[i];

        if (!board.make_move(m)) continue;

        int score = -quiescence(board, -beta, -alpha, ply + 1);
        board.unmake_move(m);

        if (info_.stop) return 0;

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

int Search::negamax(Board& board, int depth, int alpha, int beta, int ply, bool is_pv, PVLine& pv) {
    pv.count = 0;

    if ((info_.nodes & 2047) == 0) {
        check_time();
    }
    if (info_.stop) return 0;

    // Draw detections
    if (ply > 0) {
        if (board.is_draw()) {
            return 0;
        }
    }

    if (ply >= MAX_PLY - 1) {
        return evaluate(board);
    }

    bool in_check = board.in_check();

    // Check extension
    if (in_check) {
        ++depth;
    }

    // Leaf node: enter Quiescence Search
    if (depth <= 0) {
        return quiescence(board, alpha, beta, ply);
    }

    ++info_.nodes;
    if (ply > info_.seldepth) {
        info_.seldepth = ply;
    }

    // Transposition Table Probe
    U64 key = board.zobrist_key();
    TTEntry tt_entry;
    Move tt_move = MOVE_NONE;
    if (g_tt.probe(key, tt_entry)) {
        tt_move = tt_entry.best_move;
        if (!is_pv && tt_entry.depth >= depth) {
            int tt_score = TranspositionTable::score_from_tt(tt_entry.score, ply);
            if (tt_entry.flag == TT_EXACT) {
                return tt_score;
            }
            if (tt_entry.flag == TT_LOWER_BOUND && tt_score >= beta) {
                return tt_score;
            }
            if (tt_entry.flag == TT_UPPER_BOUND && tt_score <= alpha) {
                return tt_score;
            }
        }
    }

    int static_eval = evaluate(board);

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (!is_pv && !in_check && depth <= 3 && static_eval - 120 * depth >= beta) {
        return static_eval;
    }

    // Null Move Pruning (NMP)
    if (!is_pv && !in_check && depth >= 3 && static_eval >= beta) {
        // Ensure side to move has non-pawn material
        Color us = board.side_to_move();
        U64 non_pawns = board.piece_bb(us, KNIGHT) | board.piece_bb(us, BISHOP) |
                        board.piece_bb(us, ROOK)   | board.piece_bb(us, QUEEN);
        if (non_pawns) {
            int R = 2 + depth / 6;
            board.make_null_move();
            PVLine null_pv;
            int null_score = -negamax(board, depth - 1 - R, -beta, -beta + 1, ply + 1, false, null_pv);
            board.unmake_null_move();

            if (info_.stop) return 0;

            if (null_score >= beta && std::abs(null_score) < SCORE_MATE_BOUND) {
                return beta;
            }
        }
    }

    // Move generation and scoring
    MoveList moves;
    generate_pseudo_legal_moves(board, moves);
    score_moves(board, moves, tt_move, ply);

    int legal_moves = 0;
    int best_score = -SCORE_INF;
    Move best_move = MOVE_NONE;
    TTFlag tt_flag = TT_UPPER_BOUND;
    PVLine child_pv;

    for (int i = 0; i < moves.size(); ++i) {
        pick_move(moves, i);
        Move m = moves[i];

        if (!board.make_move(m)) continue;
        ++legal_moves;

        int score = 0;
        // Principal Variation Search (PVS) with Late Move Reductions (LMR)
        if (legal_moves == 1) {
            // Search PV move with full window
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, is_pv, child_pv);
        } else {
            // Late Move Reductions
            int reduction = 0;
            if (depth >= 3 && legal_moves > 3 && !in_check && m.type() == NORMAL && board.piece_at(m.to()) == NO_PIECE) {
                reduction = 1 + (depth > 5 && legal_moves > 5 ? 1 : 0);
            }

            // Zero-window search
            score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, false, child_pv);

            // Re-search if reduced search fails high
            if (reduction > 0 && score > alpha) {
                score = -negamax(board, depth - 1, -alpha - 1, -alpha, ply + 1, false, child_pv);
            }

            // Full window re-search if needed
            if (score > alpha && score < beta) {
                score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true, child_pv);
            }
        }

        board.unmake_move(m);

        if (info_.stop) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = m;

            if (score > alpha) {
                alpha = score;
                tt_flag = TT_EXACT;

                // Update PV
                pv.moves[0] = m;
                for (int j = 0; j < child_pv.count && j < MAX_PLY - 1; ++j) {
                    pv.moves[j + 1] = child_pv.moves[j];
                }
                pv.count = child_pv.count + 1;

                if (score >= beta) {
                    tt_flag = TT_LOWER_BOUND;

                    // Update killers & history if quiet move
                    if (m.type() == NORMAL && board.piece_at(m.to()) == NO_PIECE) {
                        if (killers_[ply][0] != m) {
                            killers_[ply][1] = killers_[ply][0];
                            killers_[ply][0] = m;
                        }
                        history_[board.side_to_move()][m.from()][m.to()] += depth * depth;
                    }
                    break; // Beta cutoff
                }
            }
        }
    }

    // Checkmate or Stalemate
    if (legal_moves == 0) {
        if (in_check) {
            return -SCORE_MATE + ply; // Checkmate
        } else {
            return 0; // Stalemate
        }
    }

    // Store in Transposition Table
    if (!info_.stop) {
        g_tt.store(key, best_move, best_score, depth, tt_flag, ply);
    }

    return best_score;
}

void Search::prepare_search() {
    searching_ = true;
    info_.stop = false;
}

void Search::start(Board& board, const SearchLimits& limits) {
    if (info_.stop) {
        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);
        Move bm = (legal_moves.size() > 0) ? legal_moves[0] : MOVE_NONE;
        std::cout << "bestmove " << bm.to_uci() << std::endl;
        searching_ = false;
        return;
    }

    info_.nodes = 0;
    info_.seldepth = 0;
    info_.start_time = std::chrono::steady_clock::now();

    // Time allocation calculation
    Color us = board.side_to_move();
    if (limits.movetime > 0) {
        info_.allocated_time_ms = std::max(5, limits.movetime - 20);
    } else if (limits.time[us] > 0) {
        int time_left = limits.time[us];
        int inc = limits.inc[us];
        if (limits.movestogo > 0) {
            int mtg = std::min(limits.movestogo, 40);
            info_.allocated_time_ms = (time_left / mtg) + (inc * 3 / 4);
        } else {
            info_.allocated_time_ms = (time_left / 20) + (inc / 2);
        }
        info_.allocated_time_ms = std::max(10, std::min(info_.allocated_time_ms, time_left - 30));
    } else {
        info_.allocated_time_ms = 0; // Unlimited or depth-controlled
    }

    g_tt.new_search();

    Move best_move = MOVE_NONE;
    Move prev_best_move = MOVE_NONE;
    PVLine root_pv;

    int max_search_depth = (limits.depth > 0) ? limits.depth : 64;

    for (int depth = 1; depth <= max_search_depth; ++depth) {
        PVLine cur_pv;
        int score = negamax(board, depth, -SCORE_INF, SCORE_INF, 0, true, cur_pv);

        if (info_.stop && depth > 1) {
            break;
        }

        if (cur_pv.count > 0) {
            best_move = cur_pv.moves[0];
            prev_best_move = best_move;
            root_pv = cur_pv;
        }

        auto now = std::chrono::steady_clock::now();
        int elapsed_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - info_.start_time).count());
        uint64_t nps = (elapsed_ms > 0) ? (info_.nodes * 1000 / elapsed_ms) : info_.nodes * 1000;

        // UCI info output
        std::cout << "info depth " << depth
                  << " seldepth " << info_.seldepth;

        if (std::abs(score) > SCORE_MATE_BOUND) {
            int mate_moves = (score > 0) ? ((SCORE_MATE - score + 1) / 2) : ((-SCORE_MATE - score) / 2);
            std::cout << " score mate " << mate_moves;
        } else {
            std::cout << " score cp " << score;
        }

        std::cout << " nodes " << info_.nodes
                  << " nps " << nps
                  << " time " << elapsed_ms
                  << " pv";

        for (int i = 0; i < root_pv.count; ++i) {
            std::cout << " " << root_pv.moves[i].to_uci();
        }
        std::cout << std::endl;

        // Stop if mate found or time limit exceeded
        if (std::abs(score) > SCORE_MATE_BOUND && depth >= 6) {
            break;
        }

        if (info_.allocated_time_ms > 0 && elapsed_ms >= info_.allocated_time_ms / 2) {
            break;
        }
    }

    if (best_move.is_none()) {
        if (!prev_best_move.is_none()) {
            best_move = prev_best_move;
        } else {
            MoveList legal_moves;
            generate_legal_moves(board, legal_moves);
            if (legal_moves.size() > 0) {
                best_move = legal_moves[0];
            }
        }
    }

    std::cout << "bestmove " << best_move.to_uci() << std::endl;
    searching_ = false;
}

} // namespace zenith
