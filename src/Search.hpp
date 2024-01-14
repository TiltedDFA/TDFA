#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "MoveGen.hpp"
#include <limits>

inline constexpr U8 SEARCH_DEPTH = 6; 

namespace Search
{
    Score GoSearch(BB::Position& pos, U8 depth, Score a = Eval::NEG_INF, Score b = Eval::POS_INF);

    Move FindBestMove(BB::Position& pos);
};



#endif // #ifndef SEARCH_HPP