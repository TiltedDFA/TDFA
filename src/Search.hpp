#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "MoveGen.hpp"
#include "TranspositionTable.hpp"
#include "Timer.hpp"
#include "Util.hpp"
#include <algorithm>
#include <limits>
#include <cstring>
#include <cmath>

static constexpr int MAX_DEPTH = 128;

class Search
{
public:
    Search() { Clear(); }

    void Clear()
    {
        std::memset(killers_, 0, sizeof(killers_));
        std::memset(pv_table_, 0, sizeof(pv_table_));
        std::memset(pv_length_, 0, sizeof(pv_length_));
        std::memset(history_, 0, sizeof(history_));
        std::memset(countermoves_, 0, sizeof(countermoves_));
        std::memset(static_evals_, 0, sizeof(static_evals_));
        std::memset(ply_moves_, 0, sizeof(ply_moves_));
        nodes_ = 0;
    }

    Score GoSearch(TransposTable* __restrict__ tt, Position* __restrict__ pos,
                   int depth, int ply, TimeManager* tm,
                   Score alpha = Eval::NEG_INF, Score beta = Eval::POS_INF,
                   Move excluded = Moves::NULL_MOVE);

    Score QSearch(Position* __restrict__ pos, TimeManager* tm,
                  Score alpha, Score beta, int ply);

    Move FindBestMove(Position* __restrict__ pos, TransposTable* __restrict__ tt,
                      TimeManager* __restrict__ tm);

private:
    Move killers_[MAX_DEPTH][2];
    Move pv_table_[MAX_DEPTH][MAX_DEPTH];
    int pv_length_[MAX_DEPTH];
    int history_[2][64][64];          // [colour][from][to]
    Move countermoves_[64][64];       // [prev_from][prev_to] = best response
    Score static_evals_[MAX_DEPTH];   // for improving heuristic
    Move ply_moves_[MAX_DEPTH];       // move played at each ply (for countermove)
    U64 nodes_;
};

#endif // #ifndef SEARCH_HPP
