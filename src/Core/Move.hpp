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
3 bits - piece type 
1 bit  - colour (white = 1/black = 0)
E.g. 
 0      | 000        | 000000     | 000000
 colour   piece type   end index    start index
*/
namespace Moves
{
    constexpr PieceType KING            = 0;
    constexpr PieceType QUEEN           = 1;
    constexpr PieceType BISHOP          = 2;
    constexpr PieceType KNIGHT          = 3;
    constexpr PieceType ROOK            = 4;
    constexpr PieceType PAWN            = 5;
    constexpr uint32_t START_SQ_MASK    = 0x0000003F;
    constexpr uint32_t END_SQ_MASK      = 0x00000FC0;
    constexpr uint32_t PIECE_TYPE_MASK  = 0x00007000;
    constexpr uint32_t COLOUR_MASK      = 0x00008000;
    constexpr uint16_t END_SQ_SHIFT     = 6;
    constexpr uint16_t PIECE_TYPE_SHIFT = 12;
    constexpr uint16_t COLOUR_SHIFT     = 15;
    [[nodiscard]] constexpr Move EncodeMove(const uint8_t start_index, const uint8_t target_index, const PieceType piece_type, const bool is_white)
    {
        Move move{0};
        move |= start_index & START_SQ_MASK;
        move |= (target_index & START_SQ_MASK) << END_SQ_SHIFT;
        move |= (piece_type << PIECE_TYPE_SHIFT) & PIECE_TYPE_MASK;
        move |= (is_white ? 1u : 0u) << COLOUR_SHIFT;
        return move;
    }
    constexpr void DecodeMove(const Move move, uint8_t& start_index, uint8_t& target_index, PieceType& piece_type, bool& is_white)
    {
        start_index = move & START_SQ_MASK;
        target_index =  (move & END_SQ_MASK) >> END_SQ_SHIFT;
        piece_type = (move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT;
        is_white = (move >> COLOUR_SHIFT);
    }
    constexpr bool IsWhite(const Move move)           {return (move >> COLOUR_SHIFT) == 1;}

    constexpr PieceType GetPieceType(const Move move) {return (move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT;}
    
    constexpr int GetTargetIndex(const Move move)     {return(move & END_SQ_MASK) >> END_SQ_SHIFT;}

    constexpr int GetStartIndex(const Move move)      {return move & START_SQ_MASK;}

    template<bool is_white>
    constexpr Move SetColour(const Move move){ return is_white ?((move & ~COLOUR_MASK) | 1 << COLOUR_SHIFT) : (move & ~COLOUR_MASK);}
    
    template<bool is_white, PieceType type>
    constexpr Move SetPieceTypeAndColour(const Move move)
    {
        return is_white ? ((move & (~PIECE_TYPE_MASK & ~COLOUR_MASK)) | (type << PIECE_TYPE_SHIFT) | 1 << COLOUR_SHIFT) : ((move & (~PIECE_TYPE_MASK & ~COLOUR_MASK)) | (type << PIECE_TYPE_SHIFT));
    }
}



#endif // #ifndef MOVE_HPP