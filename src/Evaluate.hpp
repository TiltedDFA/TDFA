#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "Types.hpp"
#include "Position.hpp"
#include "MagicConstants.hpp"
#include "MoveGen.hpp"
#include <array>
#include <limits>

namespace Eval
{
    //constants
    constexpr Score POS_INF = std::numeric_limits<Score>::max() >> 1;
    constexpr Score NEG_INF = -POS_INF;
    constexpr Score PAWN_VAL    = 100;
    constexpr Score KNIGHT_VAL  = 310;
    constexpr Score BISHOP_VAL  = 320;
    constexpr Score ROOK_VAL    = 520;
    constexpr Score QUEEN_VAL   = 950;

    constexpr Score TEMPO_BONUS = 10;
    constexpr Score BISHOP_PAIR_MG = 30;
    constexpr Score BISHOP_PAIR_EG = 45;
    constexpr Score PAWN_SHIELD_BONUS = 10;
    constexpr Score KING_NO_PAWN_FILE_PENALTY = 12;
    constexpr Score KING_OPEN_FILE_EXTRA_PENALTY = 6;

    constexpr std::array<Score, 8> PASSED_PAWN_BONUS_MG = {0, 6, 12, 20, 35, 55, 90, 0};
    constexpr std::array<Score, 8> PASSED_PAWN_BONUS_EG = {0, 10, 20, 35, 60, 90, 140, 0};

    constexpr Score MOB_KING_MG = -2;
    constexpr Score MOB_KING_EG = 3;
    constexpr Score MOB_QUEEN_MG = 1;
    constexpr Score MOB_QUEEN_EG = 2;
    constexpr Score MOB_BISHOP_MG = 4;
    constexpr Score MOB_BISHOP_EG = 4;
    constexpr Score MOB_KNIGHT_MG = 4;
    constexpr Score MOB_KNIGHT_EG = 4;
    constexpr Score MOB_ROOK_MG = 2;
    constexpr Score MOB_ROOK_EG = 3;

    constexpr int PHASE_KNIGHT = 1;
    constexpr int PHASE_BISHOP = 1;
    constexpr int PHASE_ROOK   = 2;
    constexpr int PHASE_QUEEN  = 4;
    constexpr int MAX_PHASE    = 24;

    constexpr int AbsI(int v) {return v < 0 ? -v : v;}
    constexpr int FileBonus(int file)
    {
        const int bonus = 3 - AbsI(file - 3);
        return (bonus > 0) ? bonus : 0;
    }
    constexpr int CenterScore(int file, int rank)
    {
        const int df = AbsI(file - 3);
        const int dr = AbsI(rank - 3);
        const int center = 6 - (df + dr);
        return (center > 0) ? center : 0;
    }
    constexpr Sq MirrorSq(const Sq sq) {return sq ^ 56;}

    consteval std::array<Score, 64> BuildPawnPstMg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = rank * 6 + FileBonus(file) * 2;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildPawnPstEg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = rank * 10 + FileBonus(file);
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildKnightPstMg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 6;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildKnightPstEg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 4;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildBishopPstMg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 4;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildBishopPstEg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 4;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildRookPstMg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            int score = FileBonus(file);
            if(rank == 6) score += 15;
            else if(rank == 5) score += 8;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildRookPstEg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            int score = FileBonus(file);
            if(rank >= 4) score += 6;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildQueenPstMg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 2 + FileBonus(file);
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildQueenPstEg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 3 + FileBonus(file);
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildKingPstMg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = -CenterScore(file, rank) * 8;
            table[sq] = Score(score);
        }
        return table;
    }
    consteval std::array<Score, 64> BuildKingPstEg()
    {
        std::array<Score, 64> table{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            const int score = CenterScore(file, rank) * 8;
            table[sq] = Score(score);
        }
        return table;
    }

    inline constexpr std::array<Score, 64> PAWN_PST_MG   = BuildPawnPstMg();
    inline constexpr std::array<Score, 64> PAWN_PST_EG   = BuildPawnPstEg();
    inline constexpr std::array<Score, 64> KNIGHT_PST_MG = BuildKnightPstMg();
    inline constexpr std::array<Score, 64> KNIGHT_PST_EG = BuildKnightPstEg();
    inline constexpr std::array<Score, 64> BISHOP_PST_MG = BuildBishopPstMg();
    inline constexpr std::array<Score, 64> BISHOP_PST_EG = BuildBishopPstEg();
    inline constexpr std::array<Score, 64> ROOK_PST_MG   = BuildRookPstMg();
    inline constexpr std::array<Score, 64> ROOK_PST_EG   = BuildRookPstEg();
    inline constexpr std::array<Score, 64> QUEEN_PST_MG  = BuildQueenPstMg();
    inline constexpr std::array<Score, 64> QUEEN_PST_EG  = BuildQueenPstEg();
    inline constexpr std::array<Score, 64> KING_PST_MG   = BuildKingPstMg();
    inline constexpr std::array<Score, 64> KING_PST_EG   = BuildKingPstEg();

    template<Colour c>
    consteval std::array<BitBoard, 64> BuildPassedMasks()
    {
        std::array<BitBoard, 64> masks{};
        for(int sq = 0; sq < 64; ++sq)
        {
            const int file = sq & 7;
            const int rank = sq >> 3;
            BitBoard mask = 0;
            if constexpr(c == White)
            {
                for(int r = rank + 1; r < 8; ++r)
                {
                    for(int f = file - 1; f <= file + 1; ++f)
                    {
                        if(f < 0 || f > 7) continue;
                        mask |= BitBoard(1) << (r * 8 + f);
                    }
                }
            }
            else
            {
                for(int r = rank - 1; r >= 0; --r)
                {
                    for(int f = file - 1; f <= file + 1; ++f)
                    {
                        if(f < 0 || f > 7) continue;
                        mask |= BitBoard(1) << (r * 8 + f);
                    }
                }
            }
            masks[sq] = mask;
        }
        return masks;
    }
    inline constexpr std::array<BitBoard, 64> PASSED_MASK_WHITE = BuildPassedMasks<White>();
    inline constexpr std::array<BitBoard, 64> PASSED_MASK_BLACK = BuildPassedMasks<Black>();

    struct EvalScore
    {
        int mg;
        int eg;
    };

    template<Colour c>
    constexpr Score PstValue(std::array<Score, 64> const& table, const Sq sq)
    {
        if constexpr(c == White) return table[sq];
        return table[MirrorSq(sq)];
    }
    template<Colour c>
    inline int KingSafety(Position const* pos)
    {
        const BitBoard pawns = pos->Pieces(c, pt_Pawn);
        const BitBoard enemy_pawns = pos->Pieces(!c, pt_Pawn);
        const Sq king_sq = Magics::FindLS1B(pos->Pieces(c, pt_King));
        const int file = Magics::FileOf(king_sq);
        const int rank = Magics::RankOf(king_sq);
        int score = 0;

        const int r1 = (c == White) ? rank + 1 : rank - 1;
        const int r2 = (c == White) ? rank + 2 : rank - 2;

        for(int df = -1; df <= 1; ++df)
        {
            const int f = file + df;
            if(f < 0 || f > 7) continue;
            if(r1 >= 0 && r1 < 8)
            {
                const Sq sq = Sq(r1 * 8 + f);
                if(pawns & Magics::SqToBB(sq)) score += PAWN_SHIELD_BONUS;
            }
            if(r2 >= 0 && r2 < 8)
            {
                const Sq sq = Sq(r2 * 8 + f);
                if(pawns & Magics::SqToBB(sq)) score += PAWN_SHIELD_BONUS / 2;
            }
        }

        const BitBoard file_bb = Magics::FILE_ABB << file;
        if((pawns & file_bb) == 0)
        {
            score -= KING_NO_PAWN_FILE_PENALTY;
            if((enemy_pawns & file_bb) == 0) score -= KING_OPEN_FILE_EXTRA_PENALTY;
        }

        return score;
    }
    template<Colour c>
    inline void AddMobility(Position const* pos, int* mg, int* eg)
    {
        const int k = Magics::PopCnt(MoveGen::KingAttacks<c>(pos));
        const int q = Magics::PopCnt(MoveGen::QueenAttacks<c>(pos));
        const int b = Magics::PopCnt(MoveGen::BishopAttacks<c>(pos));
        const int n = Magics::PopCnt(MoveGen::KnightAttacks<c>(pos));
        const int r = Magics::PopCnt(MoveGen::RookAttacks<c>(pos));
        *mg += k * MOB_KING_MG + q * MOB_QUEEN_MG + b * MOB_BISHOP_MG + n * MOB_KNIGHT_MG + r * MOB_ROOK_MG;
        *eg += k * MOB_KING_EG + q * MOB_QUEEN_EG + b * MOB_BISHOP_EG + n * MOB_KNIGHT_EG + r * MOB_ROOK_EG;
    }
    template<Colour c>
    inline EvalScore EvaluateSide(Position const* pos)
    {
        EvalScore s{0, 0};

        const BitBoard enemy_pawns = pos->Pieces(!c, pt_Pawn);
        const auto& passed_masks = (c == White) ? PASSED_MASK_WHITE : PASSED_MASK_BLACK;

        BitBoard pawns = pos->Pieces(c, pt_Pawn);
        while(pawns)
        {
            const Sq sq = Magics::PopNRetLS1B(pawns);
            s.mg += PAWN_VAL + PstValue<c>(PAWN_PST_MG, sq);
            s.eg += PAWN_VAL + PstValue<c>(PAWN_PST_EG, sq);

            if((enemy_pawns & passed_masks[sq]) == 0)
            {
                const Sq psq = (c == White) ? sq : MirrorSq(sq);
                const U8 rank = Magics::RankOf(psq);
                s.mg += PASSED_PAWN_BONUS_MG[rank];
                s.eg += PASSED_PAWN_BONUS_EG[rank];
            }
        }

        BitBoard knights = pos->Pieces(c, pt_Knight);
        while(knights)
        {
            const Sq sq = Magics::PopNRetLS1B(knights);
            s.mg += KNIGHT_VAL + PstValue<c>(KNIGHT_PST_MG, sq);
            s.eg += KNIGHT_VAL + PstValue<c>(KNIGHT_PST_EG, sq);
        }

        BitBoard bishops = pos->Pieces(c, pt_Bishop);
        while(bishops)
        {
            const Sq sq = Magics::PopNRetLS1B(bishops);
            s.mg += BISHOP_VAL + PstValue<c>(BISHOP_PST_MG, sq);
            s.eg += BISHOP_VAL + PstValue<c>(BISHOP_PST_EG, sq);
        }

        BitBoard rooks = pos->Pieces(c, pt_Rook);
        while(rooks)
        {
            const Sq sq = Magics::PopNRetLS1B(rooks);
            s.mg += ROOK_VAL + PstValue<c>(ROOK_PST_MG, sq);
            s.eg += ROOK_VAL + PstValue<c>(ROOK_PST_EG, sq);
        }

        BitBoard queens = pos->Pieces(c, pt_Queen);
        while(queens)
        {
            const Sq sq = Magics::PopNRetLS1B(queens);
            s.mg += QUEEN_VAL + PstValue<c>(QUEEN_PST_MG, sq);
            s.eg += QUEEN_VAL + PstValue<c>(QUEEN_PST_EG, sq);
        }

        const Sq king_sq = Magics::FindLS1B(pos->Pieces(c, pt_King));
        s.mg += PstValue<c>(KING_PST_MG, king_sq);
        s.eg += PstValue<c>(KING_PST_EG, king_sq);

        if(Magics::PopCnt(pos->Pieces(c, pt_Bishop)) >= 2)
        {
            s.mg += BISHOP_PAIR_MG;
            s.eg += BISHOP_PAIR_EG;
        }

        s.mg += KingSafety<c>(pos);
        AddMobility<c>(pos, &s.mg, &s.eg);

        return s;
    }

    inline int Phase(Position const* pos)
    {
        int phase = 0;
        phase += PHASE_KNIGHT * (Magics::PopCnt(pos->Pieces(White, pt_Knight)) + Magics::PopCnt(pos->Pieces(Black, pt_Knight)));
        phase += PHASE_BISHOP * (Magics::PopCnt(pos->Pieces(White, pt_Bishop)) + Magics::PopCnt(pos->Pieces(Black, pt_Bishop)));
        phase += PHASE_ROOK   * (Magics::PopCnt(pos->Pieces(White, pt_Rook))   + Magics::PopCnt(pos->Pieces(Black, pt_Rook)));
        phase += PHASE_QUEEN  * (Magics::PopCnt(pos->Pieces(White, pt_Queen))  + Magics::PopCnt(pos->Pieces(Black, pt_Queen)));
        if(phase > MAX_PHASE) phase = MAX_PHASE;
        return phase;
    }

    inline Score Evaluate(Position const* pos)
    {
        const EvalScore white = EvaluateSide<White>(pos);
        const EvalScore black = EvaluateSide<Black>(pos);
        const int phase = Phase(pos);
        const int mg = white.mg - black.mg;
        const int eg = white.eg - black.eg;
        const int tapered = (mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE;
        Score score = Score(tapered);
        if(pos->ColourToMove() == Black)
            score = Score(-score);
        score = Score(score + TEMPO_BONUS);
        return score;
    }
}

#endif // #ifndef EVALUATE_HPP
