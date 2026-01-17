#include "Search.hpp"

constexpr U8 KILLER_SCORE_1 = 10;
constexpr U8 KILLER_SCORE_2 = 12;
#define LOW_PRIORITY_MVV_LVA 20
constexpr U64 TIME_CHECK_MASK = 0x3FF;
constexpr U8 PieceValVictim(PieceType pt)
{
    switch (pt)
    {
        case pt_Pawn: return 5;
        case pt_Knight: return 4;
        case pt_Bishop: return 3;
        case pt_Rook: return 2;
        case pt_Queen: return 1;
        default: return LOW_PRIORITY_MVV_LVA;
    }
}
constexpr U8 PieceValAttacker(PieceType pt)
{
    return 6 - int(pt);
}
constexpr U8 GetExchangeValue(PieceType atk, PieceType def)
{
    return PieceValVictim(atk)*10 - PieceValVictim(def);
}
void Search::SortMoves(MoveList* moves, Position* pos, Move tt_move, U16 ply)
{
    // auto& ml_data = moves->all();
    // U8 scores[MAX_MOVES];
    // using arr_it = std::array<Move, MAX_MOVES>::iterator;
    // std::fill_n(scores, MAX_MOVES, LOW_PRIORITY_MVV_LVA);
    // for (size_t i = 0; i < moves->len(); ++i)
    // {
    //     Sq start_sq;
    //     Sq target_sq;
    //     PieceType p_type;
    //     Moves::DecodeMove(ml_data _AT(i), &start_sq, &target_sq, &p_type);
    //     if (pos->PieceOn(target_sq) != p_None)
    //     {
    //         scores[i] = GetExchangeValue(Magics::TypeOf(pos->PieceOn(target_sq)), Magics::TypeOf(pos->PieceOn(start_sq)));
    //     }
    // }
    // std::sort(ml_data.begin(), ml_data.begin() + moves->len(),
    //     [&scores, &ml_data](const arr_it a, const arr_it b)
    //     {
    //         return scores[a - ml_data.begin()] < scores[b - ml_data.begin()];
    //     });
    auto& ml_data = moves->all();
    std::pair<U8, Move> scored_moves[MAX_MOVES];
    for (size_t i = 0; i < moves->len(); ++i)
    {
        Sq start_sq;
        Sq target_sq;
        MoveType mt;
        Moves::DecodeMove(ml_data _AT(i), &start_sq, &target_sq, &mt);
        scored_moves[i].second = ml_data[i];
        if (ml_data[i] == tt_move)
        {
            scored_moves[i].first = 0;
        }
        else if (mt == mt_Capture || mt == mt_EnPassant || pos->PieceOn(target_sq) != p_None)
        {
            const PieceType victim = (mt == mt_EnPassant) ? pt_Pawn : Magics::TypeOf(pos->PieceOn(target_sq));
            scored_moves[i].first = GetExchangeValue(victim, Magics::TypeOf(pos->PieceOn(start_sq)));
        }
        else if (ply < MAX_PLY && ml_data[i] == killer_moves_[ply][0])
        {
            scored_moves[i].first = KILLER_SCORE_1;
        }
        else if (ply < MAX_PLY && ml_data[i] == killer_moves_[ply][1])
        {
            scored_moves[i].first = KILLER_SCORE_2;
        }
        else
        {
            scored_moves[i].first = LOW_PRIORITY_MVV_LVA;
        }
    }
    std::sort(scored_moves, scored_moves + moves->len(), [](std::pair<U8, Move> const& a, std::pair<U8, Move> const& b) {return a.first < b.first;});
    for (size_t i = 0; i < moves->len(); ++i)
    {
        ml_data[i] = scored_moves[i].second;
    }
}
Score Search::GoSearch(TransposTable* tt, Position* pos, const U16 depth, TimeManager const* tm, Score alpha, Score beta, U16 ply)
{
    if((++nodes_ & TIME_CHECK_MASK) == 0 && tm->OutOfTime()) return Eval::NEG_INF;
    if(depth == 0) return Eval::Evaluate(pos);
    Move best_move = Moves::NULL_MOVE;
    Move tt_move = Moves::NULL_MOVE;
    #if USE_TRANSPOSITION_TABLE == 1
    BoundType hash_entry_flag = BoundType::UPPER_BOUND;

    //tt.probe() will set entry to nullptr if not found
    if(HashEntry const* entry; (entry = tt->Probe(pos->ZKey())))
    {
        tt_move = entry->best_;
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
    if(list.len() > 1)
        SortMoves(&list, pos, tt_move, ply);
    if(list.len() == 0 || pos->HalfMoves() >= 50)
    {
        if(pos->ColourToMove() == White ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos))
            return Eval::NEG_INF;
        return 0;
    }
    
    for(size_t i = 0; i < list.len(); ++i)
    {
        pos->MakeMove(list[i]);

        const Score eval = -GoSearch(tt, pos, depth - 1, tm, -beta, -alpha, ply + 1);

        pos->UnmakeMove(list[i]);

        if (eval >= beta)
        {
            #if USE_TRANSPOSITION_TABLE == 1
            tt->Store(pos->ZKey(), eval, list[i], depth, BoundType::LOWER_BOUND);
            #endif
            if(ply < MAX_PLY)
            {
                Sq start_sq;
                Sq target_sq;
                MoveType mt;
                Moves::DecodeMove(list[i], &start_sq, &target_sq, &mt);
                if(mt == mt_Quiet)
                {
                    if(killer_moves_[ply][0] != list[i])
                    {
                        killer_moves_[ply][1] = killer_moves_[ply][0];
                        killer_moves_[ply][0] = list[i];
                    }
                }
            }
            return beta;
        }

        if (eval > alpha)
        {
            #if USE_TRANSPOSITION_TABLE == 1
            hash_entry_flag = BoundType::EXACT_VAL;
            #endif
            alpha = eval;
            best_move = list[i];
        }

    }

    #if USE_TRANSPOSITION_TABLE == 1
    tt->Store(pos->ZKey(), alpha, best_move, depth, hash_entry_flag);
    #endif

    return alpha;
}
Move Search::FindBestMove(Position* pos, TransposTable* tt, TimeManager const* tm)
{
    nodes_ = 0;
    std::fill(&killer_moves_[0][0], &killer_moves_[0][0] + (MAX_PLY * 2), Moves::NULL_MOVE);
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
        if(ml.len() > 1)
            SortMoves(&ml, pos, last_best_move, 0);

        for(size_t i{0}; i < ml.len(); ++i)
        {
            if(tm->OutOfTime() || last_best_eval == Eval::POS_INF)
//            if(0)
            {
                std::cout << "info score cp " << last_best_eval << " depth " << depth << std::endl;
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

                const Score eval = -GoSearch(tt, pos, depth, tm, Eval::NEG_INF, Eval::POS_INF, 1);

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
        std::cout << "info score cp " << last_best_eval << " depth " << depth << std::endl;
    }
}
