#include "Uci.hpp"


std::string UCI::square(Sq sq)
{
    return std::string{char('a' + Magics::FileOf(sq)), char('1' + Magics::RankOf(sq))};
}
std::string PromotionChar(PieceType p)
{
    switch (p)
    {
    case Moves::PROM_BISHOP:
        return "b";
    case Moves::PROM_KNIGHT:
        return "n";
    case Moves::PROM_QUEEN:
        return "q";
    case Moves::PROM_ROOK:
        return "r";
    default:
        return "z";
    }
}
std::string UCI::move(Move m)
{
    return (Moves::IsPromotionMove(m) ? (square(Moves::GetStartIndex(m)) + square(Moves::GetTargetIndex(m)) + PromotionChar(Moves::GetPieceType(m))) :
                                        square(Moves::GetStartIndex(m)) + square(Moves::GetTargetIndex(m)));
}
