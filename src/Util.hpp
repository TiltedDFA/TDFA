#ifndef UTIL_HPP
#define UTIL_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "BitBoard.hpp"
#include "Debug.hpp"
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
    static Move UciToMove(const std::string_view str, const BB::Position& pos)
    {
        const Sq from = (str[0] - 'a') + (str[1] - '1') * 8;
        const Sq to = (str[2] - 'a') + (str[3] - '1') * 8;

        if(str.length() == 5)
        {
            switch(str[4])
            {
            case 'q':
                return Moves::EncodeMove(from, to, Moves::PROM_QUEEN);
            case 'r':
                return Moves::EncodeMove(from, to, Moves::PROM_ROOK);
            case 'b':
                return Moves::EncodeMove(from, to, Moves::PROM_BISHOP);
            case 'n':
                return Moves::EncodeMove(from, to, Moves::PROM_KNIGHT);
            default:
                return Moves::EncodeMove(from, to, Moves::BAD_MOVE);
            }
        }

        PieceType type;
        if(pos.whites_turn_)
            type = pos.GetTypeAtSq<true>(from);
        else
            type = pos.GetTypeAtSq<false>(from);

        const Move constructed_move = Moves::EncodeMove(from, to, type);
        Debug::PrintEncodedMoveStr(constructed_move);
        return constructed_move;
    }
}




#endif // #ifndef UTIL_HPP