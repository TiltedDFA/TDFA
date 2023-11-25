#ifndef MOVE_HPP
#define MOVE_HPP
#include <cstdint>
#include <algorithm>

#include "Types.hpp"
#include "MagicConstants.hpp"

/*
Move storage: 
from right to left:
6 bits - start index
6 bits - end index
4 bits - piece type. This is also used for promotions
E.g. 
 0000        | 000000     | 000000
piece type   end index    start index
*/
namespace Moves
{
    constexpr PieceType KING            = 0b0000;
    constexpr PieceType QUEEN           = 0b0001;
    constexpr PieceType BISHOP          = 0b0010;
    constexpr PieceType KNIGHT          = 0b0011;
    constexpr PieceType ROOK            = 0b0100;
    constexpr PieceType PAWN            = 0b0101;
    constexpr PieceType PROM_QUEEN      = 0b1000;
    constexpr PieceType PROM_BISHOP     = 0b1001;
    constexpr PieceType PROM_KNIGHT     = 0b1010;
    constexpr PieceType PROM_ROOK       = 0b1011;
    constexpr uint32_t START_SQ_MASK    = 0x0000003F;
    constexpr uint32_t END_SQ_MASK      = 0x00000FC0;
    constexpr uint32_t PIECE_TYPE_MASK  = 0x0000F000;
    constexpr uint32_t COLOUR_MASK      = 0x00008000;
    constexpr uint16_t END_SQ_SHIFT     = 6;
    constexpr uint16_t PIECE_TYPE_SHIFT = 12;

    [[nodiscard]] constexpr Move EncodeMove(const Sq start_index, const Sq target_index, const PieceType piece_type)
    {
        Move move{0};
        move |= start_index & START_SQ_MASK;
        move |= (target_index & START_SQ_MASK) << END_SQ_SHIFT;
        move |= (piece_type << PIECE_TYPE_SHIFT) & PIECE_TYPE_MASK;
        return move;
    }
    constexpr void DecodeMove(const Move move, Sq& start_index, Sq& target_index, PieceType& piece_type)
    {
        start_index = move & START_SQ_MASK;
        target_index =  (move & END_SQ_MASK) >> END_SQ_SHIFT;
        piece_type = (move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT;
    }

    constexpr PieceType GetPieceType(const Move move) {return (move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT;}
    
    constexpr uint8_t GetTargetIndex(const Move move)     {return(move & END_SQ_MASK) >> END_SQ_SHIFT;}

    constexpr uint8_t GetStartIndex(const Move move)      {return move & START_SQ_MASK;}
    
    constexpr bool IsPromotionMove(const Move move) {return move & 0x8000;}

    template<PieceType type>
    constexpr Move SetPieceType(const Move move)
    {
        return (move & ~PIECE_TYPE_MASK) | (type << PIECE_TYPE_SHIFT);
    }
}



#endif // #ifndef MOVE_HPP