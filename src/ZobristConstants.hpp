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
    const static PieceZobArr PIECES_ARR = []
                                            {
                                                PieceZobArr arr;
                                                for(int clr = 0; clr < 2; ++clr)
                                                    for(int pt = 0; pt < 6; ++pt)
                                                        for(int sq = 0; sq < 64; ++sq)
                                                            arr[clr][pt][sq] = rng_gen(rng);

                                                return arr;
                                            }();
    const static std::array<ZobristKey, 64> EN_PASSANT_ARR = []
                                                                {
                                                                    std::array<ZobristKey, 64> arr;
                                                                    for(int i = 0; i < 64; ++i) arr[i] = rng_gen(rng);
                                                                    return arr;
                                                                }();
    //
    const static std::array<ZobristKey, 16> CAST_ARR = []
                                                        {
                                                            std::array<ZobristKey, 16> arr;
                                                            for(int i = 0; i < 16; ++i) arr[i] = rng_gen(rng);
                                                            return arr;
                                                        }();
    const static ZobristKey WHITE_TO_MOVE = rng_gen(rng);

    // ZobristKey GetHash(const BB::Position& pos)
    // {
    //     ZobristKey ret{0};

    //     ret ^= WHITE_TO_MOVE * pos.whites_turn_;
        
    //     for(int clr = 0; clr < 2; ++clr)
    //     {
    //         for(int type = 0; type < 6; ++type)
    //         {
    //             BitBoard piece_board = pos.GetSpecificPieces(clr, type);
    //             while(piece_board)
    //             {
    //                 const Sq idx = Magics::FindLS1B(piece_board);
    //                 ret ^= PIECES_ARR[clr][type][idx];
    //                 piece_board = Magics::PopLS1B(piece_board);
    //             }
    //         }
    //     }

    //     ret ^= EN_PASSANT_ARR[pos.GetEnPassantSq()];

    //     ret ^= CAST_ARR[pos.GetRawCastling()];

    //     return ret;
    // }
}




#endif // #ifndef ZOBRISTSCONSTANTS_HPP