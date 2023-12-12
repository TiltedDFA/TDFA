#ifndef ZOBRISTSCONSTANTS_HPP
#define ZOBRISTSCONSTANTS_HPP

#include <cstdlib>
#include <cstdint>
#include "Types.hpp"
#include <array>
#include <cassert>

using ZobristKey = uint64_t;
using PieceZobArr = std::array<std::array<std::array<ZobristKey, 64>,6>,2>;

namespace Zobrist
{


    //access by [colour][piece type][square]
    static PieceZobArr PIECES_ARR;
    static std::array<ZobristKey, 64> EN_PASSANT_ARR;
    static std::array<ZobristKey, 16> CAST_ARR;

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
    }
}




#endif // #ifndef ZOBRISTSCONSTANTS_HPP