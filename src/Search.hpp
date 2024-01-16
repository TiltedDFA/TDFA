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
    Score GoSearch(TransposTable& tt, BB::Position& pos, U8 depth, Score a = Eval::NEG_INF, Score b = Eval::POS_INF);

    Move FindBestMove(BB::Position& pos, TransposTable& tt, TimeManager& tm);
};



#endif // #ifndef SEARCH_HPP