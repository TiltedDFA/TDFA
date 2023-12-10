#include "Search.hpp"

Score Search::GoSearch(uint16_t depth)
{
    if(depth == 0) return Evaluate(this->pos_);

    MoveList list;
    if(this->pos_.whites_turn_)
    {
        generator_.GeneratePseudoLegalMoves<true>(this->pos_, list);
    }
    else
    {
        generator_.GeneratePseudoLegalMoves<false>(this->pos_, list);
    }

    Score max = NEG_INF;
    
    for(size_t i = 0; i < list.len(); ++i)
    {
        this->pos_.MakeMove(list[i]);

        if(!(pos.whites_turn_ ? MoveGen::InCheck<false>(this->pos_) : MoveGen::InCheck<true>(this->pos_)))
        {
            Score eval = -GoSearch(depth-1);

            if(eval > max) max = eval;
        }

        this->pos_.UnmakeMove();
    }
    return max;
}