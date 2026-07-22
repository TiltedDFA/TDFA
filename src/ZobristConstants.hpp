#ifndef ZOBRISTSCONSTANTS_HPP
#define ZOBRISTSCONSTANTS_HPP

#include <random>
#include "Types.hpp"
#include <array>
#include <random>
#include <cassert>
#include <limits>

using ZobristKey = U64;
using PieceZobArr = std::array<std::array<std::array<ZobristKey, 64>, 6>, 2>;
using EnPZobArr =  std::array<ZobristKey, 64>;
using CastlingZobArr = std::array<ZobristKey, 16>;

namespace Zobrist
{
    namespace _
    {
        struct tmp
        {
            PieceZobArr pieces;
            EnPZobArr en_passant;
            CastlingZobArr castling;
            ZobristKey side_to_move;
        };
        constexpr U64 ZobRand64(U64& s)
        {
            //source: stockfish
            s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
            return s * 2685821657736338717LL;
        }
        consteval PieceZobArr InitPieces(U64& prng_seed)
        {
            PieceZobArr arr;
            for(int clr = 0; clr < 2; ++clr)
                for(int pt = 0; pt < 6; ++pt)
                    for(int sq = 0; sq < 64; ++sq)
                        arr[clr][pt][sq] = ZobRand64(prng_seed);
            return arr;
        }
        consteval EnPZobArr InitEnPassant(U64& prng_seed)
        {
            std::array<ZobristKey, 64> arr{};
            for(int i = 0; i < 64; ++i) arr[i] = ZobRand64(prng_seed);
            return arr;
        }
        consteval CastlingZobArr InitCastling(U64& prng_seed)
        {
            std::array<ZobristKey, 16> arr{};
            for(int i = 0; i < 16; ++i) arr[i] = ZobRand64(prng_seed);
            return arr;
        }
        consteval tmp InitTmp()
        {
            tmp t;
            U64 prng_seed{1070372};
            t.pieces = InitPieces(prng_seed);
            t.en_passant = InitEnPassant(prng_seed);
            t.castling = InitCastling(prng_seed);
            t.side_to_move = ZobRand64(prng_seed);
            return t;
        }
    }


    constexpr inline  _::tmp t{_::InitTmp()};
    constexpr inline PieceZobArr    const&  PIECES{t.pieces};
    constexpr inline EnPZobArr      const&  EN_PASSANT{t.en_passant};
    constexpr inline CastlingZobArr const&  CASTLING{t.castling};
    constexpr inline ZobristKey     const&  SIDE_TO_MOVE{t.side_to_move};
}
#endif // #ifndef ZOBRISTSCONSTANTS_HPP