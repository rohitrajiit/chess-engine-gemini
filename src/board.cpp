#include "board.h"
#include <sstream>
#include <iostream>
#include <cctype>

namespace zenith {

Board::Board() {
    reset();
}

Board::Board(const std::string& fen) {
    set_fen(fen);
}

void Board::clear() {
    pieces_.fill(0ULL);
    occupancies_.fill(0ULL);
    piece_on_.fill(NO_PIECE);

    side_to_move_ = WHITE;
    ep_sq_ = SQ_NONE;
    castling_rights_ = NO_CASTLING;
    halfmove_clock_ = 0;
    fullmove_number_ = 1;
    zobrist_key_ = 0ULL;
    history_ply_ = 0;
}

void Board::reset() {
    set_fen(START_FEN);
}

void Board::put_piece(Piece p, Square sq) {
    U64 mask = square_bb(sq);
    pieces_[p] |= mask;
    Color c = color_of(p);
    occupancies_[c] |= mask;
    occupancies_[2] |= mask;
    piece_on_[sq] = p;
    zobrist_key_ ^= zobrist::PIECE_KEYS[p][sq];
}

void Board::remove_piece(Square sq) {
    Piece p = piece_on_[sq];
    if (p == NO_PIECE) return;
    U64 mask = ~square_bb(sq);
    pieces_[p] &= mask;
    Color c = color_of(p);
    occupancies_[c] &= mask;
    occupancies_[2] &= mask;
    piece_on_[sq] = NO_PIECE;
    zobrist_key_ ^= zobrist::PIECE_KEYS[p][sq];
}

void Board::move_piece(Piece p, Square from, Square to) {
    U64 move_mask = square_bb(from) | square_bb(to);
    pieces_[p] ^= move_mask;
    Color c = color_of(p);
    occupancies_[c] ^= move_mask;
    occupancies_[2] ^= move_mask;
    piece_on_[from] = NO_PIECE;
    piece_on_[to] = p;
    zobrist_key_ ^= zobrist::PIECE_KEYS[p][from] ^ zobrist::PIECE_KEYS[p][to];
}

U64 Board::compute_zobrist_key() const {
    U64 key = 0ULL;

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = piece_on_[sq];
        if (p != NO_PIECE) {
            key ^= zobrist::PIECE_KEYS[p][sq];
        }
    }

    if (ep_sq_ != SQ_NONE) {
        key ^= zobrist::EP_KEYS[ep_sq_];
    }

    key ^= zobrist::CASTLING_KEYS[castling_rights_];

    if (side_to_move_ == BLACK) {
        key ^= zobrist::SIDE_KEY;
    }

    return key;
}

bool Board::set_fen(const std::string& fen) {
    clear();
    std::istringstream ss(fen);
    std::string piece_placement, side, castling, ep_str;
    int halfmove = 0, fullmove = 1;

    if (!(ss >> piece_placement >> side >> castling >> ep_str)) {
        return false;
    }
    ss >> halfmove >> fullmove;

    // 1. Piece placement
    int r = 7;
    int f = 0;
    for (char c : piece_placement) {
        if (c == '/') {
            --r;
            f = 0;
        } else if (std::isdigit(c)) {
            f += (c - '0');
        } else {
            Piece p = char_to_piece(c);
            if (p == NO_PIECE || r < 0 || f > 7) return false;
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            put_piece(p, sq);
            ++f;
        }
    }

    // 2. Side to move
    side_to_move_ = (side == "b" || side == "B") ? BLACK : WHITE;

    // 3. Castling rights
    castling_rights_ = NO_CASTLING;
    for (char c : castling) {
        if (c == 'K') castling_rights_ |= WHITE_OO;
        else if (c == 'Q') castling_rights_ |= WHITE_OOO;
        else if (c == 'k') castling_rights_ |= BLACK_OO;
        else if (c == 'q') castling_rights_ |= BLACK_OOO;
    }

    // 4. En passant
    if (ep_str != "-") {
        ep_sq_ = str_to_square(ep_str);
    } else {
        ep_sq_ = SQ_NONE;
    }

    // 5. Clocks
    halfmove_clock_ = halfmove;
    fullmove_number_ = fullmove;

    // Recompute key
    zobrist_key_ = compute_zobrist_key();
    return true;
}

std::string Board::get_fen() const {
    std::ostringstream ss;

    // 1. Pieces
    for (int r = 7; r >= 0; --r) {
        int empty_count = 0;
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Piece p = piece_on_[sq];
            if (p == NO_PIECE) {
                ++empty_count;
            } else {
                if (empty_count > 0) {
                    ss << empty_count;
                    empty_count = 0;
                }
                ss << piece_to_char(p);
            }
        }
        if (empty_count > 0) {
            ss << empty_count;
        }
        if (r > 0) ss << '/';
    }

    // 2. Side
    ss << ' ' << (side_to_move_ == WHITE ? 'w' : 'b');

    // 3. Castling
    ss << ' ';
    if (castling_rights_ == NO_CASTLING) {
        ss << '-';
    } else {
        if (castling_rights_ & WHITE_OO) ss << 'K';
        if (castling_rights_ & WHITE_OOO) ss << 'Q';
        if (castling_rights_ & BLACK_OO) ss << 'k';
        if (castling_rights_ & BLACK_OOO) ss << 'q';
    }

    // 4. En passant
    ss << ' ' << square_to_str(ep_sq_);

    // 5. Clocks
    ss << ' ' << halfmove_clock_ << ' ' << fullmove_number_;

    return ss.str();
}

void Board::print() const {
    std::cout << "\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << "  ";
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Piece p = piece_on_[sq];
            std::cout << piece_to_char(p) << ' ';
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n\n";
    std::cout << "Side to move: " << (side_to_move_ == WHITE ? "white" : "black") << "\n";
    std::cout << "Castling: ";
    if (castling_rights_ & WHITE_OO) std::cout << "K";
    if (castling_rights_ & WHITE_OOO) std::cout << "Q";
    if (castling_rights_ & BLACK_OO) std::cout << "k";
    if (castling_rights_ & BLACK_OOO) std::cout << "q";
    if (castling_rights_ == NO_CASTLING) std::cout << "-";
    std::cout << "\n";
    std::cout << "En passant: " << square_to_str(ep_sq_) << "\n";
    std::cout << "Halfmove clock: " << halfmove_clock_ << "\n";
    std::cout << "Fullmove: " << fullmove_number_ << "\n";
    std::cout << "FEN: " << get_fen() << "\n";
    std::cout << "Zobrist Key: 0x" << std::hex << zobrist_key_ << std::dec << "\n\n";
}

bool Board::is_square_attacked(Square sq, Color attacking_color) const {
    // Pawn attacks
    if (pawn_attacks_bb(~attacking_color, sq) & pieces_[make_piece(attacking_color, PAWN)]) {
        return true;
    }

    // Knight attacks
    if (knight_attacks_bb(sq) & pieces_[make_piece(attacking_color, KNIGHT)]) {
        return true;
    }

    // King attacks
    if (king_attacks_bb(sq) & pieces_[make_piece(attacking_color, KING)]) {
        return true;
    }

    // Bishop / Queen diagonal attacks
    U64 bishop_queen = pieces_[make_piece(attacking_color, BISHOP)] | pieces_[make_piece(attacking_color, QUEEN)];
    if (bishop_queen && (bishop_attacks_bb(sq, occupancies_[2]) & bishop_queen)) {
        return true;
    }

    // Rook / Queen straight attacks
    U64 rook_queen = pieces_[make_piece(attacking_color, ROOK)] | pieces_[make_piece(attacking_color, QUEEN)];
    if (rook_queen && (rook_attacks_bb(sq, occupancies_[2]) & rook_queen)) {
        return true;
    }

    return false;
}

bool Board::make_move(Move m) {
    Square from = m.from();
    Square to = m.to();
    MoveType type = m.type();
    Piece moving_piece = piece_on_[from];
    Color us = side_to_move_;
    Color them = ~us;

    // Save history
    GameState& state = history_[history_ply_++];
    state.ep_sq = ep_sq_;
    state.castling_rights = castling_rights_;
    state.halfmove_clock = halfmove_clock_;
    state.captured_piece = piece_on_[to];
    state.zobrist_key = zobrist_key_;

    // Reset EP square in hash if previously present
    if (ep_sq_ != SQ_NONE) {
        zobrist_key_ ^= zobrist::EP_KEYS[ep_sq_];
        ep_sq_ = SQ_NONE;
    }

    // Update 50-move halfmove clock
    if (type_of(moving_piece) == PAWN || state.captured_piece != NO_PIECE) {
        halfmove_clock_ = 0;
    } else {
        ++halfmove_clock_;
    }

    switch (type) {
        case NORMAL: {
            if (state.captured_piece != NO_PIECE) {
                remove_piece(to);
            }
            move_piece(moving_piece, from, to);

            // Double pawn push - create EP square
            if (type_of(moving_piece) == PAWN) {
                int diff = static_cast<int>(to) - static_cast<int>(from);
                if (us == WHITE && diff == 16) {
                    ep_sq_ = static_cast<Square>(from + 8);
                    zobrist_key_ ^= zobrist::EP_KEYS[ep_sq_];
                } else if (us == BLACK && diff == -16) {
                    ep_sq_ = static_cast<Square>(from - 8);
                    zobrist_key_ ^= zobrist::EP_KEYS[ep_sq_];
                }
            }
            break;
        }

        case CASTLING: {
            move_piece(moving_piece, from, to);
            if (to == SQ_G1) {
                move_piece(W_ROOK, SQ_H1, SQ_F1);
            } else if (to == SQ_C1) {
                move_piece(W_ROOK, SQ_A1, SQ_D1);
            } else if (to == SQ_G8) {
                move_piece(B_ROOK, SQ_H8, SQ_F8);
            } else if (to == SQ_C8) {
                move_piece(B_ROOK, SQ_A8, SQ_D8);
            }
            break;
        }

        case EN_PASSANT: {
            move_piece(moving_piece, from, to);
            Square cap_sq = make_square(file_of(to), rank_of(from));
            state.captured_piece = piece_on_[cap_sq];
            remove_piece(cap_sq);
            break;
        }

        case PROMOTION: {
            if (state.captured_piece != NO_PIECE) {
                remove_piece(to);
            }
            remove_piece(from);
            put_piece(make_piece(us, m.promo_piece()), to);
            break;
        }
    }

    // Update castling rights
    zobrist_key_ ^= zobrist::CASTLING_KEYS[castling_rights_];
    castling_rights_ &= CASTLING_MASKS[from] & CASTLING_MASKS[to];
    zobrist_key_ ^= zobrist::CASTLING_KEYS[castling_rights_];

    // Change side to move
    side_to_move_ = them;
    zobrist_key_ ^= zobrist::SIDE_KEY;

    if (side_to_move_ == WHITE) {
        ++fullmove_number_;
    }

    // Verify move legality (cannot leave own king in check)
    if (in_check(us)) {
        unmake_move(m);
        return false;
    }

    return true;
}

void Board::unmake_move(Move m) {
    Square from = m.from();
    Square to = m.to();
    MoveType type = m.type();

    if (side_to_move_ == WHITE) {
        --fullmove_number_;
    }
    side_to_move_ = ~side_to_move_;
    Color us = side_to_move_;

    const GameState& state = history_[--history_ply_];
    ep_sq_ = state.ep_sq;
    castling_rights_ = state.castling_rights;
    halfmove_clock_ = state.halfmove_clock;
    zobrist_key_ = state.zobrist_key;

    switch (type) {
        case NORMAL: {
            move_piece(piece_on_[to], to, from);
            if (state.captured_piece != NO_PIECE) {
                put_piece(state.captured_piece, to);
            }
            break;
        }

        case CASTLING: {
            move_piece(piece_on_[to], to, from);
            if (to == SQ_G1) {
                move_piece(W_ROOK, SQ_F1, SQ_H1);
            } else if (to == SQ_C1) {
                move_piece(W_ROOK, SQ_D1, SQ_A1);
            } else if (to == SQ_G8) {
                move_piece(B_ROOK, SQ_F8, SQ_H8);
            } else if (to == SQ_C8) {
                move_piece(B_ROOK, SQ_D8, SQ_A8);
            }
            break;
        }

        case EN_PASSANT: {
            move_piece(piece_on_[to], to, from);
            Square cap_sq = make_square(file_of(to), rank_of(from));
            put_piece(make_piece(~us, PAWN), cap_sq);
            break;
        }

        case PROMOTION: {
            remove_piece(to);
            put_piece(make_piece(us, PAWN), from);
            if (state.captured_piece != NO_PIECE) {
                put_piece(state.captured_piece, to);
            }
            break;
        }
    }
}

void Board::make_null_move() {
    GameState& state = history_[history_ply_++];
    state.ep_sq = ep_sq_;
    state.castling_rights = castling_rights_;
    state.halfmove_clock = halfmove_clock_;
    state.captured_piece = NO_PIECE;
    state.zobrist_key = zobrist_key_;

    if (ep_sq_ != SQ_NONE) {
        zobrist_key_ ^= zobrist::EP_KEYS[ep_sq_];
        ep_sq_ = SQ_NONE;
    }

    side_to_move_ = ~side_to_move_;
    zobrist_key_ ^= zobrist::SIDE_KEY;
    ++halfmove_clock_;
}

void Board::unmake_null_move() {
    side_to_move_ = ~side_to_move_;
    const GameState& state = history_[--history_ply_];
    ep_sq_ = state.ep_sq;
    castling_rights_ = state.castling_rights;
    halfmove_clock_ = state.halfmove_clock;
    zobrist_key_ = state.zobrist_key;
}

bool Board::is_repetition(int count) const {
    int repetitions = 0;
    for (int i = history_ply_ - 2; i >= 0 && i >= history_ply_ - halfmove_clock_; i -= 2) {
        if (history_[i].zobrist_key == zobrist_key_) {
            ++repetitions;
            if (repetitions >= count - 1) {
                return true;
            }
        }
    }
    return false;
}

bool Board::is_insufficient_material() const {
    // If pawns, rooks, or queens exist, material is sufficient
    if (pieces_[W_PAWN] || pieces_[B_PAWN] ||
        pieces_[W_ROOK] || pieces_[B_ROOK] ||
        pieces_[W_QUEEN] || pieces_[B_QUEEN]) {
        return false;
    }

    int white_knights = popcount(pieces_[W_KNIGHT]);
    int black_knights = popcount(pieces_[B_KNIGHT]);
    int white_bishops = popcount(pieces_[W_BISHOP]);
    int black_bishops = popcount(pieces_[B_BISHOP]);

    int total_minors = white_knights + black_knights + white_bishops + black_bishops;

    // King vs King
    if (total_minors == 0) return true;

    // King + minor vs King
    if (total_minors == 1) return true;

    // King + Bishop vs King + Bishop on same color
    if (total_minors == 2 && white_bishops == 1 && black_bishops == 1) {
        Square w_bsq = lsb(pieces_[W_BISHOP]);
        Square b_bsq = lsb(pieces_[B_BISHOP]);
        bool w_light = ((rank_of(w_bsq) + file_of(w_bsq)) % 2) != 0;
        bool b_light = ((rank_of(b_bsq) + file_of(b_bsq)) % 2) != 0;
        if (w_light == b_light) return true;
    }

    return false;
}

} // namespace zenith
