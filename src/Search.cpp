#include "Search.hpp"

Score Search::GoSearch(TransposTable* tt, Position* pos, const U16 depth, Score alpha, Score beta)
{
    if(depth == 0) return Eval::Evaluate(pos);

    #if USE_TRANSPOSITION_TABLE == 1
    BoundType hash_entry_flag = BoundType::UPPER_BOUND;

    //tt.probe() will set entry to nullptr if not found
    if(HashEntry const* entry; (entry = tt->Probe(pos->ZKey())))
    {
        if(entry->key_ == pos->ZKey() && entry->depth_ >= depth)
        {
            if(entry->bound_ == BoundType::EXACT_VAL)
                return entry->eval_;
            if(entry->bound_ == BoundType::UPPER_BOUND && entry->eval_ <= alpha)
                return alpha;
            if(entry->bound_ == BoundType::LOWER_BOUND && entry->eval_ >= beta)
                return beta;
        }
    }
    #endif

    MoveList list;
    if(pos->ColourToMove() == White)
    {
        MoveGen::GenerateLegalMoves<White>(pos, &list);
    }
    else
    {
        MoveGen::GenerateLegalMoves<Black>(pos, &list);
    }

    if(list.len() == 0 || pos->HalfMoves() >= 50)
    {
        if(pos->ColourToMove() == White ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos))
            return Eval::NEG_INF;
        return 0;
    }
    
    for(size_t i = 0; i < list.len(); ++i)
    {
        pos->MakeMove(list[i]);

        const Score eval = -GoSearch(tt, pos, depth - 1, -beta, -alpha);

        pos->UnmakeMove(list[i]);

        if (eval >= beta)
        {
            #if USE_TRANSPOSITION_TABLE == 1
            tt->Store(pos->ZKey(), eval, Moves::NULL_MOVE, depth, BoundType::LOWER_BOUND);
            #endif
            return beta;
        }

        if (eval > alpha)
        {
            #if USE_TRANSPOSITION_TABLE == 1
            hash_entry_flag = BoundType::EXACT_VAL;
            #endif
            alpha = eval;
        }

    }

    #if USE_TRANSPOSITION_TABLE == 1
    tt->Store(pos->ZKey(), alpha, Moves::NULL_MOVE, depth, hash_entry_flag);
    #endif

    return alpha;
}
Move Search::FindBestMove(Position* pos, TransposTable* tt, TimeManager const* tm)
{
    Move last_best_move = Moves::NULL_MOVE;
    Score last_best_eval = Eval::NEG_INF;

    for(U16 depth{1};;++depth)
    {
        MoveList ml;

        Score best_eval = Eval::NEG_INF;
        Move best_move = Moves::NULL_MOVE;

        if(pos->ColourToMove() == White)
        {
            MoveGen::GeneratePseudoLegalMoves<White>(pos, &ml);
        }
        else
        {
            MoveGen::GeneratePseudoLegalMoves<Black>(pos, &ml);
        }

        for(size_t i{0}; i < ml.len(); ++i)
        {
            if(tm->OutOfTime() || last_best_eval == Eval::POS_INF || last_best_move == Eval::NEG_INF)
//            if(0)
            {
                std::cout << std::format("info score cp {} depth {}", last_best_eval, depth) << std::endl;
                #ifdef TDFA_DEBUG
                Debug::PrintEncodedMoveStr(best_move);
                #endif
                return last_best_move;
            }
            pos->MakeMove(ml[i]);
            if(!(pos->ColourToMove() == White ? MoveGen::InCheck<Black>(pos) : MoveGen::InCheck<White>(pos)))
            {
                //init best move with first legal
                if(best_move == Moves::NULL_MOVE) best_move = ml[i];

                const Score eval = -GoSearch(tt, pos, depth);

                if(eval > best_eval)
                {
                    best_eval = eval;
                    best_move = ml[i];
                }
            }
            pos->UnmakeMove(ml[i]);
        }
        last_best_eval = best_eval;
        last_best_move = best_move;
        std::cout << std::format("info score cp {} depth {}", last_best_eval, depth) << std::endl;
    }
}