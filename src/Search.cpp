#include "Search.hpp"

Search::Search(BB::Position& pos):pos_(pos),generator_(pos_){}

void Search::SetPos(BB::Position& pos)
{
    pos_ = pos;
}
Score Search::GoSearch(U16 depth, Score a, Score b)
{
    ++nodes_;
    if(depth == 0) return Eval::Evaluate(this->pos_);

    MoveList list;
    if(this->pos_.whites_turn_)
    {
        generator_.GeneratePseudoLegalMoves<true>(this->pos_, list);
    }
    else
    {
        generator_.GeneratePseudoLegalMoves<false>(this->pos_, list);
    }

    if(list.len() == 0)
    {
        if(this->pos_.whites_turn_ ? MoveGen::InCheck<false>(pos_) : MoveGen::InCheck<true>(pos_))
            return Eval::NEG_INF;
        return 0;
    }
    
    for(size_t i = 0; i < list.len(); ++i)
    {
        this->pos_.MakeMove(list[i]);

        if(!(this->pos_.whites_turn_ ? MoveGen::InCheck<false>(this->pos_) : MoveGen::InCheck<true>(this->pos_)))
        {
            Score eval = -GoSearch(depth - 1, -b, -a);

            this->pos_.UnmakeMove();

            if(eval >= b) {return b;}

            a = std::max(a, eval);
        }
        else {this->pos_.UnmakeMove();}
    }
    return a;
}