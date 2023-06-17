#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"

namespace Bitboard
{
    struct Boards
    {
    public:
        constexpr Boards():
        white_kings_(0ull), white_queens_(0ull), white_rooks_(0ull), white_bishops_(0ull), white_knights_(0ull), white_pawns_(0ull),
        black_kings_(0ull), black_queens_(0ull), black_rooks_(0ull), black_bishops_(0ull), black_knights_(0ull), black_pawns_(0ull){}
    public:
        BitBoard white_kings_;
        BitBoard white_queens_;
        BitBoard white_rooks_;
        BitBoard white_bishops_;
        BitBoard white_knights_;
        BitBoard white_pawns_;

        BitBoard black_kings_;
        BitBoard black_queens_;
        BitBoard black_rooks_;
        BitBoard black_bishops_;
        BitBoard black_knights_;
        BitBoard black_pawns_;
    };
    
}

#endif //#ifndef BITBOARD_HPP