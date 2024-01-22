#ifndef UTIL_HPP
#define UTIL_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "BitBoard.hpp"
#include "Debug.hpp"
namespace UTIL
{
    inline std::string Square(Sq sq)
    {
        return std::string{char('a' + Magics::FileOf(sq)), char('1' + Magics::RankOf(sq))};
    }
    inline char PromotionChar(PieceType p)
    {
        switch (p)
        {
        case PromBishop:
            return 'b';
        case PromKnight:
            return 'n';
        case PromQueen:
            return 'q';
        case PromRook:
            return 'r';
        default:
            return 'z';
        }
    }
    inline std::string MoveToStr(Move m)
    {
        return (Moves::IsPromotionMove(m) ? (Square(Moves::StartSq(m)) + Square(Moves::TargetSq(m)) + PromotionChar(Moves::PType(m))) :
                                            Square(Moves::StartSq(m)) + Square(Moves::TargetSq(m)));
    }
    inline Move UciToMove(const std::string_view str, const Position& pos)
    {
        const Sq from = (str[0] - 'a') + (str[1] - '1') * 8;
        const Sq to = (str[2] - 'a') + (str[3] - '1') * 8;

        if(str.length() == 5)
        {
            switch(str[4])
            {
            case 'q':
                return Moves::EncodeMove(from, to, PromQueen);
            case 'r':
                return Moves::EncodeMove(from, to, PromRook);
            case 'b':
                return Moves::EncodeMove(from, to, PromBishop);
            case 'n':
                return Moves::EncodeMove(from, to, PromKnight);
            default:
                return Moves::EncodeMove(from, to, NullPiece);
            }
        }

        PieceType type;
        if(pos.WhiteToMove())
            type = pos.TypeAtSq<true>(from);
        else
            type = pos.TypeAtSq<false>(from);

        const Move constructed_move = Moves::EncodeMove(from, to, type);
        Debug::PrintEncodedMoveStr(constructed_move);
        return constructed_move;
    }
}
#endif // #ifndef UTIL_HPP