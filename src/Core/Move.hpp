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
    constexpr PieceType KING            = 0x01u;
    constexpr PieceType QUEEN           = 0x02u;
    constexpr PieceType BISHOP          = 0x03u;
    constexpr PieceType KNIGHT          = 0x04u;
    constexpr PieceType ROOK            = 0x05u;
    constexpr PieceType PAWN            = 0x06u;
    constexpr uint32_t START_SQ_MASK    = 0x0000003F;
    constexpr uint32_t END_SQ_MASK      = 0x00000FC0;
    constexpr uint32_t PIECE_TYPE_MASK  = 0x00007000;
    constexpr uint32_t COLOUR_MASK      = 0x00008000;
    constexpr uint16_t END_SQ_SHIFT     = 6;
    constexpr uint16_t PIECE_TYPE_SHIFT = 12;
    constexpr uint16_t COLOUR_SHIFT     = 15;
    [[nodiscard]] constexpr Move EncodeMove(const int start_index, const int target_index,const int piece_type,const bool is_white)
    {
        Move move{0};
        move |= start_index & START_SQ_MASK;
        move |= (target_index & START_SQ_MASK) << END_SQ_SHIFT;
        move |= (piece_type << PIECE_TYPE_SHIFT) & PIECE_TYPE_MASK ;
        move |= (is_white ? 1u : 0u) << COLOUR_SHIFT;
        return move;
    }
    constexpr void DecodeMove(const Move move,int& start_index, int& target_index,int& piece_type,bool& is_white)
    {
        start_index = move & START_SQ_MASK;
        target_index =  (move & END_SQ_MASK) >> END_SQ_SHIFT;
        piece_type = (move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT;
        is_white = (move >> COLOUR_SHIFT) == 1;
    }
    constexpr bool IsWhite(const Move move)            {return (move >> COLOUR_SHIFT) == 1;}
    constexpr PieceType GetPieceType(const Move move) {return (move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT;}
    constexpr int GetTargetIndex(const Move move)     {return(move & END_SQ_MASK) >> END_SQ_SHIFT;}
    constexpr int GetStartIndex(const Move move)      {return move & START_SQ_MASK;}
    template<bool is_white>
    constexpr Move SetPieceTypeAndColour(const Move move, const PieceType type)
    {
        return is_white ? ((move & (~PIECE_TYPE_MASK | ~COLOUR_MASK)) | (type << PIECE_TYPE_SHIFT) | 1 << PIECE_TYPE_SHIFT) : ((move & (~PIECE_TYPE_MASK | ~COLOUR_MASK)) | (type << PIECE_TYPE_SHIFT));
    }
    template<bool is_white>
    constexpr Move SetColour(const Move move){ return is_white ? ((move & ~PIECE_TYPE_MASK) | 1 << PIECE_TYPE_SHIFT) : (move & ~PIECE_TYPE_MASK);}
}



#endif // #ifndef MOVE_HPP