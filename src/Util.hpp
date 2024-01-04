#ifndef UTIL_HPP
#define UTIL_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"


namespace UTIL
{
    static std::string Square(Sq sq)
    {
        return std::string{char('a' + Magics::FileOf(sq)), char('1' + Magics::RankOf(sq))};
    }
    static char PromotionChar(PieceType p)
    {
        switch (p)
        {
        case Moves::PROM_BISHOP:
            return 'b';
        case Moves::PROM_KNIGHT:
            return 'n';
        case Moves::PROM_QUEEN:
            return 'q';
        case Moves::PROM_ROOK:
            return 'r';
        default:
            return 'z';
        }
    }
    static std::string MoveToStr(Move m)
    {
        return (Moves::IsPromotionMove(m) ? (Square(Moves::GetStartIndex(m)) + Square(Moves::GetTargetIndex(m)) + PromotionChar(Moves::GetPieceType(m))) :
                                            Square(Moves::GetStartIndex(m)) + Square(Moves::GetTargetIndex(m)));
    }
}




#endif // #ifndef UTIL_HPP