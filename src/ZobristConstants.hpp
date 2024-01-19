#ifndef ZOBRISTSCONSTANTS_HPP
#define ZOBRISTSCONSTANTS_HPP

#include <random>
#include "Types.hpp"
#include "MagicConstants.hpp"
#include <array>
#include <random>
#include <cassert>
#include <limits>

using ZobristKey = U64;
using PieceZobArr = std::array<std::array<std::array<ZobristKey, 64>, 6>, 2>;
//source: stockfish
namespace Zobrist
{    
    constexpr U64 ZobRand64(U64& s) 
    {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
    constexpr U64 ZobRand64NoRef(U64 s) 
    {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
    constexpr PieceZobArr InitPieces()
    {
        U64 prng_seed{1070372};

        PieceZobArr arr;
        for(int clr = 0; clr < 2; ++clr)
            for(int pt = 0; pt < 6; ++pt)
                for(int sq = 0; sq < 64; ++sq)
                    arr[clr][pt][sq] = ZobRand64(prng_seed);
        return arr;
    }
    constexpr std::array<ZobristKey, 64> InitEnPassant()
    {
        U64 prng_seed{1070372};
        std::array<ZobristKey, 64> arr;
        for(int i = 0; i < 64; ++i) arr[i] = ZobRand64(prng_seed);
        return arr;
    }
    constexpr std::array<ZobristKey, 16> InitCastling()
    {
        U64 prng_seed{1909068137};
        std::array<ZobristKey, 16> arr;
        for(int i = 0; i < 16; ++i) arr[i] = ZobRand64(prng_seed);
        return arr;
    }
    constexpr inline PieceZobArr PIECES = InitPieces();
    constexpr inline std::array<ZobristKey, 64> EN_PASSANT = InitEnPassant();
    constexpr inline std::array<ZobristKey, 16> CASTLING = InitCastling();
    constexpr inline ZobristKey SIDE_TO_MOVE = ZobRand64NoRef(144641901);
}




#endif // #ifndef ZOBRISTSCONSTANTS_HPP