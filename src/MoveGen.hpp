#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>
#include <immintrin.h>

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveList.hpp"

extern std::array<std::array<std::array<BitBoard, 2187>, 4>, 64> SLIDING_ATTACKS;
extern std::array<std::array<std::array<U8, 2187>, 4>, 64> SLIDING_ENDPOINTS;
extern std::array<std::array<std::array<ray_moves, 256>, 4>, 64> MOVE_LOOKUP;
extern std::array<std::array<std::array<move_info, 2187>, 4>, 64> SLIDING_ATTACK_CONFIG;
namespace MoveGen
{
    constexpr void GenerateMovesFromBB(BitBoard b, MoveList* ml, const Sq from, const MoveType type)
    {
        while(b)
        {
            ml->add(Moves::EncodeMove(from, Magics::PopNRetLS1B(b), type));
        }
    }

    template<AttackDirection direction, bool UsePext = USE_PEXT>
    inline INLINE BitBoard GetSlidingAttacks(Sq piece_sq, BitBoard us, BitBoard them) noexcept
    {
        if constexpr(direction == Rank)
        {
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            const U8 file_bit = U8(1u << file_of_attacker);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                const BitBoard rank_mask = Magics::RANK_1BB << (piece_sq & 56);
                us_collapsed = U8(_pext_u64(us, rank_mask));
                them_collapsed = U8(_pext_u64(them, rank_mask));
            }
            else
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedFilesIndex(us   & attack_mask);
                them_collapsed = Magics::CollapsedFilesIndex(them & attack_mask);
            }
            us_collapsed &= U8(~file_bit);
            them_collapsed &= U8(~file_bit);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, file_of_attacker);
            assert(lookup_index <= 2187);

            return SLIDING_ATTACKS _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else if constexpr(direction == File)
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                const BitBoard file_mask = Magics::FILE_ABB << file_of_attacker;
                us_collapsed = U8(_pext_u64(us, file_mask));
                them_collapsed = U8(_pext_u64(them, file_mask));
                const U8 rank_bit = U8(1u << rank_of_attacker);
                us_collapsed &= U8(~rank_bit);
                them_collapsed &= U8(~rank_bit);
            }
            else
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedRanksIndex(us   & attack_mask, file_of_attacker);
                them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask, file_of_attacker);
            }
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            assert(lookup_index <= 2187);

            return SLIDING_ATTACKS _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else //direction == Diag || direction == Anti Diag
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                constexpr int diag_idx = (direction == Diagonal) ? 0 : 1;
                const BitBoard full_mask = Magics::DIAG_FULL_MASK[piece_sq][diag_idx];
                const U8 start_rank = (direction == Diagonal)
                    ? (rank_of_attacker > file_of_attacker ? U8(rank_of_attacker - file_of_attacker) : 0)
                    : ((rank_of_attacker + file_of_attacker > 7)
                        ? U8(rank_of_attacker + file_of_attacker - 7)
                        : 0);
                us_collapsed = U8(_pext_u64(us, full_mask) << start_rank);
                them_collapsed = U8(_pext_u64(them, full_mask) << start_rank);
                const U8 rank_bit = U8(1u << rank_of_attacker);
                us_collapsed &= U8(~rank_bit);
                them_collapsed &= U8(~rank_bit);
            }
            else
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedRanksIndex(us   & attack_mask);
                them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask);
            }
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            assert(lookup_index <= 2187);

            return SLIDING_ATTACKS _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
    }

    template<AttackDirection direction, bool UsePext = USE_PEXT>
    inline INLINE U8 GetEndpoint(Sq piece_sq, BitBoard us, BitBoard them) noexcept
    {
        if constexpr(direction == Rank)
        {
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            const U8 file_bit = U8(1u << file_of_attacker);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                const BitBoard rank_mask = Magics::RANK_1BB << (piece_sq & 56);
                us_collapsed = U8(_pext_u64(us, rank_mask));
                them_collapsed = U8(_pext_u64(them, rank_mask));
            }
            else
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedFilesIndex(us   & attack_mask);
                them_collapsed = Magics::CollapsedFilesIndex(them & attack_mask);
            }
            us_collapsed &= U8(~file_bit);
            them_collapsed &= U8(~file_bit);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, file_of_attacker);
            assert(lookup_index <= 2187);

            return SLIDING_ENDPOINTS _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else if constexpr(direction == File)
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                const BitBoard file_mask = Magics::FILE_ABB << file_of_attacker;
                us_collapsed = U8(_pext_u64(us, file_mask));
                them_collapsed = U8(_pext_u64(them, file_mask));
                const U8 rank_bit = U8(1u << rank_of_attacker);
                us_collapsed &= U8(~rank_bit);
                them_collapsed &= U8(~rank_bit);
            }
            else
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedRanksIndex(us   & attack_mask, file_of_attacker);
                them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask, file_of_attacker);
            }
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            assert(lookup_index <= 2187);

            return SLIDING_ENDPOINTS _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else //direction == Diag || direction == Anti Diag
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                constexpr int diag_idx = (direction == Diagonal) ? 0 : 1;
                const BitBoard full_mask = Magics::DIAG_FULL_MASK[piece_sq][diag_idx];
                const U8 start_rank = (direction == Diagonal)
                    ? (rank_of_attacker > file_of_attacker ? U8(rank_of_attacker - file_of_attacker) : 0)
                    : ((rank_of_attacker + file_of_attacker > 7)
                        ? U8(rank_of_attacker + file_of_attacker - 7)
                        : 0);
                us_collapsed = U8(_pext_u64(us, full_mask) << start_rank);
                them_collapsed = U8(_pext_u64(them, full_mask) << start_rank);
                const U8 rank_bit = U8(1u << rank_of_attacker);
                us_collapsed &= U8(~rank_bit);
                them_collapsed &= U8(~rank_bit);
            }
            else
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedRanksIndex(us   & attack_mask);
                them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask);
            }
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            assert(lookup_index <= 2187);

            return SLIDING_ENDPOINTS _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
    }

    void WhitePawnMoves(Position const* pos, MoveList* ml) noexcept;

    void BlackPawnMoves(Position const* pos, MoveList* ml) noexcept;

    template<Colour colour_to_move>
    constexpr void BishopMoves(Position const* pos, MoveList* ml)
    {
        BitBoard bishops = pos->Pieces(colour_to_move, pt_Bishop);
        if(!bishops) return;
        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);
        while(bishops)
        {
            const Sq sq = Magics::PopNRetLS1B(bishops);
            const U8 ep_diag  = GetEndpoint<Diagonal>(sq, us, them);
            const U8 ep_adiag = GetEndpoint<AntiDiagonal>(sq, us, them);
            if(ep_diag)  ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(Diagonal)_AT(ep_diag));
            if(ep_adiag) ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(AntiDiagonal)_AT(ep_adiag));
        }
    }
    
    template<Colour colour_to_move>
    constexpr void RookMoves(Position const* pos, MoveList* ml)
    {
        BitBoard rooks = pos->Pieces(colour_to_move, pt_Rook);
        if(!rooks) return;
        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);
        while(rooks)
        {
            const Sq sq = Magics::PopNRetLS1B(rooks);
            const U8 ep_file = GetEndpoint<File>(sq, us, them);
            const U8 ep_rank = GetEndpoint<Rank>(sq, us, them);
            if(ep_file) ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(File)_AT(ep_file));
            if(ep_rank) ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(Rank)_AT(ep_rank));
        }
    }

    template<Colour colour_to_move>
    constexpr void QueenMoves(Position const* pos, MoveList* ml)
    {
        BitBoard queens = pos->Pieces(colour_to_move, pt_Queen);
        if(!queens) return;
        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);
        while(queens)
        {
            const Sq sq = Magics::PopNRetLS1B(queens);
            const U8 ep_file  = GetEndpoint<File>(sq, us, them);
            const U8 ep_rank  = GetEndpoint<Rank>(sq, us, them);
            const U8 ep_diag  = GetEndpoint<Diagonal>(sq, us, them);
            const U8 ep_adiag = GetEndpoint<AntiDiagonal>(sq, us, them);
            if(ep_file)  ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(File)_AT(ep_file));
            if(ep_rank)  ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(Rank)_AT(ep_rank));
            if(ep_diag)  ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(Diagonal)_AT(ep_diag));
            if(ep_adiag) ml->merge_ray(&MOVE_LOOKUP _AT(sq)_AT(AntiDiagonal)_AT(ep_adiag));
        }
    }

    template<Colour colour_to_move>
    constexpr void KnightMoves(Position const* pos, MoveList* ml)
    {
        BitBoard knights = pos->Pieces(colour_to_move, pt_Knight);
        if(!knights) return;
        const BitBoard occupied = pos->Pieces(White, Black);
        const BitBoard them = pos->Pieces(!colour_to_move);
        while(knights)
        {
            const U8 knight_index = Magics::PopNRetLS1B(knights);
            const BitBoard knight_attacks = Magics::KNIGHT_ATTACK_MASKS[knight_index];
            const BitBoard possible_quiet_move      = knight_attacks & ~occupied;
            const BitBoard possible_capture_moves   = knight_attacks & them;
            GenerateMovesFromBB(possible_quiet_move, ml, knight_index, mt_Quiet);
            GenerateMovesFromBB(possible_capture_moves, ml, knight_index, mt_Capture);
        }
    }

    template<Colour colour_to_move>
    void KingMoves(Position const* pos, MoveList* ml)
    {
        const U8 king_index = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        const BitBoard possible_quiet_move      = Magics::KING_ATTACK_MASKS[king_index] & ~pos->Pieces(White, Black);
        const BitBoard possible_capture_moves   = Magics::KING_ATTACK_MASKS[king_index] & pos->Pieces(!colour_to_move);
        GenerateMovesFromBB(possible_quiet_move, ml, king_index, mt_Quiet);
        GenerateMovesFromBB(possible_capture_moves, ml, king_index, mt_Capture);
    }

    template<Colour colour_to_move>
    constexpr BitBoard PawnAttacks(Position const* pos)
    {
        const BitBoard pawns = pos->Pieces(colour_to_move, pt_Pawn);
        if(!pawns) return 0;
        if constexpr(colour_to_move == White)
        {
            return (Magics::Shift<NORTH_EAST>(pawns) | Magics::Shift<NORTH_WEST>(pawns)) & ~pos->Pieces(White);
        }
        return (Magics::Shift<SOUTH_EAST>(pawns) | Magics::Shift<SOUTH_WEST>(pawns)) & ~pos->Pieces(Black);
    }
    
    template<Colour colour_to_move>
    constexpr BitBoard KingAttacks(Position const* pos)
    {
        const BitBoard king_index = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        return (Magics::KING_ATTACK_MASKS[king_index] & ~pos->Pieces(colour_to_move));
    }
    
    template<Colour colour_to_move>
    constexpr BitBoard KnightAttacks(Position const* pos)
    {
        BitBoard knights = pos->Pieces(colour_to_move, pt_Knight);
        if(!knights) return 0;

        BitBoard attacks{0};
        const BitBoard them = ~pos->Pieces(colour_to_move);

        while(knights)
        {
            attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::PopNRetLS1B(knights)] & them;
        }
        return attacks;
    }
    
    template<Colour colour_to_move>
    constexpr BitBoard BishopAttacks(Position const* pos)
    {
        BitBoard bishops = pos->Pieces(colour_to_move, pt_Bishop);
        if(!bishops) return 0;

        BitBoard attacks{0};
        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);

        while(bishops)
        {
            const U8 bishop_index = Magics::PopNRetLS1B(bishops);

            attacks |= GetSlidingAttacks<Diagonal      >(bishop_index, us, them);
            attacks |= GetSlidingAttacks<AntiDiagonal  >(bishop_index, us, them);
        }
        return attacks;
    }

    template<Colour colour_to_move>
    constexpr BitBoard RookAttacks(Position const* pos)
    {
        BitBoard rooks = pos->Pieces(colour_to_move, pt_Rook);
        if(!rooks) return 0;
        BitBoard attacks{0};

        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);

        while(rooks)
        {
            const U8 rook_index = Magics::PopNRetLS1B(rooks);
            attacks |= GetSlidingAttacks<File>(rook_index, us, them);
            attacks |= GetSlidingAttacks<Rank>(rook_index, us, them);
        }
        return attacks;
    }

    template<Colour colour_to_move>
    constexpr BitBoard QueenAttacks(Position const* pos)
    {
        BitBoard queens = pos->Pieces(colour_to_move, pt_Queen);
        if(!queens) return 0;

        BitBoard attacks{0};
        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);

        while(queens)
        {
            const U8 queen_index = Magics::PopNRetLS1B(queens);

            attacks |= GetSlidingAttacks<File          >(queen_index, us, them);
            attacks |= GetSlidingAttacks<Rank          >(queen_index, us, them);
            attacks |= GetSlidingAttacks<Diagonal      >(queen_index, us, them);
            attacks |= GetSlidingAttacks<AntiDiagonal  >(queen_index, us, them);
        }
        return attacks;
    }
    
    // ── Per-square attack detection ──
    template<Colour attacked_by>
    inline bool IsSquareAttacked(Position const* pos, Sq sq)
    {
        // --- Cheapest checks first ---
        const BitBoard sq_bb = Magics::SqToBB(sq);

        // Pawn attacks (2 shifts + 1 OR + 1 AND)
        const BitBoard pawns = pos->Pieces(attacked_by, pt_Pawn);
        if constexpr (attacked_by == White)
        {
            if ((Magics::Shift<SOUTH_EAST>(sq_bb) | Magics::Shift<SOUTH_WEST>(sq_bb)) & pawns) return true;
        }
        else
        {
            if ((Magics::Shift<NORTH_EAST>(sq_bb) | Magics::Shift<NORTH_WEST>(sq_bb)) & pawns) return true;
        }

        // Knight attacks (1 lookup + 1 AND)
        if (Magics::KNIGHT_ATTACK_MASKS[sq] & pos->Pieces(attacked_by, pt_Knight)) return true;

        // King attacks (1 lookup + 1 AND)
        if (Magics::KING_ATTACK_MASKS[sq] & pos->Pieces(attacked_by, pt_King)) return true;

        // --- Slider attacks with line-mask filtering ---
        const BitBoard us   = pos->Pieces(!attacked_by);
        const BitBoard them = pos->Pieces(attacked_by);

        const BitBoard bishop_queen = pos->Pieces(attacked_by, pt_Bishop, pt_Queen);
        if (bishop_queen & Magics::BISHOP_LINE_MASK[sq])
        {
            if ((GetSlidingAttacks<Diagonal>(sq, us, them) | GetSlidingAttacks<AntiDiagonal>(sq, us, them)) & bishop_queen) return true;
        }

        const BitBoard rook_queen = pos->Pieces(attacked_by, pt_Rook, pt_Queen);
        if (rook_queen & Magics::ROOK_LINE_MASK[sq])
        {
            if ((GetSlidingAttacks<File>(sq, us, them) | GetSlidingAttacks<Rank>(sq, us, them)) & rook_queen) return true;
        }

        return false;
    }

    template<Colour colour_to_move>
    bool InCheck(Position const* pos)
    {
        const Sq king_sq = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        return IsSquareAttacked<!colour_to_move>(pos, king_sq);
    }

    template<Colour colour_to_move>
    BitBoard GenerateAllAttacks(Position const* pos)
    {
        return (QueenAttacks<colour_to_move>(pos) | BishopAttacks<colour_to_move>(pos) |
                RookAttacks<colour_to_move>(pos)  | KnightAttacks<colour_to_move>(pos) |
                PawnAttacks<colour_to_move>(pos)  | KingAttacks<colour_to_move>(pos));
    }

    template<Colour C>
    constexpr void Castling(Position const* pos, MoveList* ml) noexcept
    {
        if (!((C == White ? 0x0C : 0x03) & pos->CastlingRights())) return;

        constexpr Sq king_sq = (C == White ? 4 : 60);
        if (IsSquareAttacked<!C>(pos, king_sq)) return;

        const BitBoard whole_board = pos->Pieces(Black, White);
        const U8 rank_occ = U8(C == White ? (whole_board & 0xFF) : whole_board >> 56);

        // Kingside
        if ((pos->CastlingRights() & (C == White ? 0x08 : 0x02))
            && !(rank_occ & 0x60)
            && !IsSquareAttacked<!C>(pos, Sq(C == White ? 5 : 61))
            && !IsSquareAttacked<!C>(pos, Sq(C == White ? 6 : 62)))
        {
            ml->add(Moves::EncodeMove(king_sq, C == White ? 6 : 62, mt_Castling));
        }

        // Queenside
        if ((pos->CastlingRights() & (C == White ? 0x04 : 0x01))
            && !(rank_occ & 0x0E)
            && !IsSquareAttacked<!C>(pos, Sq(C == White ? 3 : 59))
            && !IsSquareAttacked<!C>(pos, Sq(C == White ? 2 : 58)))
        {
            ml->add(Moves::EncodeMove(king_sq, C == White ? 2 : 58, mt_Castling));
        }
    }

    template<Colour colour_to_move>
    constexpr void GeneratePseudoLegalMoves(Position const* __restrict__  pos, MoveList* __restrict__ ml)
    {
        KingMoves<colour_to_move>(pos, ml);
        QueenMoves<colour_to_move>(pos, ml);
        BishopMoves<colour_to_move>(pos, ml);
        KnightMoves<colour_to_move>(pos, ml);
        RookMoves<colour_to_move>(pos, ml);
        (colour_to_move == White ? WhitePawnMoves(pos, ml) : BlackPawnMoves(pos, ml));
        Castling<colour_to_move>(pos, ml);
    }
    
    // ── Capture-only generation (for QSearch) ─────────────────────
    template<Colour C>
    constexpr void SlidingCaptures(Position const* pos, MoveList* ml, PieceType pt)
    {
        BitBoard pieces = pos->Pieces(C, pt);
        if (!pieces) return;

        const BitBoard us   = pos->Pieces(C);
        const BitBoard them = pos->Pieces(!C);

        while (pieces)
        {
            const Sq sq = Magics::PopNRetLS1B(pieces);
            BitBoard attacks = 0;

            if (pt == pt_Bishop || pt == pt_Queen)
            {
                attacks |= GetSlidingAttacks<Diagonal>(sq, us, them);
                attacks |= GetSlidingAttacks<AntiDiagonal>(sq, us, them);
            }
            if (pt == pt_Rook || pt == pt_Queen)
            {
                attacks |= GetSlidingAttacks<File>(sq, us, them);
                attacks |= GetSlidingAttacks<Rank>(sq, us, them);
            }

            GenerateMovesFromBB(attacks & them, ml, sq, mt_Capture);
        }
    }

    void WhitePawnCaptures(Position const* pos, MoveList* ml) noexcept;
    void BlackPawnCaptures(Position const* pos, MoveList* ml) noexcept;

    template<Colour C>
    constexpr void GeneratePseudoLegalCaptures(Position const* __restrict__ pos, MoveList* __restrict__ ml)
    {
        {
            const Sq king_sq = Magics::FindLS1B(pos->Pieces(C, pt_King));
            BitBoard caps = Magics::KING_ATTACK_MASKS[king_sq] & pos->Pieces(!C);
            GenerateMovesFromBB(caps, ml, king_sq, mt_Capture);
        }

        SlidingCaptures<C>(pos, ml, pt_Queen);
        SlidingCaptures<C>(pos, ml, pt_Bishop);
        SlidingCaptures<C>(pos, ml, pt_Rook);

        {
            BitBoard knights = pos->Pieces(C, pt_Knight);
            while (knights)
            {
                const Sq sq = Magics::PopNRetLS1B(knights);
                BitBoard caps = Magics::KNIGHT_ATTACK_MASKS[sq] & pos->Pieces(!C);
                GenerateMovesFromBB(caps, ml, sq, mt_Capture);
            }
        }

        (C == White ? WhitePawnCaptures(pos, ml) : BlackPawnCaptures(pos, ml));
    }

    template<Colour colour_to_move>
    void GenerateLegalMoves(Position* __restrict__  pos, MoveList* __restrict__  ml)
    {
        //pseudo legal
        MoveList pseudo_legal_ml;
        GeneratePseudoLegalMoves<colour_to_move>(pos, &pseudo_legal_ml);
        
        //filtering
        for(size_t i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos->MakeMove(pseudo_legal_ml[i]);

            //bc templates lol
            if(!InCheck<colour_to_move>(pos))
            {
                ml->add(pseudo_legal_ml[i]);
            }
            pos->UnmakeMove(pseudo_legal_ml[i]);
        }
    }
    // === OLD SYSTEM (for A/B benchmarking) ===
    template<AttackDirection direction, bool UsePext = USE_PEXT>
    inline INLINE move_info const* GetMovesForSliding(Sq piece_sq, BitBoard us, BitBoard them) noexcept
    {
        if constexpr(direction == Rank)
        {
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            const U8 file_bit = U8(1u << file_of_attacker);
            U8 us_collapsed{}, them_collapsed{};
            if constexpr (UsePext) {
                const BitBoard rank_mask = Magics::RANK_1BB << (piece_sq & 56);
                us_collapsed = U8(_pext_u64(us, rank_mask));
                them_collapsed = U8(_pext_u64(them, rank_mask));
            } else {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedFilesIndex(us & attack_mask);
                them_collapsed = Magics::CollapsedFilesIndex(them & attack_mask);
            }
            us_collapsed &= U8(~file_bit);
            them_collapsed &= U8(~file_bit);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, file_of_attacker);
            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else if constexpr(direction == File)
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{}, them_collapsed{};
            if constexpr (UsePext) {
                const BitBoard file_mask = Magics::FILE_ABB << file_of_attacker;
                us_collapsed = U8(_pext_u64(us, file_mask));
                them_collapsed = U8(_pext_u64(them, file_mask));
                const U8 rank_bit = U8(1u << rank_of_attacker);
                us_collapsed &= U8(~rank_bit);
                them_collapsed &= U8(~rank_bit);
            } else {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedRanksIndex(us & attack_mask, file_of_attacker);
                them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask, file_of_attacker);
            }
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{}, them_collapsed{};
            if constexpr (UsePext) {
                constexpr int diag_idx = (direction == Diagonal) ? 0 : 1;
                const BitBoard full_mask = Magics::DIAG_FULL_MASK[piece_sq][diag_idx];
                const U8 start_rank = (direction == Diagonal)
                    ? (rank_of_attacker > file_of_attacker ? U8(rank_of_attacker - file_of_attacker) : 0)
                    : ((rank_of_attacker + file_of_attacker > 7) ? U8(rank_of_attacker + file_of_attacker - 7) : 0);
                us_collapsed = U8(_pext_u64(us, full_mask) << start_rank);
                them_collapsed = U8(_pext_u64(them, full_mask) << start_rank);
                const U8 rank_bit = U8(1u << rank_of_attacker);
                us_collapsed &= U8(~rank_bit);
                them_collapsed &= U8(~rank_bit);
            } else {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                us_collapsed   = Magics::CollapsedRanksIndex(us & attack_mask);
                them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask);
            }
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
    }

    template<Colour C>
    constexpr void OldBishopMoves(Position const* pos, MoveList* ml)
    {
        BitBoard bishops = pos->Pieces(C, pt_Bishop);
        if(!bishops) return;
        const BitBoard us = pos->Pieces(C);
        const BitBoard them = pos->Pieces(!C);
        while(bishops) {
            const Sq sq = Magics::PopNRetLS1B(bishops);
            ml->merge(GetMovesForSliding<Diagonal>(sq, us, them));
            ml->merge(GetMovesForSliding<AntiDiagonal>(sq, us, them));
        }
    }
    template<Colour C>
    constexpr void OldRookMoves(Position const* pos, MoveList* ml)
    {
        BitBoard rooks = pos->Pieces(C, pt_Rook);
        if(!rooks) return;
        const BitBoard us = pos->Pieces(C);
        const BitBoard them = pos->Pieces(!C);
        while(rooks) {
            const Sq sq = Magics::PopNRetLS1B(rooks);
            ml->merge(GetMovesForSliding<File>(sq, us, them));
            ml->merge(GetMovesForSliding<Rank>(sq, us, them));
        }
    }
    template<Colour C>
    constexpr void OldQueenMoves(Position const* pos, MoveList* ml)
    {
        BitBoard queens = pos->Pieces(C, pt_Queen);
        if(!queens) return;
        const BitBoard us = pos->Pieces(C);
        const BitBoard them = pos->Pieces(!C);
        while(queens) {
            const Sq sq = Magics::PopNRetLS1B(queens);
            ml->merge(GetMovesForSliding<File>(sq, us, them));
            ml->merge(GetMovesForSliding<Rank>(sq, us, them));
            ml->merge(GetMovesForSliding<Diagonal>(sq, us, them));
            ml->merge(GetMovesForSliding<AntiDiagonal>(sq, us, them));
        }
    }
    template<Colour C>
    constexpr void OldGeneratePseudoLegalMoves(Position const* __restrict__ pos, MoveList* __restrict__ ml)
    {
        KingMoves<C>(pos, ml);
        OldQueenMoves<C>(pos, ml);
        OldBishopMoves<C>(pos, ml);
        KnightMoves<C>(pos, ml);
        OldRookMoves<C>(pos, ml);
        (C == White ? WhitePawnMoves(pos, ml) : BlackPawnMoves(pos, ml));
        Castling<C>(pos, ml);
    }
};

#endif // #ifndef MOVEGEN_HPP
