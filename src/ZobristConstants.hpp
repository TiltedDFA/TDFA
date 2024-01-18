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

namespace Zobrist
{
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<ZobristKey> rng_gen(
                                                            std::numeric_limits<ZobristKey>::min(),
                                                            std::numeric_limits<ZobristKey>::max()
                                                            );
    
    //access by [colour][piece type][square]
    const inline PieceZobArr PIECES_ARR = []
                                            {
                                                PieceZobArr arr;
                                                for(int clr = 0; clr < 2; ++clr)
                                                    for(int pt = 0; pt < 6; ++pt)
                                                        for(int sq = 0; sq < 64; ++sq)
                                                            arr[clr][pt][sq] = rng_gen(rng);

                                                return arr;
                                            }();
    const inline std::array<ZobristKey, 64> EN_PASSANT_ARR = []
                                                                {
                                                                    std::array<ZobristKey, 64> arr;
                                                                    for(int i = 0; i < 64; ++i) arr[i] = rng_gen(rng);
                                                                    return arr;
                                                                }();
    //
    const inline std::array<ZobristKey, 16> CAST_ARR = []
                                                        {
                                                            std::array<ZobristKey, 16> arr;
                                                            for(int i = 0; i < 16; ++i) arr[i] = rng_gen(rng);
                                                            return arr;
                                                        }();
    const inline ZobristKey WHITE_TO_MOVE = rng_gen(rng);
}




#endif // #ifndef ZOBRISTSCONSTANTS_HPP