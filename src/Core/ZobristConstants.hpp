#ifndef ZOBRISTSCONSTANTS_HPP
#define ZOBRISTSCONSTANTS_HPP

#include <cstdlib>
#include <cstdint>
#include "Types.hpp"
#include "MagicConstants.hpp"
#include <array>
#include <cassert>
#include <BitBoard.hpp>

using ZobristKey = uint64_t;
using PieceZobArr = std::array<std::array<std::array<ZobristKey, 64>,6>,2>;

namespace Zobrist
{
    //access by [colour][piece type][square]
    static PieceZobArr PIECES_ARR;
    static std::array<ZobristKey, 64> EN_PASSANT_ARR;
    static std::array<ZobristKey, 16> CAST_ARR;
    static ZobristKey WHITE_TO_MOVE;

    #if DEVELOPER_MODE == 1
    static bool HAS_BEEN_INITED{false};
    #endif

    ZobristKey GetHashValue()
    {
        return (std::rand() | std::rand() << 4 | std::rand() << 8 | std::rand() << 12 | std::rand() << 16);
    }
    void Init()
    {
    #if DEVELOPER_MODE == 1
        assert(!HAS_BEEN_INITED);
    #endif
        std::srand(187697591);

        for(int clr = 0; clr < 2; ++clr)
            for(int pt = 0; pt < 6; ++pt)
                for(int sq = 0; sq < 64; ++sq)
                    PIECES_ARR[clr][pt][sq] = GetHashValue();
            
        for(int i = 0; i < 64; ++i) EN_PASSANT_ARR[i] = GetHashValue();

        for(int i = 0; i < 16; ++i) CAST_ARR[i] = GetHashValue();

        WHITE_TO_MOVE = GetHashValue();
    }
    ZobristKey GetHash(const BB::Position& pos)
    {
        ZobristKey ret{0};

        ret ^= WHITE_TO_MOVE * pos.whites_turn_;
        
        for(int clr = 0; clr < 2; ++clr)
        {
            for(int type = 0; type < 6; ++type)
            {
                BitBoard piece_board = pos.GetSpecificPieces(clr, type);
                while(piece_board)
                {
                    const Sq idx = Magics::FindLS1B(piece_board);
                    ret ^= PIECES_ARR[clr][type][idx];
                    piece_board = Magics::PopLS1B(piece_board);
                }
            }
        }

        ret ^= EN_PASSANT_ARR[pos.GetEnPassantSq()];

        ret ^= CAST_ARR[pos.GetRawCastling()];

        return ret;
    }
}




#endif // #ifndef ZOBRISTSCONSTANTS_HPP