#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "Types.hpp"
#include "Position.hpp"
#include "MagicConstants.hpp"
#include "MoveGen.hpp"
#include <array>
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
    constexpr std::array<Score, 7> PAWN_PROGRESS_BONUS = {0,0,7,15,35,42,70};
    inline bool is_middle_game;

    template<Colour current_turn>
    constexpr Score CountMaterial(Position const* pos)
    {
        Score material{0};
        material += Score(Magics::PopCnt(pos->Pieces(Colour(current_turn), pt_Pawn))     * PAWN_VAL);
        material += Score(Magics::PopCnt(pos->Pieces(Colour(current_turn), pt_Knight))   * KNIGHT_VAL);
        material += Score(Magics::PopCnt(pos->Pieces(Colour(current_turn), pt_Bishop))   * BISHOP_VAL);
        material += Score(Magics::PopCnt(pos->Pieces(Colour(current_turn), pt_Rook))     * ROOK_VAL);
        material += Score(Magics::PopCnt(pos->Pieces(Colour(current_turn), pt_Queen))    * QUEEN_VAL);
        return material;
    }
    constexpr void UpdateData(Position const* pos)
    {
        const BitBoard all_pieces = pos->Pieces(White) | pos->Pieces(Black);
        is_middle_game = Magics::PopCnt(all_pieces) > 16;
    }
    template<Colour current_turn>
    constexpr Score Mobility(Position const* pos)
    {
        Score mobility{0};
        const Score num_attks_king    = Magics::PopCnt(MoveGen::KingAttacks<current_turn>(pos));
        const Score num_attks_queen   = Magics::PopCnt(MoveGen::QueenAttacks<current_turn>(pos));
        const Score num_attks_bishop  = Magics::PopCnt(MoveGen::BishopAttacks<current_turn>(pos));
        const Score num_attks_knight  = Magics::PopCnt(MoveGen::KnightAttacks<current_turn>(pos));
        const Score num_attks_rook    = Magics::PopCnt(MoveGen::RookAttacks<current_turn>(pos));
        mobility += Score(num_attks_king  * (is_middle_game ? -30 : 20));
        mobility += Score(num_attks_queen * (is_middle_game ?   3 : 10));
        mobility += Score(num_attks_bishop* (is_middle_game ?  10 : 20));
        mobility += Score(num_attks_knight* (is_middle_game ?  15 : 10));
        mobility += Score(num_attks_rook  * (is_middle_game ?  10 : 15));
        return mobility;
    }
    template<Colour current_turn>
    constexpr Score PawnProgress(Position const* pos)
    {
        BitBoard our_pawns = pos->Pieces(current_turn, pt_Pawn);
        Score bonus{0};
        while (our_pawns)
        {
            const Sq rank = Magics::RankOf(Magics::FindLS1B(our_pawns));
            if constexpr(current_turn == White)
            {
                bonus += PAWN_PROGRESS_BONUS[rank];
            }
            else
            {
                bonus += PAWN_PROGRESS_BONUS[7-rank];
            }
            our_pawns = Magics::PopLS1B(our_pawns);
        }
        return bonus;
    }
    constexpr Score Evaluate(Position const* pos)
    {
        UpdateData(pos);
        const Score white_eval = CountMaterial<White>(pos) + Mobility<White>(pos) + PawnProgress<White>(pos);
        const Score black_eval = CountMaterial<Black>(pos)+ Mobility<Black>(pos)+ PawnProgress<Black>(pos);

        return (white_eval - black_eval) * (pos->WhiteToMove() ? 1 : -1);
    }
}
#endif // #ifndef EVALUATE_HPP