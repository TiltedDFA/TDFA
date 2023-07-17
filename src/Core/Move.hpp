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
    [[nodiscard]] constexpr Move EncodeMove(const int start_index, const int target_index,const int piece_type,const bool is_white)
    {
        Move move{0};
        move |= start_index & Magics::START_SQ_MASK;
        move |= (target_index & Magics::START_SQ_MASK) << Magics::END_SQ_SHIFT;
        move |= (piece_type << Magics::PIECE_TYPE_SHIFT) & Magics::PIECE_TYPE_MASK ;
        move |= (is_white ? 1u : 0u) << Magics::COLOUR_SHIFT;
        return move;
    }
    constexpr void DecodeMove(const Move move,int& start_index, int& target_index,int& piece_type,bool& is_white)
    {
        start_index = move & Magics::START_SQ_MASK;
        target_index =  (move & Magics::END_SQ_MASK) >> Magics::END_SQ_SHIFT;
        piece_type = (move & Magics::PIECE_TYPE_MASK) >> Magics::PIECE_TYPE_SHIFT;
        is_white = (move >> Magics::COLOUR_SHIFT) == 1;
    }
    constexpr bool IsWhite(const Move move)            {return (move >> Magics::COLOUR_SHIFT) == 1;}
    constexpr PieceType GetPieceType(const Move move) {return (move & Magics::PIECE_TYPE_MASK) >> Magics::PIECE_TYPE_SHIFT;}
    constexpr int GetTargetIndex(const Move move)     {return(move & Magics::END_SQ_MASK) >> Magics::END_SQ_SHIFT;}
    constexpr int GetStartIndex(const Move move)      {return move & Magics::START_SQ_MASK;}
}



#endif // #ifndef MOVE_HPP