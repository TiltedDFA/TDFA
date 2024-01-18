#include "Search.hpp"

Score Search::GoSearch(TransposTable& tt, BB::Position& pos, U8 depth, Score alpha, Score beta)
{
    if(depth == 0) return Eval::Evaluate(pos);

    #if USE_TRANSPOSITION_TABLE == 1
    BoundType hash_entry_flag = BoundType::ALPHA;

    //tt.probe() will set entry to nullptr if not found
    if(HashEntry const* entry; entry = tt.Probe(pos.postion_key_))
    {
        if(entry->key_ == pos.postion_key_ && entry->depth_ >= depth)
        {
            if(entry->bound_ == BoundType::EXACT_VAL)
                return entry->eval_;
            if(entry->bound_ == BoundType::ALPHA && entry->eval_ <= alpha)
                return alpha;
            if(entry->bound_ == BoundType::BETA && entry->eval_ >= beta)
                return beta;
        }
    }
    #endif

    MoveList list;
    if(pos.whites_turn_)
    {
        MoveGen::GeneratePseudoLegalMoves<true>(pos, list);
    }
    else
    {
        MoveGen::GeneratePseudoLegalMoves<false>(pos, list);
    }

    if(list.len() == 0 || pos.info_.half_moves_ >= 50)
    {
        if(pos.whites_turn_ ? MoveGen::InCheck<true>(pos) : MoveGen::InCheck<false>(pos))
            return Eval::NEG_INF;
        return 0;
    }
    
    for(size_t i = 0; i < list.len(); ++i)
    {
        pos.MakeMove(list[i]);

        if(!(pos.whites_turn_ ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
        {
            Score eval = -GoSearch(tt, pos, depth - 1, -beta, -alpha);

            pos.UnmakeMove();

            if(eval >= beta)
            {
                #if USE_TRANSPOSITION_TABLE == 1
                tt.Store(pos.postion_key_, eval, Moves::NULL_MOVE, depth, BoundType::BETA);
                #endif
                return beta;
            }

            if(eval > alpha)
            {
                #if USE_TRANSPOSITION_TABLE == 1
                hash_entry_flag = BoundType::EXACT_VAL;
                #endif
                alpha = eval;
            }
        }
        else {pos.UnmakeMove();}
    }

    #if USE_TRANSPOSITION_TABLE == 1
    tt.Store(pos.postion_key_, alpha, Moves::NULL_MOVE, depth, hash_entry_flag);
    #endif

    return alpha;
}
Move Search::FindBestMove(BB::Position& pos, TransposTable& tt, TimeManager& tm)
{
    Move last_best_move = Moves::NULL_MOVE;
    Score last_best_eval = Eval::NEG_INF;

    for(U8 depth{1};;++depth)
    {
        MoveList ml;

        Score best_eval = Eval::NEG_INF;
        Move best_move = Moves::NULL_MOVE;
        
        if(pos.whites_turn_)
        {
            MoveGen::GeneratePseudoLegalMoves<true>(pos, ml);
        }
        else
        {
            MoveGen::GeneratePseudoLegalMoves<false>(pos, ml);
        }

        for(size_t i{0}; i < ml.len(); ++i)
        {
            if(tm.OutOfTime()) return last_best_move;
            pos.MakeMove(ml[i]);
            if(!(pos.whites_turn_ ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
            {
                if(best_move == Moves::NULL_MOVE) best_move = ml[i];

                const Score eval = -GoSearch(tt, pos, depth);

                if(eval > best_eval)
                {
                    best_eval = eval;
                    best_move = ml[i];
                }
            }
            pos.UnmakeMove();
        }
        last_best_eval = best_eval;
        last_best_move = best_move;
        std::cout << std::format("info score cp {} depth {}\n", last_best_eval, depth);
    }
}