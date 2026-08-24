#pragma once

#include "types.h"
#include <iostream>
#include <iomanip>

namespace zenith {

// Bit manipulation intrinsics
inline int popcount(U64 b) {
    return __builtin_popcountll(b);
}

inline Square lsb(U64 b) {
    return static_cast<Square>(__builtin_ctzll(b));
}

inline Square pop_lsb(U64& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

constexpr U64 square_bb(Square sq) {
    return 1ULL << sq;
}

constexpr bool get_bit(U64 b, Square sq) {
    return (b >> sq) & 1ULL;
}

constexpr void set_bit(U64& b, Square sq) {
    b |= (1ULL << sq);
}

constexpr void clear_bit(U64& b, Square sq) {
    b &= ~(1ULL << sq);
}

// File and Rank Bitboard Constants
constexpr U64 FILE_A_BB = 0x0101010101010101ULL;
constexpr U64 FILE_B_BB = FILE_A_BB << 1;
constexpr U64 FILE_C_BB = FILE_A_BB << 2;
constexpr U64 FILE_D_BB = FILE_A_BB << 3;
constexpr U64 FILE_E_BB = FILE_A_BB << 4;
constexpr U64 FILE_F_BB = FILE_A_BB << 5;
constexpr U64 FILE_G_BB = FILE_A_BB << 6;
constexpr U64 FILE_H_BB = FILE_A_BB << 7;

constexpr U64 RANK_1_BB = 0x00000000000000FFULL;
constexpr U64 RANK_2_BB = RANK_1_BB << 8;
constexpr U64 RANK_3_BB = RANK_1_BB << 16;
constexpr U64 RANK_4_BB = RANK_1_BB << 24;
constexpr U64 RANK_5_BB = RANK_1_BB << 32;
constexpr U64 RANK_6_BB = RANK_1_BB << 40;
constexpr U64 RANK_7_BB = RANK_1_BB << 48;
constexpr U64 RANK_8_BB = RANK_1_BB << 56;

// Precomputed attack tables
extern U64 PAWN_ATTACKS[COLOR_NB][64];
extern U64 KNIGHT_ATTACKS[64];
extern U64 KING_ATTACKS[64];
extern U64 BETWEEN_BB[64][64];
extern U64 LINE_BB[64][64];

// Magic bitboard structures
struct Magic {
    const U64* attacks;
    U64 mask;
    U64 magic;
    int shift;

    U64 get_attacks(U64 occ) const {
        return attacks[((occ & mask) * magic) >> shift];
    }
};

extern Magic ROOK_MAGICS[64];
extern Magic BISHOP_MAGICS[64];

// Attack getters
inline U64 pawn_attacks_bb(Color c, Square sq) {
    return PAWN_ATTACKS[c][sq];
}

inline U64 knight_attacks_bb(Square sq) {
    return KNIGHT_ATTACKS[sq];
}

inline U64 king_attacks_bb(Square sq) {
    return KING_ATTACKS[sq];
}

inline U64 bishop_attacks_bb(Square sq, U64 occ) {
    return BISHOP_MAGICS[sq].get_attacks(occ);
}

inline U64 rook_attacks_bb(Square sq, U64 occ) {
    return ROOK_MAGICS[sq].get_attacks(occ);
}

inline U64 queen_attacks_bb(Square sq, U64 occ) {
    return bishop_attacks_bb(sq, occ) | rook_attacks_bb(sq, occ);
}

inline U64 attacks_bb(PieceType pt, Square sq, U64 occ) {
    switch (pt) {
        case KNIGHT: return knight_attacks_bb(sq);
        case BISHOP: return bishop_attacks_bb(sq, occ);
        case ROOK:   return rook_attacks_bb(sq, occ);
        case QUEEN:  return queen_attacks_bb(sq, occ);
        case KING:   return king_attacks_bb(sq);
        default:     return 0ULL;
    }
}

// Bitboard initialization
void init_bitboards();
void print_bitboard(U64 bb);

} // namespace zenith
