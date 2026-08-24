#pragma once

#include "types.h"
#include "bitboard.h"
#include "zobrist.h"
#include <string>
#include <vector>
#include <array>

namespace zenith {

struct GameState {
    Square ep_sq{SQ_NONE};
    uint8_t castling_rights{0};
    int halfmove_clock{0};
    Piece captured_piece{NO_PIECE};
    U64 zobrist_key{0ULL};
};

class Board {
public:
    static constexpr const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board();
    explicit Board(const std::string& fen);

    void reset();
    bool set_fen(const std::string& fen);
    std::string get_fen() const;
    void print() const;

    // Bitboard and piece getters
    U64 piece_bb(Piece p) const { return pieces_[p]; }
    U64 piece_bb(Color c, PieceType pt) const { return pieces_[make_piece(c, pt)]; }
    U64 color_bb(Color c) const { return occupancies_[c]; }
    U64 occupied_bb() const { return occupancies_[2]; }
    Piece piece_at(Square sq) const { return piece_on_[sq]; }
    Color side_to_move() const { return side_to_move_; }
    Square ep_square() const { return ep_sq_; }
    uint8_t castling_rights() const { return castling_rights_; }
    int halfmove_clock() const { return halfmove_clock_; }
    int fullmove_number() const { return fullmove_number_; }
    U64 zobrist_key() const { return zobrist_key_; }
    int game_ply() const { return history_ply_; }

    Square king_square(Color c) const {
        return lsb(pieces_[make_piece(c, KING)]);
    }

    // Attacks and checks
    bool is_square_attacked(Square sq, Color attacking_color) const;
    bool in_check(Color c) const {
        Square ksq = king_square(c);
        return (ksq != SQ_NONE) && is_square_attacked(ksq, ~c);
    }
    bool in_check() const {
        return in_check(side_to_move_);
    }

    // Move execution
    bool make_move(Move m);
    void unmake_move(Move m);
    void make_null_move();
    void unmake_null_move();

    // Draw detections
    bool is_repetition(int count = 2) const;
    bool is_draw_50_moves() const { return halfmove_clock_ >= 100; }
    bool is_insufficient_material() const;
    bool is_draw() const {
        return is_draw_50_moves() || is_repetition() || is_insufficient_material();
    }

    // Hash computation
    U64 compute_zobrist_key() const;

private:
    void clear();
    void put_piece(Piece p, Square sq);
    void remove_piece(Square sq);
    void move_piece(Piece p, Square from, Square to);

    std::array<U64, PIECE_NB> pieces_{};
    std::array<U64, 3> occupancies_{};
    std::array<Piece, 64> piece_on_{};

    Color side_to_move_{WHITE};
    Square ep_sq_{SQ_NONE};
    uint8_t castling_rights_{NO_CASTLING};
    int halfmove_clock_{0};
    int fullmove_number_{1};
    U64 zobrist_key_{0ULL};

    // History for unmake and repetition detection
    static constexpr int MAX_HISTORY = 1024;
    std::array<GameState, MAX_HISTORY> history_{};
    int history_ply_{0};

    // Castling rights update table
    static constexpr uint8_t CASTLING_MASKS[64] = {
        13, 15, 15, 15, 12, 15, 15, 14, // 0..7 (a1..h1)
        15, 15, 15, 15, 15, 15, 15, 15, // 8..15
        15, 15, 15, 15, 15, 15, 15, 15, // 16..23
        15, 15, 15, 15, 15, 15, 15, 15, // 24..31
        15, 15, 15, 15, 15, 15, 15, 15, // 32..39
        15, 15, 15, 15, 15, 15, 15, 15, // 40..47
        15, 15, 15, 15, 15, 15, 15, 15, // 48..55
        7,  15, 15, 15,  3, 15, 15, 11  // 56..63 (a8..h8)
    };
};

} // namespace zenith
