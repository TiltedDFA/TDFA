#ifndef MOVE_HPP
#define MOVE_HPP
#include <cstdint>
#include <algorithm>
#include <limits>

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
    constexpr U32 START_SQ_MASK    = 0x0000003F;
    constexpr U32 END_SQ_MASK      = 0x00000FC0;
    constexpr U32 PIECE_TYPE_MASK  = 0x0000F000;
    constexpr U32 COLOUR_MASK      = 0x00008000;
    constexpr U16 END_SQ_SHIFT     = 6;
    constexpr U16 PIECE_TYPE_SHIFT = 12;

    constexpr PieceType BAD_MOVE = std::numeric_limits<PieceType>::max();
    constexpr Move NULL_MOVE = std::numeric_limits<PieceType>::max();

    constexpr Move EncodeMove(const Sq start_index, const Sq target_index, const PieceType piece_type)
    {
        Move move{0};
        move |= start_index & START_SQ_MASK;
        move |= (target_index & START_SQ_MASK) << END_SQ_SHIFT;
        // move |= (piece_type << PIECE_TYPE_SHIFT) & PIECE_TYPE_MASK;
        move |= piece_type << PIECE_TYPE_SHIFT;
        return move;
    }
    constexpr void DecodeMove(const Move move, Sq* __restrict__ start_index, Sq* __restrict__ target_index, PieceType* __restrict__ piece_type)
    {
        *start_index    = move & START_SQ_MASK;
        *target_index   = (move & END_SQ_MASK) >> END_SQ_SHIFT;
        // *piece_type     = PieceType((move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT);
        *piece_type     = PieceType(move >> PIECE_TYPE_SHIFT);
    }

    constexpr PieceType PType(const Move move) {return PieceType((move & PIECE_TYPE_MASK) >> PIECE_TYPE_SHIFT);}
    
    constexpr U8 TargetSq(const Move move)     {return(move & END_SQ_MASK) >> END_SQ_SHIFT;}

    constexpr U8 StartSq(const Move move)      {return move & START_SQ_MASK;}
    
    constexpr bool IsPromotionMove(const Move move) {return bool(move & 0x8000);}

    constexpr PieceType PTypeOfProm(const Move move) {return PieceType(PType(move) - 7);}
    
    template<PieceType type>
    constexpr Move SetPieceType(const Move move) {return Move(move & ~PIECE_TYPE_MASK) | (type << PIECE_TYPE_SHIFT);}
}



#endif // #ifndef MOVE_HPP