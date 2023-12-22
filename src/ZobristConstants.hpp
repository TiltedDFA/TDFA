#ifndef ZOBRISTSCONSTANTS_HPP
#define ZOBRISTSCONSTANTS_HPP

#include <random>
#include "Types.hpp"
#include "MagicConstants.hpp"
#include <array>
#include <cassert>
#include "BitBoard.hpp"
#include <limits>

using ZobristKey = U64;
using PieceZobArr = std::array<std::array<std::array<ZobristKey, 64>, 6>, 2>;

namespace Zobrist
{
    //access by [colour][piece type][square]
    static PieceZobArr PIECES_ARR;
    static std::array<ZobristKey, 64> EN_PASSANT_ARR;
    static std::array<ZobristKey, 16> CAST_ARR;
    static ZobristKey WHITE_TO_MOVE;

    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<ZobristKey> rnd_val(std::numeric_limits<ZobristKey>::min(), std::numeric_limits<ZobristKey>::max());

    #if DEVELOPER_MODE == 1
    static bool HAS_BEEN_INITED{false};
    #endif

    void Init()
    {
    #if DEVELOPER_MODE == 1
        assert(!HAS_BEEN_INITED);
    #endif
        std::srand(187697591);

        for(int clr = 0; clr < 2; ++clr)
            for(int pt = 0; pt < 6; ++pt)
                for(int sq = 0; sq < 64; ++sq)
                    PIECES_ARR[clr][pt][sq] = rnd_val(rng);
            
        for(int i = 0; i < 64; ++i) EN_PASSANT_ARR[i] = rnd_val(rng);

        for(int i = 0; i < 16; ++i) CAST_ARR[i] = rnd_val(rng);

        WHITE_TO_MOVE = rnd_val(rng);
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