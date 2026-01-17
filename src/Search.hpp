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

    Search():nodes_(0), stop_(false){}

    Score GoSearch(TransposTable* __restrict__  tt, Position* __restrict__  pos, U16 depth, TimeManager const* tm, Score a = Eval::NEG_INF, Score b = Eval::POS_INF, U16 ply = 0);

    Move FindBestMove(Position* __restrict__  pos, TransposTable* __restrict__  tt, TimeManager const* __restrict__ tm);
private:
    Score Quiescence(Position* __restrict__ pos, TimeManager const* tm, Score alpha, Score beta, U16 ply);
    void SortMoves(MoveList* moves, Position* pos, Move tt_move, U16 ply);
    Move killer_moves_[MAX_PLY][2]{};
    U64 nodes_;
    int history_[2][64][64]{};
    bool stop_;
};



#endif // #ifndef SEARCH_HPP
