#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "MoveGen.hpp"
#include "TranspositionTable.hpp"
#include "Timer.hpp"
#include <limits>

inline constexpr U8 SEARCH_DEPTH = 8; 

namespace Search
{
    Score GoSearch(TransposTable* __restrict__  tt, Position* __restrict__  pos, U8 depth, Score a = Eval::NEG_INF, Score b = Eval::POS_INF);

    Move FindBestMove(Position* __restrict__  pos, TransposTable* __restrict__  tt, TimeManager const* __restrict__ tm);
};



#endif // #ifndef SEARCH_HPP