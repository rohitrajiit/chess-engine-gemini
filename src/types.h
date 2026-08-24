#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <array>
#include <vector>
#include <string_view>

namespace zenith {

using U64 = uint64_t;

enum Color : int {
    WHITE = 0,
    BLACK = 1,
    COLOR_NB = 2,
    NO_COLOR = 3
};

constexpr Color operator~(Color c) {
    return static_cast<Color>(c ^ 1);
}

enum PieceType : int {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    PIECE_TYPE_NB = 6,
    NO_PIECE_TYPE = 6
};

enum Piece : int {
    W_PAWN = 0, W_KNIGHT = 1, W_BISHOP = 2, W_ROOK = 3, W_QUEEN = 4, W_KING = 5,
    B_PAWN = 6, B_KNIGHT = 7, B_BISHOP = 8, B_ROOK = 9, B_QUEEN = 10, B_KING = 11,
    PIECE_NB = 12,
    NO_PIECE = 12
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return static_cast<Piece>(c * 6 + pt);
}

constexpr PieceType type_of(Piece p) {
    return static_cast<PieceType>(p % 6);
}

constexpr Color color_of(Piece p) {
    return static_cast<Color>(p / 6);
}

enum Square : int {
    SQ_A1 = 0, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64
};

enum File : int {
    FILE_A = 0, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
};

enum Rank : int {
    RANK_1 = 0, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
};

constexpr Square make_square(File f, Rank r) {
    return static_cast<Square>((r << 3) | f);
}

constexpr File file_of(Square sq) {
    return static_cast<File>(sq & 7);
}

constexpr Rank rank_of(Square sq) {
    return static_cast<Rank>(sq >> 3);
}

enum CastlingRights : uint8_t {
    NO_CASTLING   = 0,
    WHITE_OO      = 1,
    WHITE_OOO     = 2,
    BLACK_OO      = 4,
    BLACK_OOO     = 8,
    WHITE_CASTLE  = WHITE_OO | WHITE_OOO,
    BLACK_CASTLE  = BLACK_OO | BLACK_OOO,
    ALL_CASTLING  = WHITE_CASTLE | BLACK_CASTLE
};

enum MoveType : uint16_t {
    NORMAL     = 0,
    PROMOTION  = 1,
    EN_PASSANT = 2,
    CASTLING   = 3
};

// 16-bit Move structure:
// bits 0-5:   from square (0..63)
// bits 6-11:  to square (0..63)
// bits 12-13: promotion piece type (0=KNIGHT, 1=BISHOP, 2=ROOK, 3=QUEEN)
// bits 14-15: move type (NORMAL=0, PROMOTION=1, EN_PASSANT=2, CASTLING=3)
struct Move {
    uint16_t data{0};

    constexpr Move() : data(0) {}
    constexpr explicit Move(uint16_t d) : data(d) {}
    constexpr Move(Square from, Square to, MoveType type = NORMAL, PieceType promo = KNIGHT) {
        data = static_cast<uint16_t>(from) |
               (static_cast<uint16_t>(to) << 6) |
               (static_cast<uint16_t>((promo - KNIGHT) & 3) << 12) |
               (static_cast<uint16_t>(type & 3) << 14);
    }

    constexpr Square from() const {
        return static_cast<Square>(data & 0x3F);
    }

    constexpr Square to() const {
        return static_cast<Square>((data >> 6) & 0x3F);
    }

    constexpr MoveType type() const {
        return static_cast<MoveType>((data >> 14) & 0x3);
    }

    constexpr PieceType promo_piece() const {
        return static_cast<PieceType>(((data >> 12) & 0x3) + KNIGHT);
    }

    constexpr bool is_none() const {
        return data == 0;
    }

    constexpr bool operator==(const Move& other) const {
        return data == other.data;
    }

    constexpr bool operator!=(const Move& other) const {
        return data != other.data;
    }

    std::string to_uci() const {
        if (is_none()) return "0000";
        std::string s;
        Square f = from();
        Square t = to();
        s += static_cast<char>('a' + file_of(f));
        s += static_cast<char>('1' + rank_of(f));
        s += static_cast<char>('a' + file_of(t));
        s += static_cast<char>('1' + rank_of(t));
        if (type() == PROMOTION) {
            switch (promo_piece()) {
                case KNIGHT: s += 'n'; break;
                case BISHOP: s += 'b'; break;
                case ROOK:   s += 'r'; break;
                case QUEEN:  s += 'q'; break;
                default: break;
            }
        }
        return s;
    }
};

constexpr Move MOVE_NONE = Move(0);

// Search and evaluation score constants
constexpr int SCORE_INF = 32000;
constexpr int SCORE_MATE = 30000;
constexpr int SCORE_MATE_BOUND = 29000;
constexpr int MAX_PLY = 64;

inline std::string square_to_str(Square sq) {
    if (sq == SQ_NONE) return "-";
    std::string s;
    s += static_cast<char>('a' + file_of(sq));
    s += static_cast<char>('1' + rank_of(sq));
    return s;
}

inline Square str_to_square(std::string_view s) {
    if (s.length() < 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8') {
        return SQ_NONE;
    }
    return make_square(static_cast<File>(s[0] - 'a'), static_cast<Rank>(s[1] - '1'));
}

inline char piece_to_char(Piece p) {
    constexpr char chars[] = "PNBRQKpnbrqk.";
    return chars[p];
}

inline Piece char_to_piece(char c) {
    switch (c) {
        case 'P': return W_PAWN;
        case 'N': return W_KNIGHT;
        case 'B': return W_BISHOP;
        case 'R': return W_ROOK;
        case 'Q': return W_QUEEN;
        case 'K': return W_KING;
        case 'p': return B_PAWN;
        case 'n': return B_KNIGHT;
        case 'b': return B_BISHOP;
        case 'r': return B_ROOK;
        case 'q': return B_QUEEN;
        case 'k': return B_KING;
        default: return NO_PIECE;
    }
}

} // namespace zenith
