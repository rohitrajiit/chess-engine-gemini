#include "movegen.h"

namespace zenith {

void generate_pseudo_legal_moves(const Board& board, MoveList& moves) {
    Color us = board.side_to_move();
    Color them = ~us;
    U64 enemy = board.color_bb(them);
    U64 valid_targets = ~board.color_bb(us);

    // 1. Pawn moves
    U64 pawns = board.piece_bb(us, PAWN);
    while (pawns) {
        Square from = pop_lsb(pawns);
        Rank r = rank_of(from);

        if (us == WHITE) {
            // Single push
            Square to = static_cast<Square>(from + 8);
            if (board.piece_at(to) == NO_PIECE) {
                if (r == RANK_7) {
                    moves.add(Move(from, to, PROMOTION, QUEEN));
                    moves.add(Move(from, to, PROMOTION, ROOK));
                    moves.add(Move(from, to, PROMOTION, BISHOP));
                    moves.add(Move(from, to, PROMOTION, KNIGHT));
                } else {
                    moves.add(Move(from, to));
                    // Double push
                    if (r == RANK_2) {
                        Square to2 = static_cast<Square>(from + 16);
                        if (board.piece_at(to2) == NO_PIECE) {
                            moves.add(Move(from, to2));
                        }
                    }
                }
            }

            // Captures
            U64 attacks = pawn_attacks_bb(WHITE, from) & enemy;
            while (attacks) {
                Square cap_to = pop_lsb(attacks);
                if (r == RANK_7) {
                    moves.add(Move(from, cap_to, PROMOTION, QUEEN));
                    moves.add(Move(from, cap_to, PROMOTION, ROOK));
                    moves.add(Move(from, cap_to, PROMOTION, BISHOP));
                    moves.add(Move(from, cap_to, PROMOTION, KNIGHT));
                } else {
                    moves.add(Move(from, cap_to));
                }
            }
        } else {
            // Black pawn single push
            Square to = static_cast<Square>(from - 8);
            if (board.piece_at(to) == NO_PIECE) {
                if (r == RANK_2) {
                    moves.add(Move(from, to, PROMOTION, QUEEN));
                    moves.add(Move(from, to, PROMOTION, ROOK));
                    moves.add(Move(from, to, PROMOTION, BISHOP));
                    moves.add(Move(from, to, PROMOTION, KNIGHT));
                } else {
                    moves.add(Move(from, to));
                    // Double push
                    if (r == RANK_7) {
                        Square to2 = static_cast<Square>(from - 16);
                        if (board.piece_at(to2) == NO_PIECE) {
                            moves.add(Move(from, to2));
                        }
                    }
                }
            }

            // Captures
            U64 attacks = pawn_attacks_bb(BLACK, from) & enemy;
            while (attacks) {
                Square cap_to = pop_lsb(attacks);
                if (r == RANK_2) {
                    moves.add(Move(from, cap_to, PROMOTION, QUEEN));
                    moves.add(Move(from, cap_to, PROMOTION, ROOK));
                    moves.add(Move(from, cap_to, PROMOTION, BISHOP));
                    moves.add(Move(from, cap_to, PROMOTION, KNIGHT));
                } else {
                    moves.add(Move(from, cap_to));
                }
            }
        }

        // En passant
        if (board.ep_square() != SQ_NONE) {
            Square ep_sq = board.ep_square();
            if (pawn_attacks_bb(us, from) & square_bb(ep_sq)) {
                moves.add(Move(from, ep_sq, EN_PASSANT));
            }
        }
    }

    // 2. Knight moves
    U64 knights = board.piece_bb(us, KNIGHT);
    while (knights) {
        Square from = pop_lsb(knights);
        U64 attacks = knight_attacks_bb(from) & valid_targets;
        while (attacks) {
            Square to = pop_lsb(attacks);
            moves.add(Move(from, to));
        }
    }

    // 3. Bishop moves
    U64 bishops = board.piece_bb(us, BISHOP);
    while (bishops) {
        Square from = pop_lsb(bishops);
        U64 attacks = bishop_attacks_bb(from, board.occupied_bb()) & valid_targets;
        while (attacks) {
            Square to = pop_lsb(attacks);
            moves.add(Move(from, to));
        }
    }

    // 4. Rook moves
    U64 rooks = board.piece_bb(us, ROOK);
    while (rooks) {
        Square from = pop_lsb(rooks);
        U64 attacks = rook_attacks_bb(from, board.occupied_bb()) & valid_targets;
        while (attacks) {
            Square to = pop_lsb(attacks);
            moves.add(Move(from, to));
        }
    }

    // 5. Queen moves
    U64 queens = board.piece_bb(us, QUEEN);
    while (queens) {
        Square from = pop_lsb(queens);
        U64 attacks = queen_attacks_bb(from, board.occupied_bb()) & valid_targets;
        while (attacks) {
            Square to = pop_lsb(attacks);
            moves.add(Move(from, to));
        }
    }

    // 6. King moves
    Square ksq = board.king_square(us);
    if (ksq != SQ_NONE) {
        U64 attacks = king_attacks_bb(ksq) & valid_targets;
        while (attacks) {
            Square to = pop_lsb(attacks);
            moves.add(Move(ksq, to));
        }

        // Castling (king must not be in check)
        if (!board.in_check(us)) {
            uint8_t cr = board.castling_rights();
            if (us == WHITE) {
                if ((cr & WHITE_OO) &&
                    board.piece_at(SQ_F1) == NO_PIECE &&
                    board.piece_at(SQ_G1) == NO_PIECE &&
                    !board.is_square_attacked(SQ_F1, BLACK) &&
                    !board.is_square_attacked(SQ_G1, BLACK)) {
                    moves.add(Move(SQ_E1, SQ_G1, CASTLING));
                }
                if ((cr & WHITE_OOO) &&
                    board.piece_at(SQ_D1) == NO_PIECE &&
                    board.piece_at(SQ_C1) == NO_PIECE &&
                    board.piece_at(SQ_B1) == NO_PIECE &&
                    !board.is_square_attacked(SQ_D1, BLACK) &&
                    !board.is_square_attacked(SQ_C1, BLACK)) {
                    moves.add(Move(SQ_E1, SQ_C1, CASTLING));
                }
            } else {
                if ((cr & BLACK_OO) &&
                    board.piece_at(SQ_F8) == NO_PIECE &&
                    board.piece_at(SQ_G8) == NO_PIECE &&
                    !board.is_square_attacked(SQ_F8, WHITE) &&
                    !board.is_square_attacked(SQ_G8, WHITE)) {
                    moves.add(Move(SQ_E8, SQ_G8, CASTLING));
                }
                if ((cr & BLACK_OOO) &&
                    board.piece_at(SQ_D8) == NO_PIECE &&
                    board.piece_at(SQ_C8) == NO_PIECE &&
                    board.piece_at(SQ_B8) == NO_PIECE &&
                    !board.is_square_attacked(SQ_D8, WHITE) &&
                    !board.is_square_attacked(SQ_C8, WHITE)) {
                    moves.add(Move(SQ_E8, SQ_C8, CASTLING));
                }
            }
        }
    }
}

void generate_legal_moves(Board& board, MoveList& moves) {
    MoveList pseudo_moves;
    generate_pseudo_legal_moves(board, pseudo_moves);

    for (int i = 0; i < pseudo_moves.size(); ++i) {
        Move m = pseudo_moves[i];
        if (board.make_move(m)) {
            moves.add(m);
            board.unmake_move(m);
        }
    }
}

void generate_captures(const Board& board, MoveList& moves) {
    Color us = board.side_to_move();
    Color them = ~us;
    U64 enemy = board.color_bb(them);

    // 1. Pawn captures & promotions
    U64 pawns = board.piece_bb(us, PAWN);
    while (pawns) {
        Square from = pop_lsb(pawns);
        Rank r = rank_of(from);

        if (us == WHITE) {
            // Promo pushes
            if (r == RANK_7) {
                Square to = static_cast<Square>(from + 8);
                if (board.piece_at(to) == NO_PIECE) {
                    moves.add(Move(from, to, PROMOTION, QUEEN));
                    moves.add(Move(from, to, PROMOTION, ROOK));
                    moves.add(Move(from, to, PROMOTION, BISHOP));
                    moves.add(Move(from, to, PROMOTION, KNIGHT));
                }
            }
            // Captures
            U64 attacks = pawn_attacks_bb(WHITE, from) & enemy;
            while (attacks) {
                Square cap_to = pop_lsb(attacks);
                if (r == RANK_7) {
                    moves.add(Move(from, cap_to, PROMOTION, QUEEN));
                    moves.add(Move(from, cap_to, PROMOTION, ROOK));
                    moves.add(Move(from, cap_to, PROMOTION, BISHOP));
                    moves.add(Move(from, cap_to, PROMOTION, KNIGHT));
                } else {
                    moves.add(Move(from, cap_to));
                }
            }
        } else {
            // Black promo pushes
            if (r == RANK_2) {
                Square to = static_cast<Square>(from - 8);
                if (board.piece_at(to) == NO_PIECE) {
                    moves.add(Move(from, to, PROMOTION, QUEEN));
                    moves.add(Move(from, to, PROMOTION, ROOK));
                    moves.add(Move(from, to, PROMOTION, BISHOP));
                    moves.add(Move(from, to, PROMOTION, KNIGHT));
                }
            }
            // Captures
            U64 attacks = pawn_attacks_bb(BLACK, from) & enemy;
            while (attacks) {
                Square cap_to = pop_lsb(attacks);
                if (r == RANK_2) {
                    moves.add(Move(from, cap_to, PROMOTION, QUEEN));
                    moves.add(Move(from, cap_to, PROMOTION, ROOK));
                    moves.add(Move(from, cap_to, PROMOTION, BISHOP));
                    moves.add(Move(from, cap_to, PROMOTION, KNIGHT));
                } else {
                    moves.add(Move(from, cap_to));
                }
            }
        }

        // En passant
        if (board.ep_square() != SQ_NONE) {
            Square ep_sq = board.ep_square();
            if (pawn_attacks_bb(us, from) & square_bb(ep_sq)) {
                moves.add(Move(from, ep_sq, EN_PASSANT));
            }
        }
    }

    // 2. Knight captures
    U64 knights = board.piece_bb(us, KNIGHT);
    while (knights) {
        Square from = pop_lsb(knights);
        U64 attacks = knight_attacks_bb(from) & enemy;
        while (attacks) {
            moves.add(Move(from, pop_lsb(attacks)));
        }
    }

    // 3. Bishop captures
    U64 bishops = board.piece_bb(us, BISHOP);
    while (bishops) {
        Square from = pop_lsb(bishops);
        U64 attacks = bishop_attacks_bb(from, board.occupied_bb()) & enemy;
        while (attacks) {
            moves.add(Move(from, pop_lsb(attacks)));
        }
    }

    // 4. Rook captures
    U64 rooks = board.piece_bb(us, ROOK);
    while (rooks) {
        Square from = pop_lsb(rooks);
        U64 attacks = rook_attacks_bb(from, board.occupied_bb()) & enemy;
        while (attacks) {
            moves.add(Move(from, pop_lsb(attacks)));
        }
    }

    // 5. Queen captures
    U64 queens = board.piece_bb(us, QUEEN);
    while (queens) {
        Square from = pop_lsb(queens);
        U64 attacks = queen_attacks_bb(from, board.occupied_bb()) & enemy;
        while (attacks) {
            moves.add(Move(from, pop_lsb(attacks)));
        }
    }

    // 6. King captures
    Square ksq = board.king_square(us);
    if (ksq != SQ_NONE) {
        U64 attacks = king_attacks_bb(ksq) & enemy;
        while (attacks) {
            moves.add(Move(ksq, pop_lsb(attacks)));
        }
    }
}

Move parse_move(Board& board, std::string_view uci_str) {
    if (uci_str.length() < 4) return MOVE_NONE;

    Square from = str_to_square(uci_str.substr(0, 2));
    Square to = str_to_square(uci_str.substr(2, 2));
    if (from == SQ_NONE || to == SQ_NONE) return MOVE_NONE;

    PieceType promo = NO_PIECE_TYPE;
    if (uci_str.length() >= 5) {
        switch (uci_str[4]) {
            case 'q': promo = QUEEN; break;
            case 'r': promo = ROOK; break;
            case 'b': promo = BISHOP; break;
            case 'n': promo = KNIGHT; break;
            default: break;
        }
    }

    MoveList legal_moves;
    generate_legal_moves(board, legal_moves);

    for (int i = 0; i < legal_moves.size(); ++i) {
        Move m = legal_moves[i];
        if (m.from() == from && m.to() == to) {
            if (m.type() == PROMOTION) {
                if (m.promo_piece() == promo) return m;
            } else {
                return m;
            }
        }
    }

    return MOVE_NONE;
}

} // namespace zenith
