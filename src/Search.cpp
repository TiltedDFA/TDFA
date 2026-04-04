#include "Search.hpp"

// ── Move ordering scores ──────────────────────────────────────────
static constexpr int SCORE_TT_MOVE    = 10'000'000;
static constexpr int SCORE_GOOD_CAP   = 1'000'000;
static constexpr int SCORE_KILLER_1   = 900'000;
static constexpr int SCORE_KILLER_2   = 800'000;
static constexpr int SCORE_COUNTERMV  = 700'000;
static constexpr int MAX_HISTORY      = 16384;

// Piece values for MVV-LVA and simple SEE
// Index by PieceType: King=0, Queen=1, Bishop=2, Knight=3, Rook=4, Pawn=5, All=6
static constexpr int MVV_VAL[]  = { 0, 900, 330, 320, 500, 100, 0 };

// ── LMR reduction table (precomputed) ─────────────────────────────
static int LMR_TABLE[64][64];

static struct LMRInit {
    LMRInit() {
        for (int d = 0; d < 64; ++d)
            for (int m = 0; m < 64; ++m)
                LMR_TABLE[d][m] = (d == 0 || m == 0) ? 0
                    : static_cast<int>(0.75 + std::log(d) * std::log(m) / 2.25);
    }
} lmr_init_;

// ── Late Move Pruning thresholds by depth ─────────────────────────
static constexpr int LMP_THRESHOLD[] = { 0, 5, 8, 13, 20, 30, 42 };

// ── Simple bad capture detection ──────────────────────────────────
// Returns true if capture is likely losing (attacker > victim and pawn-defended)
static bool IsBadCapture(Position const* pos, Sq from, Sq to)
{
    const PieceType attacker = Magics::TypeOf(pos->PieceOn(from));
    const PieceType victim   = Magics::TypeOf(pos->PieceOn(to));

    // Equal or favorable trade — not bad
    if (MVV_VAL[victim] >= MVV_VAL[attacker] - 50) return false;

    // Check if target square is defended by enemy pawn
    const Colour them = Magics::ColourOf(pos->PieceOn(to));
    const BitBoard to_bb = Magics::SqToBB(to);

    BitBoard pawn_defenders;
    if (them == White)
        pawn_defenders = (Magics::Shift<SOUTH_WEST>(to_bb) | Magics::Shift<SOUTH_EAST>(to_bb))
                       & pos->Pieces(White, pt_Pawn);
    else
        pawn_defenders = (Magics::Shift<NORTH_WEST>(to_bb) | Magics::Shift<NORTH_EAST>(to_bb))
                       & pos->Pieces(Black, pt_Pawn);

    return pawn_defenders != 0;
}

// ── Scored move for incremental picking ───────────────────────────
struct ScoredMove {
    Move move;
    int score;
};

// Score all moves for incremental picking
static void ScoreMoves(ScoredMove* scored, MoveList* moves, Position* pos,
                       Move tt_move, const Move killers[2],
                       const int history[64][64], Move countermove)
{
    for (size_t i = 0; i < moves->len(); ++i)
    {
        const Move m = (*moves)[i];
        scored[i].move = m;

        if (m == tt_move) { scored[i].score = SCORE_TT_MOVE; continue; }

        Sq from, to;
        MoveType mt;
        Moves::DecodeMove(m, &from, &to, &mt);

        if (mt == mt_Capture || mt == mt_EnPassant)
        {
            const int victim  = (mt == mt_EnPassant) ? MVV_VAL[pt_Pawn]
                              : MVV_VAL[Magics::TypeOf(pos->PieceOn(to))];
            const int attacker = MVV_VAL[Magics::TypeOf(pos->PieceOn(from))];
            scored[i].score = SCORE_GOOD_CAP + victim * 10 - attacker;
        }
        else if (Moves::IsPromotionMove(m))
        {
            scored[i].score = SCORE_GOOD_CAP + 5000;
        }
        else if (m == killers[0])      { scored[i].score = SCORE_KILLER_1; }
        else if (m == killers[1])      { scored[i].score = SCORE_KILLER_2; }
        else if (m == countermove)     { scored[i].score = SCORE_COUNTERMV; }
        else                           { scored[i].score = history[from][to]; }
    }
}

// Pick the best move from position 'start' onwards and swap it to 'start'
static inline Move PickNext(ScoredMove* scored, size_t start, size_t count)
{
    size_t best_idx = start;
    int best_score = scored[start].score;

    for (size_t i = start + 1; i < count; ++i)
    {
        if (scored[i].score > best_score)
        {
            best_score = scored[i].score;
            best_idx = i;
        }
    }

    if (best_idx != start)
    {
        const ScoredMove tmp = scored[start];
        scored[start] = scored[best_idx];
        scored[best_idx] = tmp;
    }

    return scored[start].move;
}

// ── QSearch capture scoring + picking ─────────────────────────────
// Input: only captures, en passant, and promotions (from GeneratePseudoLegalCaptures)
static void ScoreCaptures(ScoredMove* scored, size_t* n_caps,
                          MoveList* moves, Position* pos)
{
    *n_caps = 0;
    for (size_t i = 0; i < moves->len(); ++i)
    {
        const Move m = (*moves)[i];
        Sq from, to;
        MoveType mt;
        Moves::DecodeMove(m, &from, &to, &mt);

        const bool is_cap = (mt == mt_Capture || mt == mt_EnPassant);
        const bool is_prom = Moves::IsPromotionMove(m);

        // Skip obviously bad captures in QSearch
        if (is_cap && !is_prom && IsBadCapture(pos, from, to)) continue;

        int score = 0;
        if (mt == mt_EnPassant)     score = MVV_VAL[pt_Pawn] * 10;
        else if (is_prom)           score = 9000;
        else                        score = MVV_VAL[Magics::TypeOf(pos->PieceOn(to))] * 10
                                          - MVV_VAL[Magics::TypeOf(pos->PieceOn(from))];

        scored[*n_caps] = {m, score};
        ++(*n_caps);
    }
}

// ── Quiescence search ─────────────────────────────────────────────
Score Search::QSearch(Position* pos, TimeManager* tm,
                      Score alpha, Score beta, int ply)
{
    ++nodes_;

    if ((nodes_ & 4095) == 0 && tm->OutOfTime()) return 0;

    const Score stand_pat = Eval::Evaluate(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    // Delta pruning
    if (stand_pat + Eval::QUEEN_VAL + 200 < alpha)
        return alpha;

    MoveList pseudo;
    if (pos->ColourToMove() == White)
        MoveGen::GeneratePseudoLegalCaptures<White>(pos, &pseudo);
    else
        MoveGen::GeneratePseudoLegalCaptures<Black>(pos, &pseudo);

    ScoredMove caps[MAX_MOVES];
    size_t n_caps = 0;
    ScoreCaptures(caps, &n_caps, &pseudo, pos);

    for (size_t ci = 0; ci < n_caps; ++ci)
    {
        const Move m = PickNext(caps, ci, n_caps);

        pos->MakeMove(m);

        const bool illegal = (pos->ColourToMove() == White)
            ? MoveGen::InCheck<Black>(pos)
            : MoveGen::InCheck<White>(pos);

        if (illegal) { pos->UnmakeMove(m); continue; }

        const Score eval = -QSearch(pos, tm, -beta, -alpha, ply + 1);
        pos->UnmakeMove(m);

        if (eval >= beta) return beta;
        if (eval > alpha) alpha = eval;
    }

    return alpha;
}

// ── Main alpha-beta search ────────────────────────────────────────
Score Search::GoSearch(TransposTable* tt, Position* pos,
                       int depth, int ply, TimeManager* tm,
                       Score alpha, Score beta, Move excluded)
{
    ++nodes_;

    // Draw detection
    if (ply > 0 && (pos->IsRepetition() || pos->HalfMoves() >= 100))
        return 0;

    // Drop to quiescence
    if (depth <= 0) return QSearch(pos, tm, alpha, beta, ply);

    // Time check
    if ((nodes_ & 4095) == 0 && tm->OutOfTime()) return 0;

    const bool is_pv = (beta - alpha) > 1;

    pv_length_[ply] = ply;

    // ── TT probe ──────────────────────────────────────────────────
#if USE_TRANSPOSITION_TABLE == 1
    BoundType hash_entry_flag = BoundType::UPPER_BOUND;
    Move tt_move = Moves::NULL_MOVE;
    Score tt_eval = 0;
    int tt_entry_depth = 0;
    BoundType tt_bound = BoundType::UPPER_BOUND;

    if (HashEntry const* entry; (entry = tt->Probe(pos->ZKey())))
    {
        tt_move = entry->best_;
        tt_eval = entry->eval_;
        tt_entry_depth = entry->depth_;
        tt_bound = entry->bound_;
        if (!is_pv && entry->depth_ >= depth && excluded == Moves::NULL_MOVE)
        {
            if (entry->bound_ == BoundType::EXACT_VAL)
                return entry->eval_;
            if (entry->bound_ == BoundType::UPPER_BOUND && entry->eval_ <= alpha)
                return alpha;
            if (entry->bound_ == BoundType::LOWER_BOUND && entry->eval_ >= beta)
                return beta;
        }
    }
#else
    Move tt_move = Moves::NULL_MOVE;
#endif

    const bool in_check = (pos->ColourToMove() == White)
        ? MoveGen::InCheck<White>(pos)
        : MoveGen::InCheck<Black>(pos);

    // ── Check extension ───────────────────────────────────────────
    if (in_check) ++depth;

    // ── Static eval & improving heuristic ─────────────────────────
    Score static_eval;
    if (in_check)
    {
        static_eval = Eval::NEG_INF;
    }
    else
    {
        static_eval = Eval::Evaluate(pos);
    }
    static_evals_[ply] = static_eval;

    const bool improving = !in_check && ply >= 2
                         && static_eval > static_evals_[ply - 2];

    // ── Internal Iterative Reduction (IIR) ────────────────────────
    if (depth >= 4 && tt_move == Moves::NULL_MOVE && !in_check)
        --depth;

    // ── Razoring ──────────────────────────────────────────────────
    if (!is_pv && !in_check && depth <= 2 && ply > 0)
    {
        const Score razor_margin = 200 + 150 * depth;
        if (static_eval + razor_margin <= alpha)
        {
            const Score razor_eval = QSearch(pos, tm, alpha, beta, ply);
            if (depth == 1 || razor_eval <= alpha)
                return razor_eval;
        }
    }

    // ── Reverse futility pruning ──────────────────────────────────
    if (!is_pv && !in_check && depth <= 6 && ply > 0)
    {
        const Score margin = (improving ? 80 : 60) * depth;
        if (static_eval - margin >= beta)
            return static_eval - margin;
    }

    // ── Null move pruning ─────────────────────────────────────────
    if (!in_check && depth >= 3 && ply > 0 && !is_pv
        && static_eval >= beta)
    {
        const int R = 3 + depth / 6 + std::min((static_eval - beta) / 200, 2);
        pos->MakeNullMove();
        const Score null_eval = -GoSearch(tt, pos, depth - 1 - R, ply + 1, tm, -beta, -beta + 1);
        pos->UnmakeNullMove();
        if (null_eval >= beta)
            return beta;
    }

    // ── Singular extensions ──────────────────────────────────────
#if USE_TRANSPOSITION_TABLE == 1
    bool singular_move = false;
    if (depth >= 8 && excluded == Moves::NULL_MOVE
        && tt_move != Moves::NULL_MOVE && !in_check
        && tt_entry_depth >= depth - 3
        && tt_bound != BoundType::UPPER_BOUND)
    {
        const Score sing_beta = Score(tt_eval - 3 * depth);
        const Score sing_eval = GoSearch(tt, pos, (depth - 1) / 2, ply, tm,
                                         Score(sing_beta - 1), sing_beta, tt_move);
        if (sing_eval < sing_beta)
            singular_move = true;
        else if (sing_eval >= beta)
            return sing_eval; // multicut
    }
#else
    const bool singular_move = false;
#endif

    // ── Futility pruning flag ─────────────────────────────────────
    const int fut_margin = improving ? 150 : 120;
    const bool can_futility = !is_pv && !in_check && depth <= 3 && ply > 0
                            && static_eval + fut_margin * depth <= alpha;

    // ── Generate pseudo-legal moves ───────────────────────────────
    MoveList list;
    if (pos->ColourToMove() == White)
        MoveGen::GeneratePseudoLegalMoves<White>(pos, &list);
    else
        MoveGen::GeneratePseudoLegalMoves<Black>(pos, &list);

    const Colour us = pos->ColourToMove();

    // Countermove: response to the move that was played at ply-1
    Move countermove = Moves::NULL_MOVE;
    if (ply > 0)
    {
        Sq pf, pt; MoveType pmt;
        Moves::DecodeMove(ply_moves_[ply - 1], &pf, &pt, &pmt);
        countermove = countermoves_[pf][pt];
    }

    // Score moves for incremental picking
    ScoredMove scored[MAX_MOVES];
    ScoreMoves(scored, &list, pos, tt_move, killers_[ply], history_[us], countermove);

    Move best_move_in_node = Moves::NULL_MOVE;
    int legal_moves = 0;
    const size_t n_moves = list.len();

    // Track quiet moves searched (for history malus on cutoff)
    Move quiets_searched[64];
    int n_quiets_searched = 0;

    for (size_t i = 0; i < n_moves; ++i)
    {
        const Move m = PickNext(scored, i, n_moves);

        // Skip excluded move (for singular extension search)
        if (m == excluded) continue;

        Sq from, to;
        MoveType mt;
        Moves::DecodeMove(m, &from, &to, &mt);
        const bool is_capture = (mt == mt_Capture || mt == mt_EnPassant);
        const bool is_promotion = Moves::IsPromotionMove(m);
        const bool is_quiet = !is_capture && !is_promotion;

        // ── Late Move Pruning (LMP) — skip before MakeMove ───────
        if (!is_pv && !in_check && is_quiet && depth <= 6
            && legal_moves >= LMP_THRESHOLD[std::min(depth, 6)])
            continue;

        // ── Futility pruning ──────────────────────────────────────
        if (can_futility && legal_moves > 0 && is_quiet)
            continue;

        // ── SEE pruning for bad captures at low depth ─────────────
        bool is_bad_capture = false;
        if (!is_pv && depth <= 4 && legal_moves > 0
            && is_capture && !is_promotion
            && IsBadCapture(pos, from, to))
        {
            if (depth <= 2) continue;  // skip at depth 1-2
            is_bad_capture = true;     // reduce at depth 3-4
        }

        pos->MakeMove(m);

        // TT prefetch: start loading the TT entry for the child position
        // while we check legality
        tt->Prefetch(pos->ZKey());

        // Legality check
        const bool illegal = (pos->ColourToMove() == White)
            ? MoveGen::InCheck<Black>(pos)
            : MoveGen::InCheck<White>(pos);

        if (illegal) { pos->UnmakeMove(m); continue; }

        ++legal_moves;

        // Track this move at the current ply for countermove lookup
        ply_moves_[ply] = m;

        // Track quiet moves for history malus
        if (is_quiet && n_quiets_searched < 64)
            quiets_searched[n_quiets_searched++] = m;

        // ── Extensions ─────────────────────────────────────────��──
        int ext = 0;
        if (m == tt_move && singular_move) ext = 1;

        // ── Late Move Reductions (LMR) ────────────────────────────
        Score eval;
        const int new_depth = depth - 1 + ext;

        if (new_depth >= 2 && legal_moves > 3 && (is_quiet || is_bad_capture))
        {
            int reduction = LMR_TABLE[std::min(depth, 63)][std::min(legal_moves, 63)];

            if (!is_pv) ++reduction;
            if (!improving) ++reduction;
            if (m == killers_[ply][0] || m == killers_[ply][1]) --reduction;
            if (is_bad_capture) ++reduction;

            // History-based: reduce less for good history, more for bad
            if (is_quiet) reduction -= history_[us][from][to] / 8000;

            reduction = std::clamp(reduction, 0, new_depth - 1);

            eval = -GoSearch(tt, pos, new_depth - reduction, ply + 1, tm, -alpha - 1, -alpha);

            if (eval > alpha && reduction > 0)
                eval = -GoSearch(tt, pos, new_depth, ply + 1, tm, -beta, -alpha);
        }
        else if (!is_pv && legal_moves > 1)
        {
            eval = -GoSearch(tt, pos, new_depth, ply + 1, tm, -alpha - 1, -alpha);
            if (eval > alpha && eval < beta)
                eval = -GoSearch(tt, pos, new_depth, ply + 1, tm, -beta, -alpha);
        }
        else
        {
            eval = -GoSearch(tt, pos, new_depth, ply + 1, tm, -beta, -alpha);
        }

        pos->UnmakeMove(m);

        if (eval >= beta)
        {
            if (is_quiet)
            {
                // Killer update
                killers_[ply][1] = killers_[ply][0];
                killers_[ply][0] = m;

                // History bonus with gravity
                int bonus = depth * depth;
                int& h = history_[us][from][to];
                h += bonus - h * bonus / MAX_HISTORY;

                // History MALUS: penalize all other quiets that didn't cut
                for (int qi = 0; qi < n_quiets_searched - 1; ++qi)
                {
                    Sq qf = Moves::StartSq(quiets_searched[qi]);
                    Sq qt = Moves::TargetSq(quiets_searched[qi]);
                    int& hm = history_[us][qf][qt];
                    hm -= bonus + hm * bonus / MAX_HISTORY;
                }

                // Countermove update
                if (ply > 0)
                {
                    Sq pf = Moves::StartSq(ply_moves_[ply - 1]);
                    Sq pt2 = Moves::TargetSq(ply_moves_[ply - 1]);
                    countermoves_[pf][pt2] = m;
                }
            }

#if USE_TRANSPOSITION_TABLE == 1
            tt->Store(pos->ZKey(), eval, m, depth, BoundType::LOWER_BOUND);
#endif
            return beta;
        }

        if (eval > alpha)
        {
            alpha = eval;
            best_move_in_node = m;
#if USE_TRANSPOSITION_TABLE == 1
            hash_entry_flag = BoundType::EXACT_VAL;
#endif

            if (is_quiet)
            {
                int bonus = depth * depth;
                int& h = history_[us][from][to];
                h += bonus - h * bonus / MAX_HISTORY;
            }

            pv_table_[ply][ply] = m;
            for (int j = ply + 1; j < pv_length_[ply + 1]; ++j)
                pv_table_[ply][j] = pv_table_[ply + 1][j];
            pv_length_[ply] = pv_length_[ply + 1];
        }
    }

    // ── Checkmate / stalemate ─────────────────────────────────────
    if (legal_moves == 0)
    {
        if (in_check) return Eval::NEG_INF + ply;
        return 0;
    }

#if USE_TRANSPOSITION_TABLE == 1
    tt->Store(pos->ZKey(), alpha, best_move_in_node, depth, hash_entry_flag);
#endif

    return alpha;
}

// ── Iterative deepening with progressive aspiration windows ───────
Move Search::FindBestMove(Position* pos, TransposTable* tt, TimeManager* tm)
{
    Clear();

    Move last_best_move = Moves::NULL_MOVE;
    Score last_best_eval = Eval::NEG_INF;

    for (int depth = 1; depth < MAX_DEPTH; ++depth)
    {
        Score alpha = Eval::NEG_INF;
        Score beta  = Eval::POS_INF;
        Score delta = 25;

        // Aspiration windows after depth 4
        if (depth >= 4)
        {
            alpha = last_best_eval - delta;
            beta  = last_best_eval + delta;
        }

        Score eval;

        // Progressive widening loop
        while (true)
        {
            eval = GoSearch(tt, pos, depth, 0, tm, alpha, beta);

            if (tm->OutOfTime()) break;

            if (eval <= alpha)
            {
                alpha = std::max(Score(eval - delta), Eval::NEG_INF);
                beta  = Score((alpha + beta) / 2);  // shrink beta toward center
            }
            else if (eval >= beta)
            {
                beta = std::min(Score(eval + delta), Eval::POS_INF);
            }
            else
            {
                break; // within window
            }

            delta += delta / 2 + 5; // progressive widening

            if (delta > 500)
            {
                alpha = Eval::NEG_INF;
                beta  = Eval::POS_INF;
            }
        }

        if (tm->OutOfTime() && last_best_move != Moves::NULL_MOVE)
            break;

        // Dynamic time: extend when best move changes
        Move new_best = (pv_length_[0] > 0) ? pv_table_[0][0] : Moves::NULL_MOVE;
        if (depth >= 6 && new_best != Moves::NULL_MOVE
            && last_best_move != Moves::NULL_MOVE && new_best != last_best_move)
        {
            tm->ExtendSoftTime(0.75); // use up to 75% of hard time
        }

        if (pv_length_[0] > 0)
            last_best_move = pv_table_[0][0];

        last_best_eval = eval;

        // UCI info output
        std::cout << "info score cp " << last_best_eval
                  << " depth " << depth
                  << " nodes " << nodes_;
        if (pv_length_[0] > 0)
        {
            std::cout << " pv";
            for (int j = 0; j < pv_length_[0]; ++j)
                std::cout << " " << UTIL::MoveToStr(pv_table_[0][j]);
        }
        std::cout << '\n';

        if (last_best_eval >= Eval::POS_INF - MAX_DEPTH
            || last_best_eval <= Eval::NEG_INF + MAX_DEPTH)
            break;

        if (tm->SoftTimeUp())
            break;
    }

    return last_best_move;
}
