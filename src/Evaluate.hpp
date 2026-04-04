#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "Types.hpp"
#include "Position.hpp"
#include "MagicConstants.hpp"
#include "MoveGen.hpp"
#include <array>

namespace Eval
{
    // ── Constants ─────────────────────────────────────────────────────
    constexpr Score POS_INF = std::numeric_limits<Score>::max() >> 1;
    constexpr Score NEG_INF = -POS_INF;
    constexpr Score PAWN_VAL    = 100;
    constexpr Score KNIGHT_VAL  = 320;
    constexpr Score BISHOP_VAL  = 330;
    constexpr Score ROOK_VAL    = 500;
    constexpr Score QUEEN_VAL   = 950;

    // ── Game phase: 0 = endgame, 24 = opening ────────────────────────
    constexpr int MAX_PHASE = 24;

    constexpr int GamePhase(Position const* pos)
    {
        int phase = 0;
        phase += Magics::PopCnt(pos->Pieces(pt_Knight));
        phase += Magics::PopCnt(pos->Pieces(pt_Bishop));
        phase += Magics::PopCnt(pos->Pieces(pt_Rook))   * 2;
        phase += Magics::PopCnt(pos->Pieces(pt_Queen))   * 4;
        return std::min(phase, MAX_PHASE);
    }

    // ── File / rank masks ─────────────────────────────────────────────
    constexpr BitBoard FILE_A_BB = 0x0101010101010101ULL;
    constexpr BitBoard FILE_H_BB = FILE_A_BB << 7;

    constexpr BitBoard FileBB(int f) { return FILE_A_BB << f; }

    constexpr std::array<BitBoard, 8> ADJACENT_FILES = [] {
        std::array<BitBoard, 8> a{};
        for (int f = 0; f < 8; ++f) {
            if (f > 0) a[f] |= FileBB(f - 1);
            if (f < 7) a[f] |= FileBB(f + 1);
        }
        return a;
    }();

    // Forward span: all squares strictly ahead on same + adjacent files
    // Used for passed pawn detection
    constexpr std::array<std::array<BitBoard, 64>, 2> PASSED_MASKS = [] {
        std::array<std::array<BitBoard, 64>, 2> m{};
        for (int sq = 0; sq < 64; ++sq) {
            int file = sq & 7;
            int rank = sq >> 3;
            BitBoard files = FileBB(file) | ADJACENT_FILES[file];

            // White: ranks above
            BitBoard wm = 0;
            for (int r = rank + 1; r < 8; ++r)
                wm |= 0xFFULL << (r * 8);
            m[White][sq] = files & wm;

            // Black: ranks below
            BitBoard bm = 0;
            for (int r = 0; r < rank; ++r)
                bm |= 0xFFULL << (r * 8);
            m[Black][sq] = files & bm;
        }
        return m;
    }();

    // ── Passed pawn bonus by rank (from that side's perspective) ──────
    //                            rank: 0   1   2   3    4    5    6   7
    constexpr Score PASSED_MG[] = { 0,  0,  5, 10,  20,  35,  60,  0 };
    constexpr Score PASSED_EG[] = { 0,  0, 10, 20,  45,  80, 150,  0 };

    // ── Pawn structure penalties ──────────────────────────────────────
    constexpr Score ISOLATED_MG = -10;
    constexpr Score ISOLATED_EG = -18;
    constexpr Score DOUBLED_MG  = -8;
    constexpr Score DOUBLED_EG  = -12;

    // ── Bishop pair bonus ─────────────────────────────────────────────
    constexpr Score BISHOP_PAIR_MG = 25;
    constexpr Score BISHOP_PAIR_EG = 45;

    // ── Rook bonuses ──────────────────────────────────────────────────
    constexpr Score ROOK_OPEN_MG     = 20;
    constexpr Score ROOK_OPEN_EG     = 12;
    constexpr Score ROOK_SEMIOPEN_MG = 10;
    constexpr Score ROOK_SEMIOPEN_EG = 8;
    constexpr Score ROOK_7TH_MG      = 15;
    constexpr Score ROOK_7TH_EG      = 25;

    // ── King safety: pawn shield ──────────────────────────────────────
    constexpr Score SHIELD_BONUS_MG = 8; // per pawn in shield zone

    // ── Piece-square tables (from White's perspective, a1=0) ──────────
    constexpr std::array<Score, 64> PST_PAWN_MG = {
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10,-20,-20, 10, 10,  5,
         5, -5,-10,  0,  0,-10, -5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5,  5, 10, 25, 25, 10,  5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    constexpr std::array<Score, 64> PST_KNIGHT_MG = {
       -50,-40,-30,-30,-30,-30,-40,-50,
       -40,-20,  0,  5,  5,  0,-20,-40,
       -30,  5, 10, 15, 15, 10,  5,-30,
       -30,  0, 15, 20, 20, 15,  0,-30,
       -30,  5, 15, 20, 20, 15,  5,-30,
       -30,  0, 10, 15, 15, 10,  0,-30,
       -40,-20,  0,  0,  0,  0,-20,-40,
       -50,-40,-30,-30,-30,-30,-40,-50
    };
    constexpr std::array<Score, 64> PST_BISHOP_MG = {
       -20,-10,-10,-10,-10,-10,-10,-20,
       -10,  5,  0,  0,  0,  0,  5,-10,
       -10, 10, 10, 10, 10, 10, 10,-10,
       -10,  0, 10, 10, 10, 10,  0,-10,
       -10,  5,  5, 10, 10,  5,  5,-10,
       -10,  0,  5, 10, 10,  5,  0,-10,
       -10,  0,  0,  0,  0,  0,  0,-10,
       -20,-10,-10,-10,-10,-10,-10,-20
    };
    constexpr std::array<Score, 64> PST_ROOK_MG = {
         0,  0,  0,  5,  5,  0,  0,  0,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         5, 10, 10, 10, 10, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    constexpr std::array<Score, 64> PST_QUEEN_MG = {
       -20,-10,-10, -5, -5,-10,-10,-20,
       -10,  0,  5,  0,  0,  0,  0,-10,
       -10,  5,  5,  5,  5,  5,  0,-10,
         0,  0,  5,  5,  5,  5,  0, -5,
        -5,  0,  5,  5,  5,  5,  0, -5,
       -10,  0,  5,  5,  5,  5,  0,-10,
       -10,  0,  0,  0,  0,  0,  0,-10,
       -20,-10,-10, -5, -5,-10,-10,-20
    };
    constexpr std::array<Score, 64> PST_KING_MG = {
        20, 30, 10,  0,  0, 10, 30, 20,
        20, 20,  0,  0,  0,  0, 20, 20,
       -10,-20,-20,-20,-20,-20,-20,-10,
       -20,-30,-30,-40,-40,-30,-30,-20,
       -30,-40,-40,-50,-50,-40,-40,-30,
       -30,-40,-40,-50,-50,-40,-40,-30,
       -30,-40,-40,-50,-50,-40,-40,-30,
       -30,-40,-40,-50,-50,-40,-40,-30
    };

    // Endgame tables
    constexpr std::array<Score, 64> PST_PAWN_EG = {
         0,  0,  0,  0,  0,  0,  0,  0,
        10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 10, 10, 10, 10, 10,
        20, 20, 20, 20, 20, 20, 20, 20,
        30, 30, 30, 30, 30, 30, 30, 30,
        50, 50, 50, 50, 50, 50, 50, 50,
        80, 80, 80, 80, 80, 80, 80, 80,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    constexpr std::array<Score, 64> PST_KING_EG = {
       -50,-30,-30,-30,-30,-30,-30,-50,
       -30,-20,-10,  0,  0,-10,-20,-30,
       -30,-10, 20, 30, 30, 20,-10,-30,
       -30,-10, 30, 40, 40, 30,-10,-30,
       -30,-10, 30, 40, 40, 30,-10,-30,
       -30,-10, 20, 30, 30, 20,-10,-30,
       -30,-30,  0,  0,  0,  0,-30,-30,
       -50,-30,-30,-30,-30,-30,-30,-50
    };

    // ── PST evaluation (tapered) ──────────────────────────────────────
    constexpr Sq FlipSq(Sq s) { return s ^ 56; }

    template<Colour C>
    constexpr Score PieceSquareScore(Position const* pos, int phase)
    {
        Score mg{0}, eg{0};

        auto eval_piece = [&](PieceType pt, const std::array<Score,64>& mg_table,
                              const std::array<Score,64>& eg_table)
        {
            BitBoard pieces = pos->Pieces(C, pt);
            while (pieces) {
                const Sq sq = Magics::FindLS1B(pieces);
                const Sq idx = (C == White) ? sq : FlipSq(sq);
                mg += mg_table[idx];
                eg += eg_table[idx];
                pieces = Magics::PopLS1B(pieces);
            }
        };

        eval_piece(pt_Pawn,   PST_PAWN_MG,   PST_PAWN_EG);
        eval_piece(pt_Knight, PST_KNIGHT_MG, PST_KNIGHT_MG);
        eval_piece(pt_Bishop, PST_BISHOP_MG, PST_BISHOP_MG);
        eval_piece(pt_Rook,   PST_ROOK_MG,   PST_ROOK_MG);
        eval_piece(pt_Queen,  PST_QUEEN_MG,  PST_QUEEN_MG);
        eval_piece(pt_King,   PST_KING_MG,   PST_KING_EG);

        return Score((mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE);
    }

    // ── Material ──────────────────────────────────────────────────────
    template<Colour C>
    constexpr Score CountMaterial(Position const* pos)
    {
        Score mat{0};
        mat += Score(Magics::PopCnt(pos->Pieces(C, pt_Pawn  )) * PAWN_VAL);
        mat += Score(Magics::PopCnt(pos->Pieces(C, pt_Knight)) * KNIGHT_VAL);
        mat += Score(Magics::PopCnt(pos->Pieces(C, pt_Bishop)) * BISHOP_VAL);
        mat += Score(Magics::PopCnt(pos->Pieces(C, pt_Rook  )) * ROOK_VAL);
        mat += Score(Magics::PopCnt(pos->Pieces(C, pt_Queen )) * QUEEN_VAL);
        return mat;
    }

    // ── Pawn structure evaluation (tapered) ───────────────────────────
    template<Colour C>
    constexpr Score EvalPawns(Position const* pos, int phase)
    {
        constexpr Colour Them = (C == White) ? Black : White;
        Score mg{0}, eg{0};

        const BitBoard our_pawns   = pos->Pieces(C, pt_Pawn);
        const BitBoard their_pawns = pos->Pieces(Them, pt_Pawn);

        // King positions for passed pawn proximity
        const Sq our_king_sq   = Magics::FindLS1B(pos->Pieces(C, pt_King));
        const Sq their_king_sq = Magics::FindLS1B(pos->Pieces(Them, pt_King));

        BitBoard pawns = our_pawns;
        while (pawns) {
            const Sq sq   = Magics::FindLS1B(pawns);
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int rel_rank = (C == White) ? rank : (7 - rank);

            // Passed pawn
            if (!(their_pawns & PASSED_MASKS[C][sq])) {
                mg += PASSED_MG[rel_rank];
                eg += PASSED_EG[rel_rank];

                // King proximity bonus (endgame): our king close = good, their king close = bad
                const int our_dist = std::max(std::abs((sq >> 3) - (our_king_sq >> 3)),
                                              std::abs((sq & 7)  - (our_king_sq & 7)));
                const int their_dist = std::max(std::abs((sq >> 3) - (their_king_sq >> 3)),
                                                std::abs((sq & 7)  - (their_king_sq & 7)));
                eg += Score((their_dist - our_dist) * rel_rank);
            }

            // Isolated pawn
            if (!(our_pawns & ADJACENT_FILES[file])) {
                mg += ISOLATED_MG;
                eg += ISOLATED_EG;
            }

            // Doubled pawn
            if (Magics::PopCnt(our_pawns & FileBB(file)) > 1) {
                mg += DOUBLED_MG / 2;
                eg += DOUBLED_EG / 2;
            }

            pawns = Magics::PopLS1B(pawns);
        }

        return Score((mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE);
    }

    // ── Piece bonuses (bishop pair, rook on open files, rook on 7th) ──
    template<Colour C>
    constexpr Score EvalPieces(Position const* pos, int phase)
    {
        Score mg{0}, eg{0};
        const BitBoard all_pawns = pos->Pieces(pt_Pawn);
        const BitBoard our_pawns = pos->Pieces(C, pt_Pawn);

        // Bishop pair
        if (Magics::PopCnt(pos->Pieces(C, pt_Bishop)) >= 2) {
            mg += BISHOP_PAIR_MG;
            eg += BISHOP_PAIR_EG;
        }

        // Rook bonuses
        constexpr int SEVENTH_RANK = (C == White) ? 6 : 1;
        BitBoard rooks = pos->Pieces(C, pt_Rook);
        while (rooks) {
            const Sq sq = Magics::FindLS1B(rooks);
            const int file = sq & 7;
            const int rank = sq >> 3;
            const BitBoard file_bb = FileBB(file);

            // Open file (no pawns at all)
            if (!(all_pawns & file_bb)) {
                mg += ROOK_OPEN_MG;
                eg += ROOK_OPEN_EG;
            }
            // Semi-open file (no friendly pawns)
            else if (!(our_pawns & file_bb)) {
                mg += ROOK_SEMIOPEN_MG;
                eg += ROOK_SEMIOPEN_EG;
            }

            // Rook on 7th rank
            if (rank == SEVENTH_RANK) {
                mg += ROOK_7TH_MG;
                eg += ROOK_7TH_EG;
            }

            rooks = Magics::PopLS1B(rooks);
        }

        return Score((mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE);
    }

    // ── King safety: pawn shield ──────────────────────────────────────
    template<Colour C>
    constexpr Score KingSafety(Position const* pos, int phase)
    {
        // Only matters in middlegame
        if (phase < 8) return 0;

        const Sq king_sq = Magics::FindLS1B(pos->Pieces(C, pt_King));
        const int king_file = king_sq & 7;
        const BitBoard our_pawns = pos->Pieces(C, pt_Pawn);

        Score bonus{0};

        // Check 2-3 squares directly in front of king
        constexpr int forward = (C == White) ? 8 : -8;
        for (int df = -1; df <= 1; ++df) {
            const int f = king_file + df;
            if (f < 0 || f > 7) continue;

            // Check one and two ranks ahead
            const int sq1 = king_sq + forward + df;
            const int sq2 = king_sq + forward * 2 + df;

            if (sq1 >= 0 && sq1 < 64 && (our_pawns & Magics::SqToBB(sq1)))
                bonus += SHIELD_BONUS_MG;
            else if (sq2 >= 0 && sq2 < 64 && (our_pawns & Magics::SqToBB(sq2)))
                bonus += SHIELD_BONUS_MG / 2;
        }

        // Taper: full weight in middlegame, zero in endgame
        return Score(bonus * phase / MAX_PHASE);
    }

    // ── Mobility weights (per-square, centred around typical mobility) ─
    // Knight: typical ~4 safe squares, bishop ~7, rook ~7, queen ~14
    constexpr Score MOB_KNIGHT_MG = 4, MOB_KNIGHT_EG = 4;
    constexpr Score MOB_BISHOP_MG = 5, MOB_BISHOP_EG = 5;
    constexpr Score MOB_ROOK_MG   = 2, MOB_ROOK_EG   = 4;
    constexpr Score MOB_QUEEN_MG  = 1, MOB_QUEEN_EG  = 2;

    template<Colour C>
    inline Score EvalMobility(Position const* pos, int phase)
    {
        Score mg{0}, eg{0};

        const BitBoard us   = pos->Pieces(C);
        const BitBoard them = pos->Pieces(!C);

        // Enemy pawn attack span — unsafe squares for our pieces
        const BitBoard their_pawns = pos->Pieces(!C, pt_Pawn);
        BitBoard enemy_pawn_atk;
        if constexpr (C == White)
            enemy_pawn_atk = Magics::Shift<SOUTH_EAST>(their_pawns) | Magics::Shift<SOUTH_WEST>(their_pawns);
        else
            enemy_pawn_atk = Magics::Shift<NORTH_EAST>(their_pawns) | Magics::Shift<NORTH_WEST>(their_pawns);

        const BitBoard safe = ~enemy_pawn_atk;

        // Knights
        BitBoard knights = pos->Pieces(C, pt_Knight);
        while (knights) {
            const Sq sq = Magics::FindLS1B(knights);
            int mob = Magics::PopCnt(Magics::KNIGHT_ATTACK_MASKS[sq] & ~us & safe);
            mg += Score(MOB_KNIGHT_MG * (mob - 4));
            eg += Score(MOB_KNIGHT_EG * (mob - 4));
            knights = Magics::PopLS1B(knights);
        }

        // Bishops
        BitBoard bishops = pos->Pieces(C, pt_Bishop);
        while (bishops) {
            const Sq sq = Magics::FindLS1B(bishops);
            BitBoard attacks = MoveGen::GetSlidingAttacks<Diagonal>(sq, us, them)
                             | MoveGen::GetSlidingAttacks<AntiDiagonal>(sq, us, them);
            int mob = Magics::PopCnt(attacks & safe);
            mg += Score(MOB_BISHOP_MG * (mob - 7));
            eg += Score(MOB_BISHOP_EG * (mob - 7));
            bishops = Magics::PopLS1B(bishops);
        }

        // Rooks
        BitBoard rooks = pos->Pieces(C, pt_Rook);
        while (rooks) {
            const Sq sq = Magics::FindLS1B(rooks);
            BitBoard attacks = MoveGen::GetSlidingAttacks<File>(sq, us, them)
                             | MoveGen::GetSlidingAttacks<Rank>(sq, us, them);
            int mob = Magics::PopCnt(attacks & safe);
            mg += Score(MOB_ROOK_MG * (mob - 7));
            eg += Score(MOB_ROOK_EG * (mob - 7));
            rooks = Magics::PopLS1B(rooks);
        }

        // Queen (small weight — less important than minor piece mobility)
        BitBoard queens = pos->Pieces(C, pt_Queen);
        while (queens) {
            const Sq sq = Magics::FindLS1B(queens);
            BitBoard attacks = MoveGen::GetSlidingAttacks<File>(sq, us, them)
                             | MoveGen::GetSlidingAttacks<Rank>(sq, us, them)
                             | MoveGen::GetSlidingAttacks<Diagonal>(sq, us, them)
                             | MoveGen::GetSlidingAttacks<AntiDiagonal>(sq, us, them);
            int mob = Magics::PopCnt(attacks & safe);
            mg += Score(MOB_QUEEN_MG * (mob - 14));
            eg += Score(MOB_QUEEN_EG * (mob - 14));
            queens = Magics::PopLS1B(queens);
        }

        return Score((mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE);
    }

    // ── Tempo bonus ─────────────────────────────────────────────────
    constexpr Score TEMPO = 12;

    // ── Knight outpost: knight on ranks 4-6, protected by pawn, ───
    //    no enemy pawns on adjacent files that can challenge it
    constexpr Score OUTPOST_MG = 20;
    constexpr Score OUTPOST_EG = 10;

    template<Colour C>
    constexpr Score EvalOutposts(Position const* pos, int phase)
    {
        constexpr Colour Them = (C == White) ? Black : White;
        Score mg{0}, eg{0};

        const BitBoard our_pawns   = pos->Pieces(C, pt_Pawn);
        const BitBoard their_pawns = pos->Pieces(Them, pt_Pawn);

        BitBoard knights = pos->Pieces(C, pt_Knight);
        while (knights) {
            const Sq sq = Magics::FindLS1B(knights);
            const int rank = sq >> 3;
            const int file = sq & 7;
            const int rel_rank = (C == White) ? rank : (7 - rank);

            // Only ranks 4-6 (from our perspective)
            if (rel_rank >= 3 && rel_rank <= 5) {
                // Protected by our pawn?
                const BitBoard sq_bb = Magics::SqToBB(sq);
                BitBoard our_pawn_support;
                if constexpr (C == White)
                    our_pawn_support = (Magics::Shift<SOUTH_WEST>(sq_bb) | Magics::Shift<SOUTH_EAST>(sq_bb)) & our_pawns;
                else
                    our_pawn_support = (Magics::Shift<NORTH_WEST>(sq_bb) | Magics::Shift<NORTH_EAST>(sq_bb)) & our_pawns;

                if (our_pawn_support) {
                    // No enemy pawns on adjacent files that can kick it
                    if (!(their_pawns & ADJACENT_FILES[file] & PASSED_MASKS[Them][sq])) {
                        mg += OUTPOST_MG;
                        eg += OUTPOST_EG;
                    }
                }
            }
            knights = Magics::PopLS1B(knights);
        }

        return Score((mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE);
    }

    // ── King danger: bonus for attacking the area around enemy king ────
    // Non-linear scaling: more attackers = disproportionately dangerous
    constexpr int KING_ZONE_ATTACK_WEIGHT[] = { 0, 0, 50, 75, 88, 95, 97, 99, 99 };
    // Piece attack units: knight=2, bishop=2, rook=3, queen=5
    constexpr int ATTACK_UNIT_KNIGHT = 2, ATTACK_UNIT_BISHOP = 2;
    constexpr int ATTACK_UNIT_ROOK = 3,   ATTACK_UNIT_QUEEN = 5;

    template<Colour C>
    inline Score EvalKingDanger(Position const* pos, int phase)
    {
        // Only meaningful in middlegame
        if (phase < 6) return 0;

        constexpr Colour Them = (C == White) ? Black : White;
        const Sq their_king_sq = Magics::FindLS1B(pos->Pieces(Them, pt_King));

        // King zone: king attack mask (8 squares around king)
        const BitBoard king_zone = Magics::KING_ATTACK_MASKS[their_king_sq]
                                 | Magics::SqToBB(their_king_sq);

        const BitBoard us   = pos->Pieces(C);
        const BitBoard them = pos->Pieces(Them);

        int attackers = 0;
        int attack_units = 0;

        // Knights attacking king zone
        BitBoard knights = pos->Pieces(C, pt_Knight);
        while (knights) {
            if (Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & king_zone) {
                ++attackers; attack_units += ATTACK_UNIT_KNIGHT;
            }
            knights = Magics::PopLS1B(knights);
        }

        // Bishops attacking king zone
        BitBoard bishops = pos->Pieces(C, pt_Bishop);
        while (bishops) {
            const Sq sq = Magics::FindLS1B(bishops);
            BitBoard atk = MoveGen::GetSlidingAttacks<Diagonal>(sq, us, them)
                         | MoveGen::GetSlidingAttacks<AntiDiagonal>(sq, us, them);
            if (atk & king_zone) { ++attackers; attack_units += ATTACK_UNIT_BISHOP; }
            bishops = Magics::PopLS1B(bishops);
        }

        // Rooks attacking king zone
        BitBoard rooks = pos->Pieces(C, pt_Rook);
        while (rooks) {
            const Sq sq = Magics::FindLS1B(rooks);
            BitBoard atk = MoveGen::GetSlidingAttacks<File>(sq, us, them)
                         | MoveGen::GetSlidingAttacks<Rank>(sq, us, them);
            if (atk & king_zone) { ++attackers; attack_units += ATTACK_UNIT_ROOK; }
            rooks = Magics::PopLS1B(rooks);
        }

        // Queens attacking king zone
        BitBoard queens = pos->Pieces(C, pt_Queen);
        while (queens) {
            const Sq sq = Magics::FindLS1B(queens);
            BitBoard atk = MoveGen::GetSlidingAttacks<File>(sq, us, them)
                         | MoveGen::GetSlidingAttacks<Rank>(sq, us, them)
                         | MoveGen::GetSlidingAttacks<Diagonal>(sq, us, them)
                         | MoveGen::GetSlidingAttacks<AntiDiagonal>(sq, us, them);
            if (atk & king_zone) { ++attackers; attack_units += ATTACK_UNIT_QUEEN; }
            queens = Magics::PopLS1B(queens);
        }

        if (attackers < 2) return 0;  // need at least 2 attackers for real danger

        // Non-linear scaling: index into weight table
        const int idx = std::min(attackers, 8);
        const Score danger = Score(attack_units * KING_ZONE_ATTACK_WEIGHT[idx] / 100);

        // Taper to middlegame
        return Score(danger * phase / MAX_PHASE);
    }

    // ── Main evaluation ───────────────────────────────────────────────
    inline Score Evaluate(Position const* pos)
    {
        const int phase = GamePhase(pos);

        const Score white_eval = CountMaterial<White>(pos)
                               + PieceSquareScore<White>(pos, phase)
                               + EvalPawns<White>(pos, phase)
                               + EvalPieces<White>(pos, phase)
                               + KingSafety<White>(pos, phase)
                               + EvalOutposts<White>(pos, phase)
                               + EvalMobility<White>(pos, phase)
                               + EvalKingDanger<White>(pos, phase);

        const Score black_eval = CountMaterial<Black>(pos)
                               + PieceSquareScore<Black>(pos, phase)
                               + EvalPawns<Black>(pos, phase)
                               + EvalPieces<Black>(pos, phase)
                               + KingSafety<Black>(pos, phase)
                               + EvalOutposts<Black>(pos, phase)
                               + EvalMobility<Black>(pos, phase)
                               + EvalKingDanger<Black>(pos, phase);

        // Side to move gets a tempo bonus
        return (white_eval - black_eval) * (pos->ColourToMove() == White ? 1 : -1) + TEMPO;
    }
}
#endif // #ifndef EVALUATE_HPP
