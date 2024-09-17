#ifndef UTIL_HPP
#define UTIL_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "Position.hpp"
#include "Debug.hpp"
namespace UTIL
{
    inline std::string Square(Sq sq)
    {
        return std::string{char('a' + Magics::FileOf(sq)), char('1' + Magics::RankOf(sq))};
    }
    inline char PromotionChar(MoveType p)
    {
        switch (p)
        {
        case mt_BishopPromotion:
            return 'b';
        case mt_KnightPromotion:
            return 'n';
        case mt_QueenPromotion:
            return 'q';
        case mt_RookPromotion:
            return 'r';
        default:
            return 'z';
        }
    }
    inline std::string MoveToStr(const Move m)
    {
        return (Moves::IsPromotion(m) ? (Square(Moves::StartSq(m)) + Square(Moves::TargetSq(m)) + PromotionChar(Moves::GetMoveType(m))) :
                                            Square(Moves::StartSq(m)) + Square(Moves::TargetSq(m)));
    }
    inline Move UciToMove(const std::string_view str, const Position& pos)
    {
        MoveList ml;
        if(pos.WhiteToMove())
        {
            MoveGen::GeneratePseudoLegalMoves<White>(&pos, &ml);
        }
        else
        {
            MoveGen::GeneratePseudoLegalMoves<Black>(&pos, &ml);
        }

        for(unsigned short i : ml)
        {
            if(str == MoveToStr(i)) return i;
        }
        assert(0);
        return Moves::NULL_MOVE;
    }
}




#endif // #ifndef UTIL_HPP