#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "Types.hpp"
#include "BitBoard.hpp"
#include "MagicConstants.hpp"
#include "MoveGen.hpp"

namespace Eval
{
    //constants
    constexpr Score POS_INF = std::numeric_limits<Score>::max() >> 1;
    constexpr Score NEG_INF = -POS_INF;
    constexpr Score PAWN_VAL    = 100;
    constexpr Score KNIGHT_VAL  = 310;
    constexpr Score BISHOP_VAL  = 320;
    constexpr Score ROOK_VAL    = 520;
    constexpr Score QUEEN_VAL   = 950;

    inline bool is_middle_game;

    template<bool is_white>
    constexpr Score CountMaterial(const BB::Position& pos)
    {
        Score material{0};
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white, loc::PAWN>())     * PAWN_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white, loc::KNIGHT>())   * KNIGHT_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white, loc::BISHOP>())   * BISHOP_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white, loc::ROOK>())     * ROOK_VAL;
        material += Magics::PopCnt(pos.GetSpecificPieces<is_white, loc::QUEEN>())    * QUEEN_VAL;
        return material;
    }
    constexpr void UpdateData(const BB::Position& pos)
    {
        const BitBoard all_pieces = pos.GetPieces<false>() | pos.GetPieces<true>();
        is_middle_game = Magics::PopCnt(all_pieces) > 8;
    }
    template<bool is_white>
    constexpr Score Mobility(const BB::Position& pos)
    {
        Score mobility{0};
        const U8 num_attks_king    = Magics::PopCnt(MoveGen::KingAttacks<is_white>(pos));
        const U8 num_attks_queen   = Magics::PopCnt(MoveGen::QueenAttacks<is_white>(pos));
        const U8 num_attks_bishop  = Magics::PopCnt(MoveGen::BishopAttacks<is_white>(pos));
        const U8 num_attks_knight  = Magics::PopCnt(MoveGen::KnightAttacks<is_white>(pos));
        const U8 num_attks_rook    = Magics::PopCnt(MoveGen::RookAttacks<is_white>(pos));
        mobility += num_attks_king  * (is_middle_game ? -10 : 20);
        mobility += num_attks_queen * (is_middle_game ?   5 : 15);
        mobility += num_attks_bishop* (is_middle_game ?  10 : 20);
        mobility += num_attks_knight* (is_middle_game ?  15 : 10);
        mobility += num_attks_rook  * (is_middle_game ?  10 : 15);
        return mobility;
    }
    constexpr Score Evaluate(const BB::Position& pos)
    {
        UpdateData(pos);
        // const Score white_eval = CountMaterial<true>(pos);
        // const Score black_eval = CountMaterial<false>(pos);
        const Score white_eval = CountMaterial<true>(pos) + Mobility<true>(pos);
        const Score black_eval = CountMaterial<false>(pos)+ Mobility<false>(pos);

        return (white_eval - black_eval) * (pos.whites_turn_ ? 1 : -1);
    }
}
#endif // #ifndef EVALUATE_HPP