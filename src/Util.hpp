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
    inline char PromotionChar(PieceType p)
    {
        switch (p)
        {
        case pt_prom_bishop:
            return 'b';
        case pt_prom_knight:
            return 'n';
        case pt_prom_queen:
            return 'q';
        case pt_prom_rook:
            return 'r';
        default:
            return 'z';
        }
    }
    inline std::string MoveToStr(const Move m)
    {
        if(Moves::IsPromotionMove(m))
        {
            const std::string start = Square(Moves::StartSq(m));
            const std::string end   = Square(Moves::TargetSq(m));
            const char prom = PromotionChar(Moves::PTypeOfProm(m));
            return start + end + prom;
        }
        else
        {
            const std::string start = Square(Moves::StartSq(m));
            const std::string end   = Square(Moves::TargetSq(m));
            return start + end;
        }
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
                return Moves::EncodeMove(from, to, mt_QueenPromotion);
            case 'r':
                return Moves::EncodeMove(from, to, mt_RookPromotion);
            case 'b':
                return Moves::EncodeMove(from, to, mt_BishopPromotion);
            case 'n':
                return Moves::EncodeMove(from, to, mt_KnightPromotion);
            default:
                {
                    std::cerr << "Unrecognized promotion char: " << str[4] << std::endl;
                    std::abort();
                }
            }
        }

        // PieceType type = Magics::TypeOf(pos.PieceOn(from));
        // if(pos.ColourToMove() == White)
        //     type = ;
        // else
        //     type = pos.TypeAtSq<false>(from);

        Move constructed_move = Moves::NULL_MOVE;
        if (pos.PieceOn(to) != p_None)
        {
            constructed_move = Moves::EncodeMove(from, to, mt_Capture);
        }
        else if (Magics::TypeOf(pos.PieceOn(from)) == pt_Pawn)
        {
            auto const distance = std::abs(to - from);
            if (distance == 7 || distance == 9)
            {
                constructed_move = Moves::EncodeMove(from, to, mt_EnPassant);
            }
            else
            {
                constructed_move = Moves::EncodeMove(from, to, mt_Quiet);
            }
        }
        else if (Magics::TypeOf(pos.PieceOn(from)) == pt_King && std::abs(to - from) == 2)
        {
            constructed_move = Moves::EncodeMove(from, to, mt_Castling);
        }
        else
        {
            constructed_move = Moves::EncodeMove(from, to, mt_Quiet);
        }

        Debug::PrintEncodedMoveStr(constructed_move);
        return constructed_move;
    }
}




#endif // #ifndef UTIL_HPP