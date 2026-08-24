#include "bitboard.h"
#include <random>
#include <cstring>
#include <vector>

namespace zenith {

U64 PAWN_ATTACKS[COLOR_NB][64];
U64 KNIGHT_ATTACKS[64];
U64 KING_ATTACKS[64];
U64 BETWEEN_BB[64][64];
U64 LINE_BB[64][64];

Magic ROOK_MAGICS[64];
Magic BISHOP_MAGICS[64];

// Table storage for magic attack lookups
// Max 4096 entries per rook square (12 bits), 512 entries per bishop square (9 bits)
static U64 ROOK_TABLE[64 * 4096];
static U64 BISHOP_TABLE[64 * 512];

// PRNG for magic bitboards
static uint64_t prng_state = 1804289383ULL;

static uint64_t rand_u64() {
    uint64_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    prng_state = x;
    return x;
}

static uint64_t rand_u64_sparse() {
    return rand_u64() & rand_u64() & rand_u64();
}

static U64 mask_pawn_attacks(Color c, Square sq) {
    U64 attacks = 0ULL;
    U64 bit = square_bb(sq);

    if (c == WHITE) {
        if ((bit << 7) & ~FILE_H_BB) attacks |= (bit << 7);
        if ((bit << 9) & ~FILE_A_BB) attacks |= (bit << 9);
    } else {
        if ((bit >> 7) & ~FILE_A_BB) attacks |= (bit >> 7);
        if ((bit >> 9) & ~FILE_H_BB) attacks |= (bit >> 9);
    }
    return attacks;
}

static U64 mask_knight_attacks(Square sq) {
    U64 attacks = 0ULL;
    U64 b = square_bb(sq);

    if ((b << 17) & ~FILE_A_BB) attacks |= (b << 17);
    if ((b << 10) & ~(FILE_A_BB | FILE_B_BB)) attacks |= (b << 10);
    if ((b >> 6)  & ~(FILE_A_BB | FILE_B_BB)) attacks |= (b >> 6);
    if ((b >> 15) & ~FILE_A_BB) attacks |= (b >> 15);

    if ((b << 15) & ~FILE_H_BB) attacks |= (b << 15);
    if ((b << 6)  & ~(FILE_G_BB | FILE_H_BB)) attacks |= (b << 6);
    if ((b >> 10) & ~(FILE_G_BB | FILE_H_BB)) attacks |= (b >> 10);
    if ((b >> 17) & ~FILE_H_BB) attacks |= (b >> 17);

    return attacks;
}

static U64 mask_king_attacks(Square sq) {
    U64 attacks = 0ULL;
    U64 b = square_bb(sq);

    if (b << 8) attacks |= (b << 8);
    if (b >> 8) attacks |= (b >> 8);
    if ((b << 1) & ~FILE_A_BB) attacks |= (b << 1);
    if ((b >> 1) & ~FILE_H_BB) attacks |= (b >> 1);
    if ((b << 9) & ~FILE_A_BB) attacks |= (b << 9);
    if ((b << 7) & ~FILE_H_BB) attacks |= (b << 7);
    if ((b >> 7) & ~FILE_A_BB) attacks |= (b >> 7);
    if ((b >> 9) & ~FILE_H_BB) attacks |= (b >> 9);

    return attacks;
}

static U64 mask_bishop_occupancy(Square sq) {
    U64 result = 0ULL;
    int r = rank_of(sq);
    int f = file_of(sq);

    for (int r1 = r + 1, f1 = f + 1; r1 < 7 && f1 < 7; ++r1, ++f1)
        result |= square_bb(make_square(static_cast<File>(f1), static_cast<Rank>(r1)));
    for (int r1 = r + 1, f1 = f - 1; r1 < 7 && f1 > 0; ++r1, --f1)
        result |= square_bb(make_square(static_cast<File>(f1), static_cast<Rank>(r1)));
    for (int r1 = r - 1, f1 = f + 1; r1 > 0 && f1 < 7; --r1, ++f1)
        result |= square_bb(make_square(static_cast<File>(f1), static_cast<Rank>(r1)));
    for (int r1 = r - 1, f1 = f - 1; r1 > 0 && f1 > 0; --r1, --f1)
        result |= square_bb(make_square(static_cast<File>(f1), static_cast<Rank>(r1)));

    return result;
}

static U64 mask_rook_occupancy(Square sq) {
    U64 result = 0ULL;
    int r = rank_of(sq);
    int f = file_of(sq);

    for (int r1 = r + 1; r1 < 7; ++r1)
        result |= square_bb(make_square(static_cast<File>(f), static_cast<Rank>(r1)));
    for (int r1 = r - 1; r1 > 0; --r1)
        result |= square_bb(make_square(static_cast<File>(f), static_cast<Rank>(r1)));
    for (int f1 = f + 1; f1 < 7; ++f1)
        result |= square_bb(make_square(static_cast<File>(f1), static_cast<Rank>(r)));
    for (int f1 = f - 1; f1 > 0; --f1)
        result |= square_bb(make_square(static_cast<File>(f1), static_cast<Rank>(r)));

    return result;
}

static U64 generate_bishop_attacks_on_the_fly(Square sq, U64 block) {
    U64 result = 0ULL;
    int r = rank_of(sq);
    int f = file_of(sq);

    for (int r1 = r + 1, f1 = f + 1; r1 <= 7 && f1 <= 7; ++r1, ++f1) {
        Square s = make_square(static_cast<File>(f1), static_cast<Rank>(r1));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }
    for (int r1 = r + 1, f1 = f - 1; r1 <= 7 && f1 >= 0; ++r1, --f1) {
        Square s = make_square(static_cast<File>(f1), static_cast<Rank>(r1));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }
    for (int r1 = r - 1, f1 = f + 1; r1 >= 0 && f1 <= 7; --r1, ++f1) {
        Square s = make_square(static_cast<File>(f1), static_cast<Rank>(r1));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }
    for (int r1 = r - 1, f1 = f - 1; r1 >= 0 && f1 >= 0; --r1, --f1) {
        Square s = make_square(static_cast<File>(f1), static_cast<Rank>(r1));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }

    return result;
}

static U64 generate_rook_attacks_on_the_fly(Square sq, U64 block) {
    U64 result = 0ULL;
    int r = rank_of(sq);
    int f = file_of(sq);

    for (int r1 = r + 1; r1 <= 7; ++r1) {
        Square s = make_square(static_cast<File>(f), static_cast<Rank>(r1));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }
    for (int r1 = r - 1; r1 >= 0; --r1) {
        Square s = make_square(static_cast<File>(f), static_cast<Rank>(r1));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }
    for (int f1 = f + 1; f1 <= 7; ++f1) {
        Square s = make_square(static_cast<File>(f1), static_cast<Rank>(r));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }
    for (int f1 = f - 1; f1 >= 0; --f1) {
        Square s = make_square(static_cast<File>(f1), static_cast<Rank>(r));
        result |= square_bb(s);
        if (block & square_bb(s)) break;
    }

    return result;
}

static U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
    U64 occupancy = 0ULL;
    for (int i = 0; i < bits_in_mask; ++i) {
        Square sq = pop_lsb(attack_mask);
        if (index & (1 << i)) {
            occupancy |= square_bb(sq);
        }
    }
    return occupancy;
}

static void find_magic(Square sq, bool is_bishop, U64* table_offset) {
    U64 mask = is_bishop ? mask_bishop_occupancy(sq) : mask_rook_occupancy(sq);
    int relevant_bits = popcount(mask);
    int num_indices = 1 << relevant_bits;
    int shift = 64 - relevant_bits;

    std::vector<U64> occupancies(num_indices);
    std::vector<U64> attacks(num_indices);

    for (int i = 0; i < num_indices; ++i) {
        occupancies[i] = set_occupancy(i, relevant_bits, mask);
        attacks[i] = is_bishop ? generate_bishop_attacks_on_the_fly(sq, occupancies[i])
                               : generate_rook_attacks_on_the_fly(sq, occupancies[i]);
    }

    std::vector<U64> used_attacks(num_indices);

    for (int attempt = 0; attempt < 100000000; ++attempt) {
        U64 magic = rand_u64_sparse();
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        std::fill(used_attacks.begin(), used_attacks.end(), 0ULL);
        bool fail = false;

        for (int i = 0; i < num_indices; ++i) {
            int magic_index = static_cast<int>((occupancies[i] * magic) >> shift);
            if (used_attacks[magic_index] == 0ULL) {
                used_attacks[magic_index] = attacks[i];
            } else if (used_attacks[magic_index] != attacks[i]) {
                fail = true;
                break;
            }
        }

        if (!fail) {
            Magic& m = is_bishop ? BISHOP_MAGICS[sq] : ROOK_MAGICS[sq];
            m.mask = mask;
            m.magic = magic;
            m.shift = shift;
            m.attacks = table_offset;
            std::memcpy(table_offset, used_attacks.data(), sizeof(U64) * num_indices);
            return;
        }
    }
}

void init_bitboards() {
    // 1. Precompute Pawn, Knight, King attacks
    for (int sq = 0; sq < 64; ++sq) {
        PAWN_ATTACKS[WHITE][sq] = mask_pawn_attacks(WHITE, static_cast<Square>(sq));
        PAWN_ATTACKS[BLACK][sq] = mask_pawn_attacks(BLACK, static_cast<Square>(sq));
        KNIGHT_ATTACKS[sq]      = mask_knight_attacks(static_cast<Square>(sq));
        KING_ATTACKS[sq]        = mask_king_attacks(static_cast<Square>(sq));
    }

    // 2. Initialize Magics for Bishops and Rooks
    U64* bishop_offset = BISHOP_TABLE;
    for (int sq = 0; sq < 64; ++sq) {
        U64 mask = mask_bishop_occupancy(static_cast<Square>(sq));
        find_magic(static_cast<Square>(sq), true, bishop_offset);
        bishop_offset += (1 << popcount(mask));
    }

    U64* rook_offset = ROOK_TABLE;
    for (int sq = 0; sq < 64; ++sq) {
        U64 mask = mask_rook_occupancy(static_cast<Square>(sq));
        find_magic(static_cast<Square>(sq), false, rook_offset);
        rook_offset += (1 << popcount(mask));
    }

    // 3. Initialize BETWEEN_BB and LINE_BB
    std::memset(BETWEEN_BB, 0, sizeof(BETWEEN_BB));
    std::memset(LINE_BB, 0, sizeof(LINE_BB));

    for (int sq1 = 0; sq1 < 64; ++sq1) {
        for (int sq2 = 0; sq2 < 64; ++sq2) {
            Square s1 = static_cast<Square>(sq1);
            Square s2 = static_cast<Square>(sq2);
            if (s1 == s2) continue;

            int r1 = rank_of(s1), f1 = file_of(s1);
            int r2 = rank_of(s2), f2 = file_of(s2);
            int dr = (r2 > r1) ? 1 : ((r2 < r1) ? -1 : 0);
            int df = (f2 > f1) ? 1 : ((f2 < f1) ? -1 : 0);

            if (r1 == r2 || f1 == f2 || (std::abs(r1 - r2) == std::abs(f1 - f2))) {
                U64 line = 0ULL;
                if (r1 == r2 || f1 == f2) {
                    line = rook_attacks_bb(s1, 0ULL) | square_bb(s1);
                } else {
                    line = bishop_attacks_bb(s1, 0ULL) | square_bb(s1);
                }
                LINE_BB[sq1][sq2] = line;

                int cur_r = r1 + dr;
                int cur_f = f1 + df;
                U64 between = 0ULL;
                while (cur_r != r2 || cur_f != f2) {
                    between |= square_bb(make_square(static_cast<File>(cur_f), static_cast<Rank>(cur_r)));
                    cur_r += dr;
                    cur_f += df;
                }
                BETWEEN_BB[sq1][sq2] = between;
            }
        }
    }
}

void print_bitboard(U64 bb) {
    std::cout << "\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << "  ";
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            std::cout << (get_bit(bb, sq) ? "1 " : ". ");
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n\n";
    std::cout << "   Bitboard (hex): 0x" << std::hex << bb << std::dec << "ULL\n\n";
}

} // namespace zenith
