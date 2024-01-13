#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "Types.hpp"
#include "BitBoard.hpp"
#include "MagicConstants.hpp"
using Score = I64;

namespace Eval
{
    constexpr Score POS_INF = 9223372036854700800u;
    constexpr Score NEG_INF = -POS_INF;

    constexpr Score PAWN_VAL    = 100;
    constexpr Score KNIGHT_VAL  = 300;
    constexpr Score BISHOP_VAL  = 300;
    constexpr Score ROOK_VAL    = 500;
    constexpr Score QUEEN_VAL   = 900;

    template<bool is_white>
    constexpr Score CountMaterial(const BB::Position& pos)
    {
        Score material{0};
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white,loc::PAWN>())     * PAWN_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white,loc::KNIGHT>())   * KNIGHT_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white,loc::BISHOP>())   * BISHOP_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white,loc::ROOK>())     * ROOK_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white,loc::QUEEN>())    * QUEEN_VAL;
        return material;
    }

    constexpr Score Evaluate(const BB::Position& pos)
    {
        const Score white_eval = CountMaterial<true>(pos);
        const Score black_eval = CountMaterial<false>(pos);

        return (white_eval - black_eval) * (pos.whites_turn_ ? 1 : -1);
    }
}
#endif // #ifndef EVALUATE_HPP