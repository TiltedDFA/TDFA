#ifndef MOVE_HPP
#define MOVE_HPP

#include <algorithm>
#include <limits>

#include "Types.hpp"

/*
Move storage: 
from right to left:
6 bits - start index
6 bits - end index
3 bits - piece type. This is also used for promotions
1 bit - unused
E.g. 
 000        | 000000     | 000000
MoveType  end index    start index
*/
namespace Moves
{
    constexpr Move NULL_MOVE = std::numeric_limits<Move>::max();
    constexpr U16 START_SQ_MASK    = 0x0000003F;
    constexpr U16 END_SQ_MASK      = 0x00000FC0;
    constexpr U16 END_SQ_SHIFT     = 6;
    constexpr U16 MOVE_TYPE_SHIFT = 12;

    [[nodiscard, gnu::always_inline]]
    inline constexpr Move EncodeMove(const Sq start_index, const Sq target_index, const MoveType move_type)
    {
        Move move{0};
        move |= start_index & START_SQ_MASK;
        move |= (target_index & START_SQ_MASK) << END_SQ_SHIFT;
        move |= move_type << MOVE_TYPE_SHIFT;
        return move;
    }

    [[nodiscard, gnu::always_inline]]
    constexpr Sq StartSq(const Move move)      {return move & START_SQ_MASK;}

    [[nodiscard, gnu::always_inline]]
    constexpr Sq TargetSq(const Move move)     {return(move & END_SQ_MASK) >> END_SQ_SHIFT;}

    [[nodiscard, gnu::always_inline]]
    constexpr bool IsPromotion(const Move move) {return bool((mt_Promotion << MOVE_TYPE_SHIFT) & move);}

    [[nodiscard, gnu::always_inline]]
    constexpr MoveType GetMoveType(const Move move) { assert(!(move & 0x8000)); return MoveType(move >> MOVE_TYPE_SHIFT);}
}



#endif // #ifndef MOVE_HPP