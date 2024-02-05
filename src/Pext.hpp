#ifndef PEXT_HPP
#define PEXT_HPP
#include <cstdint>
#include <bit>
#include <immintrin.h>

#include "Types.hpp"
#include "MagicConstants.hpp"


typedef uint64_t u64;


namespace Pext
{
    extern u64 bishop_table[5248];
    extern u64 rook_table[102400];
    extern u64 slider_masks[2][64];
    extern int slider_offsets[2][64];

    inline u64 left_mask(u64 bb) {
        return bb & 0x7f7f7f7f7f7f7f7f;
    }

    inline u64 right_mask(u64 bb) {
        return bb & 0xfefefefefefefefe;
    }

    inline u64 no_mask(u64 bb) {
        return bb;
    }

    inline u64 signed_shift(u64 bb, int dir) {
        return dir > 0 ? bb << dir : bb >> -dir;
    }

    template<typename T>
    inline u64 slide(u64 bb, int dir, u64 occupied, T mask) {
        for (int i = 0; i < 7; i++)
            bb |= mask(signed_shift(bb, dir)) & ~occupied;

        return mask(signed_shift(bb, dir));
    }

    inline u64 get_left_pawn_attacks(int color, u64 bb) {
        return left_mask(color ? bb >> 9 : bb << 7);
    }

    inline u64 get_right_pawn_attacks(int color, u64 bb) {
        return right_mask(color ? bb >> 7 : bb << 9);
    }

    inline u64 bishop_attacks_slow(u64 bb, u64 occupied) {
        return slide(bb, -9, occupied, left_mask) |
            slide(bb, -7, occupied, right_mask) |
            slide(bb, 7, occupied, left_mask) |
            slide(bb, 9, occupied, right_mask);
    }

    inline u64 rook_attacks_slow(u64 bb, u64 occupied) {
        return slide(bb, -8, occupied, no_mask) |
            slide(bb, -1, occupied, left_mask) |
            slide(bb, 1, occupied, right_mask) |
            slide(bb, 8, occupied, no_mask);
    }

    inline void PextInit() {
        int current_offset[2] = {};

        for (int sq = 0; sq < 64; sq++) {
            u64 bb = 1ull << sq;
            u64 relevant = ~(
                ((sq >> 3) > 0 ? 0xff : 0) |
                ((sq >> 3) < 7 ? 0xff00000000000000 : 0) |
                ((sq & 7) > 0 ? 0x0101010101010101 : 0) |
                ((sq & 7) < 7 ? 0x8080808080808080 : 0));

            for (int type = 0; type < 2; type++) {
                auto *get_attacks = type ? rook_attacks_slow : bishop_attacks_slow;
                auto *table = type ? rook_table : bishop_table;

                slider_offsets[type][sq] = current_offset[type];
                u64 mask = get_attacks(bb, 0) & relevant;
                slider_masks[type][sq] = mask;

                for (int i = 0; i < 1 << Magics::PopCnt(mask); i++)
                    table[current_offset[type]++] = get_attacks(bb, _pdep_u64(i, mask));
            }
        }
    }

    inline u64 bishop_attacks(int from, u64 occupied) {
        return bishop_table[slider_offsets[0][from] + _pext_u64(occupied, slider_masks[0][from])];
    }

    inline u64 rook_attacks(int from, u64 occupied) {
        return rook_table[slider_offsets[1][from] + _pext_u64(occupied, slider_masks[1][from])];
    }
}

#endif //#ifndef PEXT_HPP