#include "Search.hpp"

Score Search::GoSearch(BB::Position& pos, U8 depth, Score a, Score b)
{
    if(depth == 0) return Eval::Evaluate(pos);

    MoveList list;
    if(pos.whites_turn_)
    {
        MoveGen::GeneratePseudoLegalMoves<true>(pos, list);
    }
    else
    {
        MoveGen::GeneratePseudoLegalMoves<false>(pos, list);
    }

    if(list.len() == 0)
    {
        if(pos.whites_turn_ ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos))
            return Eval::NEG_INF;
        return 0;
    }
    
    for(size_t i = 0; i < list.len(); ++i)
    {
        pos.MakeMove(list[i]);

        if(!(pos.whites_turn_ ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
        {
            Score eval = -GoSearch(pos, depth - 1, -b, -a);

            pos.UnmakeMove();

            if(eval >= b) {return b;}

            a = std::max(a, eval);
        }
        else {pos.UnmakeMove();}
    }
    return a;
}
Move Search::FindBestMove(BB::Position& pos)
{
    MoveList ml;

    if(pos.whites_turn_)
    {
        MoveGen::GeneratePseudoLegalMoves<true>(pos, ml);
    }
    else
    {
        MoveGen::GeneratePseudoLegalMoves<false>(pos, ml);
    }

    Move best_move = Moves::NULL_MOVE;
    Score best_eval = Eval::NEG_INF;

    for(size_t i{0}; i < ml.len(); ++i)
    {
        pos.MakeMove(ml[i]);
        if(!(pos.whites_turn_ ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
        {
            if(best_move == Moves::NULL_MOVE) best_move = ml[i];

            const Score eval = -GoSearch(pos, SEARCH_DEPTH);

            if(eval > best_eval)
            {
                best_eval = eval;
                best_move = ml[i];
            }
        }
        pos.UnmakeMove();
    }
    return best_move;
}