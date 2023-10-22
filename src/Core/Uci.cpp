#include "Uci.hpp"


std::string UCI::square(Sq sq)
{
    return std::string{char('a' + Magics::FileOf(sq)), char('1' + Magics::RankOf(sq))};
}
std::string UCI::move(Move m)
{
    return square(Moves::GetStartIndex(m)) + square(Moves::GetTargetIndex(m));
}
