#include "Search.hpp"

#define LOW_PRIORITY_MVV_LVA 20
constexpr U64 TIME_CHECK_MASK = 0x3FF;
constexpr Score ASPIRATION_WINDOW = 50;
constexpr Score MATE_SCORE = Score(Eval::POS_INF - 100);
constexpr int HISTORY_MAX = 200000;
constexpr int SCORE_TT_MOVE = 1000000;
constexpr int SCORE_CAPTURE_BASE = 900000;
constexpr int SCORE_KILLER_1 = 800000;
constexpr int SCORE_KILLER_2 = 790000;
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
    auto& ml_data = moves->all();
    std::pair<int, Move> scored_moves[MAX_MOVES];
    const Colour us = pos->ColourToMove();
    for (size_t i = 0; i < moves->len(); ++i)
    {
        Sq start_sq;
        Sq target_sq;
        MoveType mt;
        Moves::DecodeMove(ml_data _AT(i), &start_sq, &target_sq, &mt);
        scored_moves[i].second = ml_data[i];
        if (ml_data[i] == tt_move)
        {
            scored_moves[i].first = SCORE_TT_MOVE;
        }
        else if (mt == mt_Capture || mt == mt_EnPassant || pos->PieceOn(target_sq) != p_None)
        {
            const PieceType victim = (mt == mt_EnPassant) ? pt_Pawn : Magics::TypeOf(pos->PieceOn(target_sq));
            const int mvv_lva = GetExchangeValue(victim, Magics::TypeOf(pos->PieceOn(start_sq)));
            scored_moves[i].first = SCORE_CAPTURE_BASE + (50 - mvv_lva);
        }
        else if (ply < MAX_PLY && ml_data[i] == killer_moves_[ply][0])
        {
            scored_moves[i].first = SCORE_KILLER_1;
        }
        else if (ply < MAX_PLY && ml_data[i] == killer_moves_[ply][1])
        {
            scored_moves[i].first = SCORE_KILLER_2;
        }
        else
        {
            scored_moves[i].first = history_[us][start_sq][target_sq];
        }
    }
    std::sort(scored_moves, scored_moves + moves->len(), [](std::pair<int, Move> const& a, std::pair<int, Move> const& b) {return a.first > b.first;});
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
    if(pos->HalfMoves() >= 50) return 0;
    if(depth == 0) return Quiescence(pos, tm, alpha, beta, ply);
    const Colour us = pos->ColourToMove();
    const bool in_check = (us == White) ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos);
    const bool is_pv = (beta - alpha > 1);
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

    if(!in_check && !is_pv && depth > 2)
    {
        const BitBoard non_pawn = pos->Pieces(us, pt_Knight, pt_Bishop, pt_Rook, pt_Queen);
        if(non_pawn)
        {
            const U16 reduction = (depth > 6) ? 3 : 2;
            if(depth > reduction + 1)
            {
                pos->MakeNullMove();
                const Score eval = -GoSearch(tt, pos, depth - 1 - reduction, tm, -beta, ClampScore(Score(-beta + 1)), ply + 1);
                pos->UnmakeNullMove();
                if(stop_) return Eval::NEG_INF;
                if(eval >= beta) return beta;
            }
        }
    }

    MoveList list;
    if(us == White)
    {
        MoveGen::GeneratePseudoLegalMoves<White>(pos, &list);
    }
    else
    {
        MoveGen::GeneratePseudoLegalMoves<Black>(pos, &list);
    }
    if(list.len() > 1)
        SortMoves(&list, pos, tt_move, ply);
    
    bool first_legal = true;
    bool legal_found = false;
    int legal_index = 0;
    for(size_t i = 0; i < list.len(); ++i)
    {
        const bool is_noisy = IsNoisyMove(list[i], pos);
        pos->MakeMove(list[i]);

        if(pos->ColourToMove() == White ? MoveGen::InCheck<Black>(pos) : MoveGen::InCheck<White>(pos))
        {
            pos->UnmakeMove(list[i]);
            continue;
        }

        legal_found = true;
        const int move_index = legal_index++;
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
            bool do_lmr = false;
            if(!is_pv && !in_check && depth >= 3 && !is_noisy && move_index >= 4)
            {
                const bool gives_check = (pos->ColourToMove() == White) ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos);
                if(!gives_check)
                    do_lmr = true;
            }
            if(do_lmr)
            {
                const U16 reduction = (depth >= 6 && move_index >= 8) ? 2 : 1;
                eval = -GoSearch(tt, pos, depth - 1 - reduction, tm, -alpha_plus_one, -alpha, ply + 1);
            }
            else
            {
                eval = -GoSearch(tt, pos, depth - 1, tm, -alpha_plus_one, -alpha, ply + 1);
            }
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
                    int& h = history_[us][start_sq][target_sq];
                    h += depth * depth;
                    if(h > HISTORY_MAX) h = HISTORY_MAX;
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

    if(!legal_found)
    {
        if(pos->ColourToMove() == White ? MoveGen::InCheck<White>(pos) : MoveGen::InCheck<Black>(pos))
            return MatedIn(ply);
        return 0;
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
    std::fill(&history_[0][0][0], &history_[0][0][0] + (2 * 64 * 64), 0);
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
