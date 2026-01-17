#include "Search.hpp"

constexpr U8 KILLER_SCORE_1 = 10;
constexpr U8 KILLER_SCORE_2 = 12;
#define LOW_PRIORITY_MVV_LVA 20
constexpr U64 TIME_CHECK_MASK = 0x3FF;
constexpr Score ASPIRATION_WINDOW = 50;
constexpr Score MATE_SCORE = Score(Eval::POS_INF - 100);
static inline bool IsNoisyMove(Move m, Position const* pos)
{
    const Sq target_sq = Moves::TargetSq(m);
    const MoveType mt = MoveType(m >> Moves::PIECE_TYPE_SHIFT);
    if(mt == mt_Capture || mt == mt_EnPassant || Moves::IsPromotionMove(m))
        return true;
    return pos->PieceOn(target_sq) != p_None;
}

static inline Score ClampScore(Score v)
{
    if(v > Eval::POS_INF) return Eval::POS_INF;
    if(v < Eval::NEG_INF) return Eval::NEG_INF;
    return v;
}
static inline Score MatedIn(U16 ply)
{
    return Score(-MATE_SCORE + ply);
}
static inline Score ToTTScore(Score s, U16 ply)
{
    if(s >= Score(MATE_SCORE - MAX_PLY)) return Score(s + ply);
    if(s <= Score(-MATE_SCORE + MAX_PLY)) return Score(s - ply);
    return s;
}
static inline Score FromTTScore(Score s, U16 ply)
{
    if(s >= Score(MATE_SCORE - MAX_PLY)) return Score(s - ply);
    if(s <= Score(-MATE_SCORE + MAX_PLY)) return Score(s + ply);
    return s;
}
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
Score Search::Quiescence(Position* __restrict__ pos, TimeManager const* tm, Score alpha, Score beta, U16 ply)
{
    if(stop_) return Eval::NEG_INF;
    if((++nodes_ & TIME_CHECK_MASK) == 0 && tm->OutOfTime())
    {
        stop_ = true;
        return Eval::NEG_INF;
    }

    const bool in_check = (pos->ColourToMove() == White) ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos);
    if(!in_check)
    {
        const Score stand_pat = Eval::Evaluate(pos);
        if(stand_pat >= beta) return beta;
        if(stand_pat > alpha) alpha = stand_pat;
    }

    MoveList list;
    if(pos->ColourToMove() == White)
    {
        MoveGen::GeneratePseudoLegalMoves<White>(pos, &list);
    }
    else
    {
        MoveGen::GeneratePseudoLegalMoves<Black>(pos, &list);
    }
    if(list.len() > 1)
        SortMoves(&list, pos, Moves::NULL_MOVE, ply);

    bool legal_found = false;
    for(size_t i = 0; i < list.len(); ++i)
    {
        if(!in_check && !IsNoisyMove(list[i], pos))
            continue;

        pos->MakeMove(list[i]);

        if(!(pos->ColourToMove() == White ? MoveGen::InCheck<Black>(pos) : MoveGen::InCheck<White>(pos)))
        {
            legal_found = true;
            const Score eval = -Quiescence(pos, tm, -beta, -alpha, ply + 1);
            if(stop_)
            {
                pos->UnmakeMove(list[i]);
                return Eval::NEG_INF;
            }
            pos->UnmakeMove(list[i]);

            if(eval >= beta) return beta;
            if(eval > alpha) alpha = eval;
        }
        else
        {
            pos->UnmakeMove(list[i]);
        }
    }

    if(in_check && !legal_found)
        return MatedIn(ply);
    return alpha;
}
Score Search::GoSearch(TransposTable* tt, Position* pos, const U16 depth, TimeManager const* tm, Score alpha, Score beta, U16 ply)
{
    if(stop_) return Eval::NEG_INF;
    if((++nodes_ & TIME_CHECK_MASK) == 0 && tm->OutOfTime())
    {
        stop_ = true;
        return Eval::NEG_INF;
    }
    if(depth == 0) return Quiescence(pos, tm, alpha, beta, ply);
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
            const Score tt_eval = FromTTScore(entry->eval_, ply);
            if(entry->bound_ == BoundType::EXACT_VAL)
                return tt_eval;
            if(entry->bound_ == BoundType::UPPER_BOUND && tt_eval <= alpha)
                return alpha;
            if(entry->bound_ == BoundType::LOWER_BOUND && tt_eval >= beta)
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
            return MatedIn(ply);
        return 0;
    }
    
    bool first_legal = true;
    for(size_t i = 0; i < list.len(); ++i)
    {
        pos->MakeMove(list[i]);

        if(best_move == Moves::NULL_MOVE)
            best_move = list[i];

        Score eval;
        if(first_legal)
        {
            eval = -GoSearch(tt, pos, depth - 1, tm, -beta, -alpha, ply + 1);
            first_legal = false;
        }
        else
        {
            const Score alpha_plus_one = ClampScore(Score(alpha + 1));
            eval = -GoSearch(tt, pos, depth - 1, tm, -alpha_plus_one, -alpha, ply + 1);
            if(eval > alpha && eval < beta)
            {
                eval = -GoSearch(tt, pos, depth - 1, tm, -beta, -alpha, ply + 1);
            }
        }

        if(stop_)
        {
            pos->UnmakeMove(list[i]);
            return Eval::NEG_INF;
        }
        pos->UnmakeMove(list[i]);

        if (eval >= beta)
        {
            #if USE_TRANSPOSITION_TABLE == 1
            tt->Store(pos->ZKey(), ToTTScore(eval, ply), list[i], depth, BoundType::LOWER_BOUND);
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
    tt->Store(pos->ZKey(), ToTTScore(alpha, ply), best_move, depth, hash_entry_flag);
    #endif

    return alpha;
}
Move Search::FindBestMove(Position* pos, TransposTable* tt, TimeManager const* tm)
{
    nodes_ = 0;
    stop_ = false;
    std::fill(&killer_moves_[0][0], &killer_moves_[0][0] + (MAX_PLY * 2), Moves::NULL_MOVE);
    Move last_best_move = Moves::NULL_MOVE;
    Score last_best_eval = Eval::NEG_INF;

    for(U16 depth{1};;++depth)
    {
        Score alpha = Eval::NEG_INF;
        Score beta = Eval::POS_INF;
        bool use_aspiration = (last_best_move != Moves::NULL_MOVE);
        if(use_aspiration)
        {
            alpha = ClampScore(Score(last_best_eval - ASPIRATION_WINDOW));
            beta  = ClampScore(Score(last_best_eval + ASPIRATION_WINDOW));
        }

        for(;;)
        {
            MoveList ml;
            Score best_eval = Eval::NEG_INF;
            Move best_move = Moves::NULL_MOVE;
            bool legal_found = false;
            bool first_legal = true;

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

            Score a = alpha;
            Score b = beta;

            for(size_t i{0}; i < ml.len(); ++i)
            {
                if(last_best_eval == Eval::POS_INF)
                {
                    std::cout << "info score cp " << last_best_eval << " depth " << depth << std::endl;
                    #ifdef TDFA_DEBUG
                    Debug::PrintEncodedMoveStr(best_move);
                    #endif
                    return last_best_move;
                }
                if(stop_ || tm->OutOfTime())
                {
                    stop_ = true;
                    break;
                }
                pos->MakeMove(ml[i]);
                if(!(pos->ColourToMove() == White ? MoveGen::InCheck<Black>(pos) : MoveGen::InCheck<White>(pos)))
                {
                    legal_found = true;
                    if(best_move == Moves::NULL_MOVE)
                        best_move = ml[i];
                    Score eval;
                    const U16 child_depth = depth - 1;
                    if(first_legal)
                    {
                        eval = -GoSearch(tt, pos, child_depth, tm, -b, -a, 1);
                        first_legal = false;
                    }
                    else
                    {
                        const Score alpha_plus_one = ClampScore(Score(a + 1));
                        eval = -GoSearch(tt, pos, child_depth, tm, -alpha_plus_one, -a, 1);
                        if(eval > a && eval < b)
                        {
                            eval = -GoSearch(tt, pos, child_depth, tm, -b, -a, 1);
                        }
                    }

                    if(stop_)
                    {
                        pos->UnmakeMove(ml[i]);
                        break;
                    }
                    if(eval > best_eval)
                    {
                        best_eval = eval;
                        best_move = ml[i];
                    }
                    if(eval > a)
                    {
                        a = eval;
                    }
                    if(a >= b)
                    {
                        pos->UnmakeMove(ml[i]);
                        break;
                    }
                }
                pos->UnmakeMove(ml[i]);
            }

            if(stop_)
            {
                return last_best_move;
            }
            if(!legal_found)
            {
                if(pos->ColourToMove() == White ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos))
                    best_eval = MatedIn(0);
                else
                    best_eval = 0;
                last_best_eval = best_eval;
                last_best_move = Moves::NULL_MOVE;
                std::cout << "info score cp " << last_best_eval << " depth " << depth << std::endl;
                return last_best_move;
            }


            if(use_aspiration && (best_eval <= alpha || best_eval >= beta))
            {
                alpha = Eval::NEG_INF;
                beta  = Eval::POS_INF;
                use_aspiration = false;
                continue;
            }

            last_best_eval = best_eval;
            last_best_move = best_move;
            std::cout << "info score cp " << last_best_eval << " depth " << depth << std::endl;
            break;
        }
    }
}
