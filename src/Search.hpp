#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "MoveGen.hpp"
#include "TranspositionTable.hpp"
#include "Timer.hpp"
#include <algorithm>
#include <limits>



class Search
{
public:

    Score GoSearch(TransposTable* __restrict__  tt, Position* __restrict__  pos, U16 depth, TimeManager const* tm, Score a = Eval::NEG_INF, Score b = Eval::POS_INF);

    Move FindBestMove(Position* __restrict__  pos, TransposTable* __restrict__  tt, TimeManager const* __restrict__ tm);
private:
};



#endif // #ifndef SEARCH_HPP