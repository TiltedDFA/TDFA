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
#include "Pext.hpp"
#include "MagicBitboards.hpp"

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

    template<SlidingGenType gen_type>
    inline constexpr bool IsTitboardsGen = (gen_type == Titboards || gen_type == TitboardsPext);

    template<SlidingGenType gen_type>
    inline constexpr bool TitboardUsesPext = (gen_type == TitboardsPext);

    template<AttackDirection direction, bool UsePext = USE_PEXT>
    inline INLINE move_info const* GetMovesForSliding(Sq piece_sq, BitBoard us, BitBoard them) noexcept
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
                const U8 rank_shift = piece_sq & 56;
                us_collapsed = U8((us >> rank_shift) & 0xFFu);
                them_collapsed = U8((them >> rank_shift) & 0xFFu);
            }
            us_collapsed &= U8(~file_bit);
            them_collapsed &= U8(~file_bit);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, file_of_attacker);
            assert(lookup_index <= 2187);

            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
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

            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else //direction == Diag || direction == Anti Diag
        {
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            U8 us_collapsed{};
            U8 them_collapsed{};
            if constexpr (UsePext)
            {
                const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
                const BitBoard full_mask = attack_mask | Magics::SqToBB(piece_sq);
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

            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
    }

    inline void GenerateMovesFromAttacks(BitBoard attacks, BitBoard occupied, BitBoard them, MoveList* ml, Sq from)
    {
        const BitBoard quiets = attacks & ~occupied;
        const BitBoard captures = attacks & them;
        GenerateMovesFromBB(quiets, ml, from, mt_Quiet);
        GenerateMovesFromBB(captures, ml, from, mt_Capture);
    }

    template<SlidingGenType gen_type>
    inline BitBoard BishopAttacksFrom(Sq piece_sq, BitBoard occupied) noexcept
    {
        if constexpr (gen_type == PextBoards)
        {
            return Pext::bishop_attacks(piece_sq, occupied);
        }
        else
        {
            return MagicBitBoards::bishopAttacks(piece_sq, occupied);
        }
    }

    template<SlidingGenType gen_type>
    inline BitBoard RookAttacksFrom(Sq piece_sq, BitBoard occupied) noexcept
    {
        if constexpr (gen_type == PextBoards)
        {
            return Pext::rook_attacks(piece_sq, occupied);
        }
        else
        {
            return MagicBitBoards::rookAttacks(piece_sq, occupied);
        }
    }

    template<SlidingGenType gen_type>
    inline BitBoard QueenAttacksFrom(Sq piece_sq, BitBoard occupied) noexcept
    {
        if constexpr (gen_type == PextBoards)
        {
            return Pext::queen_attacks(piece_sq, occupied);
        }
        else
        {
            return MagicBitBoards::queenAttacks(piece_sq, occupied);
        }
    }

    void WhitePawnMoves(Position const* pos, MoveList* ml) noexcept;

    void BlackPawnMoves(Position const* pos, MoveList* ml) noexcept;

    template<Colour colour_to_move, bool UsePext>
    constexpr void BishopMovesTitboards(Position const* pos, MoveList* ml)
    {
        BitBoard bishops = pos->Pieces(colour_to_move, pt_Bishop);
        if(!bishops) return;

        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);

        while(bishops)
        {
            const U8 bishop_index = Magics::PopNRetLS1B(bishops);

            move_info const* move = GetMovesForSliding<Diagonal, UsePext>(bishop_index, us, them);
            ml->merge(move);

            move = GetMovesForSliding<AntiDiagonal, UsePext>(bishop_index, us, them);
            ml->merge(move);
        }
    }

    template<Colour colour_to_move, bool UsePext>
    constexpr void RookMovesTitboards(Position const* pos, MoveList* ml)
    {
        BitBoard rooks = pos->Pieces(colour_to_move, pt_Rook);
        if(!rooks) return;

        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);
        
        while(rooks)
        {
            const U8 rook_index = Magics::PopNRetLS1B(rooks);

            move_info const* move = GetMovesForSliding<File, UsePext>(rook_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Rank, UsePext>(rook_index, us, them);
            ml->merge(move);
        }
    }

    template<Colour colour_to_move, bool UsePext>
    constexpr void QueenMovesTitboards(Position const* pos, MoveList* ml)
    {
        BitBoard queens = pos->Pieces(colour_to_move, pt_Queen);
        if(!queens) return;

        const BitBoard us = pos->Pieces(colour_to_move);
        const BitBoard them = pos->Pieces(!colour_to_move);

        while(queens)
        {
            const U8 queen_index = Magics::PopNRetLS1B(queens);
            
            move_info const* move = GetMovesForSliding<File, UsePext>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Rank, UsePext>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Diagonal, UsePext>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<AntiDiagonal, UsePext>(queen_index, us, them);
            ml->merge(move);
        }
    }

    template<Colour colour_to_move>
    constexpr void BishopMoves(Position const* pos, MoveList* ml)
    {
        BishopMovesTitboards<colour_to_move, USE_PEXT>(pos, ml);
    }
    
    template<Colour colour_to_move>
    constexpr void RookMoves(Position const* pos, MoveList* ml)
    {
        RookMovesTitboards<colour_to_move, USE_PEXT>(pos, ml);
    }

    template<Colour colour_to_move>
    constexpr void QueenMoves(Position const* pos, MoveList* ml)
    {
        QueenMovesTitboards<colour_to_move, USE_PEXT>(pos, ml);
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline void BishopMoves(Position const* pos, MoveList* ml)
    {
        if constexpr (IsTitboardsGen<gen_type>)
        {
            BishopMovesTitboards<colour_to_move, TitboardUsesPext<gen_type>>(pos, ml);
        }
        else
        {
            BitBoard bishops = pos->Pieces(colour_to_move, pt_Bishop);
            if(!bishops) return;

            const BitBoard occupied = pos->Pieces(White, Black);
            const BitBoard us = pos->Pieces(colour_to_move);
            const BitBoard them = pos->Pieces(!colour_to_move);

            while(bishops)
            {
                const Sq bishop_index = Magics::PopNRetLS1B(bishops);
                BitBoard attacks = BishopAttacksFrom<gen_type>(bishop_index, occupied) & ~us;
                GenerateMovesFromAttacks(attacks, occupied, them, ml, bishop_index);
            }
        }
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline void RookMoves(Position const* pos, MoveList* ml)
    {
        if constexpr (IsTitboardsGen<gen_type>)
        {
            RookMovesTitboards<colour_to_move, TitboardUsesPext<gen_type>>(pos, ml);
        }
        else
        {
            BitBoard rooks = pos->Pieces(colour_to_move, pt_Rook);
            if(!rooks) return;

            const BitBoard occupied = pos->Pieces(White, Black);
            const BitBoard us = pos->Pieces(colour_to_move);
            const BitBoard them = pos->Pieces(!colour_to_move);

            while(rooks)
            {
                const Sq rook_index = Magics::PopNRetLS1B(rooks);
                BitBoard attacks = RookAttacksFrom<gen_type>(rook_index, occupied) & ~us;
                GenerateMovesFromAttacks(attacks, occupied, them, ml, rook_index);
            }
        }
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline void QueenMoves(Position const* pos, MoveList* ml)
    {
        if constexpr (IsTitboardsGen<gen_type>)
        {
            QueenMovesTitboards<colour_to_move, TitboardUsesPext<gen_type>>(pos, ml);
        }
        else
        {
            BitBoard queens = pos->Pieces(colour_to_move, pt_Queen);
            if(!queens) return;

            const BitBoard occupied = pos->Pieces(White, Black);
            const BitBoard us = pos->Pieces(colour_to_move);
            const BitBoard them = pos->Pieces(!colour_to_move);

            while(queens)
            {
                const Sq queen_index = Magics::PopNRetLS1B(queens);
                BitBoard attacks = QueenAttacksFrom<gen_type>(queen_index, occupied) & ~us;
                GenerateMovesFromAttacks(attacks, occupied, them, ml, queen_index);
            }
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
            const BitBoard possible_quiet_move      = Magics::KNIGHT_ATTACK_MASKS[knight_index] & ~occupied;
            const BitBoard possible_capture_moves   = Magics::KNIGHT_ATTACK_MASKS[knight_index] & them;
            GenerateMovesFromBB(possible_quiet_move, ml, knight_index, mt_Quiet);
            GenerateMovesFromBB(possible_capture_moves, ml, knight_index, mt_Capture);
        }
    }

    template<Colour colour_to_move>
    void KingMoves(Position const* pos, MoveList* ml) 
    {
        const U8 king_index = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        const BitBoard occupied = pos->Pieces(White, Black);
        const BitBoard them = pos->Pieces(!colour_to_move);
        const BitBoard possible_quiet_move      = Magics::KING_ATTACK_MASKS[king_index] & ~occupied;
        const BitBoard possible_capture_moves   = Magics::KING_ATTACK_MASKS[king_index] & them;
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

            attacks |= GetMovesForSliding<Diagonal      >(bishop_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<AntiDiagonal  >(bishop_index, us, them)->attacks_;
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
            attacks |= GetMovesForSliding<File>(rook_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Rank>(rook_index, us, them)->attacks_;
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
            
            attacks |= GetMovesForSliding<File          >(queen_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Rank          >(queen_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Diagonal      >(queen_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<AntiDiagonal  >(queen_index, us, them)->attacks_;
        }
        return attacks;
    }
    
    template<Colour colour_to_move, bool UsePext>
    inline bool SquareAttackedTitboards(Position const* pos, const Sq sq)
    {
        const BitBoard us = pos->Pieces(!colour_to_move);
        const BitBoard them = pos->Pieces(colour_to_move);
        const BitBoard bishop_queen = pos->Pieces(colour_to_move, pt_Bishop, pt_Queen);
        const BitBoard rook_queen = pos->Pieces(colour_to_move, pt_Rook, pt_Queen);
        const BitBoard sq_bb = Magics::SqToBB(sq);

        if(GetMovesForSliding<Diagonal, UsePext>(sq, us, them)->attacks_ & bishop_queen) return true;
        if(GetMovesForSliding<AntiDiagonal, UsePext>(sq, us, them)->attacks_ & bishop_queen) return true;
        if(GetMovesForSliding<File, UsePext>(sq, us, them)->attacks_ & rook_queen) return true;
        if(GetMovesForSliding<Rank, UsePext>(sq, us, them)->attacks_ & rook_queen) return true;

        if(Magics::KNIGHT_ATTACK_MASKS[sq] & pos->Pieces(colour_to_move, pt_Knight)) return true;

        const BitBoard pawns = pos->Pieces(colour_to_move, pt_Pawn);
        if constexpr (colour_to_move == White)
        {
            if(Magics::Shift<NORTH_EAST>(pawns) & sq_bb) return true;
            if(Magics::Shift<NORTH_WEST>(pawns) & sq_bb) return true;
        }
        else
        {
            if(Magics::Shift<SOUTH_EAST>(pawns) & sq_bb) return true;
            if(Magics::Shift<SOUTH_WEST>(pawns) & sq_bb) return true;
        }

        return (Magics::KING_ATTACK_MASKS[sq] & pos->Pieces(colour_to_move, pt_King));
    }

    template<Colour colour_to_move>
    inline bool SquareAttacked(Position const* pos, const Sq sq)
    {
        return SquareAttackedTitboards<colour_to_move, USE_PEXT>(pos, sq);
    }

    template<Colour colour_to_move>
    inline bool InCheck(Position const* pos)
    {
        const Sq king_sq = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        return SquareAttacked<!colour_to_move>(pos, king_sq);
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline bool SquareAttacked(Position const* pos, const Sq sq)
    {
        if constexpr (IsTitboardsGen<gen_type>)
        {
            return SquareAttackedTitboards<colour_to_move, TitboardUsesPext<gen_type>>(pos, sq);
        }

        const BitBoard occupied = pos->Pieces(White, Black);
        const BitBoard bishop_queen = pos->Pieces(colour_to_move, pt_Bishop, pt_Queen);
        const BitBoard rook_queen = pos->Pieces(colour_to_move, pt_Rook, pt_Queen);

        if(BishopAttacksFrom<gen_type>(sq, occupied) & bishop_queen) return true;
        if(RookAttacksFrom<gen_type>(sq, occupied) & rook_queen) return true;

        if(Magics::KNIGHT_ATTACK_MASKS[sq] & pos->Pieces(colour_to_move, pt_Knight)) return true;

        const BitBoard pawns = pos->Pieces(colour_to_move, pt_Pawn);
        const BitBoard sq_bb = Magics::SqToBB(sq);
        if constexpr (colour_to_move == White)
        {
            if(Magics::Shift<NORTH_EAST>(pawns) & sq_bb) return true;
            if(Magics::Shift<NORTH_WEST>(pawns) & sq_bb) return true;
        }
        else
        {
            if(Magics::Shift<SOUTH_EAST>(pawns) & sq_bb) return true;
            if(Magics::Shift<SOUTH_WEST>(pawns) & sq_bb) return true;
        }

        return (Magics::KING_ATTACK_MASKS[sq] & pos->Pieces(colour_to_move, pt_King));
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline bool InCheck(Position const* pos)
    {
        const Sq king_sq = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        return SquareAttacked<!colour_to_move, gen_type>(pos, king_sq);
    }

    template<Colour colour_to_move>
    BitBoard GenerateAllAttacks(Position const* pos)
    {
        return (QueenAttacks<colour_to_move>(pos) | BishopAttacks<colour_to_move>(pos) |
                RookAttacks<colour_to_move>(pos)  | KnightAttacks<colour_to_move>(pos) |
                PawnAttacks<colour_to_move>(pos)  | KingAttacks<colour_to_move>(pos));
    }

    template<Colour colour_to_move>
    constexpr void Castling(Position const* pos, MoveList* ml) noexcept
    {
        if(!((colour_to_move == White ? 0x0C : 0x03) & pos->CastlingRights())) {return;} //checks for castling rights
        const BitBoard whole_board = pos->Pieces(Black, White);
        const U8 king_index = (colour_to_move == White  ? 4 : 60);
        const U8 rank_looked_at = U8(colour_to_move == White  ? (whole_board & 0xFF) : whole_board >> 56);

        const U8 rights = pos->CastlingRights();
        const bool can_kingside =
            (rights & (colour_to_move == White  ? 0x08 : 0x02)) &&
            !(rank_looked_at & 0x60);
        const bool can_queenside =
            (rights & (colour_to_move == White  ? 0x04 : 0x01)) &&
            !(rank_looked_at & 0x0E);

        if(!can_kingside && !can_queenside) return;
        if(InCheck<colour_to_move>(pos)) {return;} //checks if king under attack

        if(can_kingside)
        {
            const Sq s1 = Sq(king_index + 1);
            const Sq s2 = Sq(king_index + 2);
            if(!SquareAttacked<!colour_to_move>(pos, s1) && !SquareAttacked<!colour_to_move>(pos, s2))
            {
                if constexpr(colour_to_move == White)
                    ml->add(Moves::EncodeMove(king_index, 6, mt_Castling));
                else
                    ml->add(Moves::EncodeMove(king_index, 62, mt_Castling));
            }
        }
        if(can_queenside)
        {
            const Sq s1 = Sq(king_index - 1);
            const Sq s2 = Sq(king_index - 2);
            if(!SquareAttacked<!colour_to_move>(pos, s1) && !SquareAttacked<!colour_to_move>(pos, s2))
            {
                if constexpr(colour_to_move == White)
                    ml->add(Moves::EncodeMove(king_index, 2, mt_Castling));
                else
                    ml->add(Moves::EncodeMove(king_index, 58, mt_Castling));
            }
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

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline void Castling(Position const* pos, MoveList* ml) noexcept
    {
        if(!((colour_to_move == White ? 0x0C : 0x03) & pos->CastlingRights())) {return;}
        const BitBoard whole_board = pos->Pieces(Black, White);
        const U8 king_index = (colour_to_move == White  ? 4 : 60);
        const U8 rank_looked_at = U8(colour_to_move == White  ? (whole_board & 0xFF) : whole_board >> 56);

        const U8 rights = pos->CastlingRights();
        const bool can_kingside =
            (rights & (colour_to_move == White  ? 0x08 : 0x02)) &&
            !(rank_looked_at & 0x60);
        const bool can_queenside =
            (rights & (colour_to_move == White  ? 0x04 : 0x01)) &&
            !(rank_looked_at & 0x0E);

        if(!can_kingside && !can_queenside) return;
        if(InCheck<colour_to_move, gen_type>(pos)) {return;}

        if(can_kingside)
        {
            const Sq s1 = Sq(king_index + 1);
            const Sq s2 = Sq(king_index + 2);
            if(!SquareAttacked<!colour_to_move, gen_type>(pos, s1) &&
               !SquareAttacked<!colour_to_move, gen_type>(pos, s2))
            {
                if constexpr(colour_to_move == White)
                    ml->add(Moves::EncodeMove(king_index, 6, mt_Castling));
                else
                    ml->add(Moves::EncodeMove(king_index, 62, mt_Castling));
            }
        }
        if(can_queenside)
        {
            const Sq s1 = Sq(king_index - 1);
            const Sq s2 = Sq(king_index - 2);
            if(!SquareAttacked<!colour_to_move, gen_type>(pos, s1) &&
               !SquareAttacked<!colour_to_move, gen_type>(pos, s2))
            {
                if constexpr(colour_to_move == White)
                    ml->add(Moves::EncodeMove(king_index, 2, mt_Castling));
                else
                    ml->add(Moves::EncodeMove(king_index, 58, mt_Castling));
            }
        }
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    inline void GeneratePseudoLegalMoves(Position const* __restrict__ pos, MoveList* __restrict__ ml)
    {
        KingMoves<colour_to_move>(pos, ml);
        QueenMoves<colour_to_move, gen_type>(pos, ml);
        BishopMoves<colour_to_move, gen_type>(pos, ml);
        KnightMoves<colour_to_move>(pos, ml);
        RookMoves<colour_to_move, gen_type>(pos, ml);
        (colour_to_move == White ? WhitePawnMoves(pos, ml) : BlackPawnMoves(pos, ml));
        Castling<colour_to_move, gen_type>(pos, ml);
    }

    template<Colour colour_to_move, SlidingGenType gen_type>
    void GenerateLegalMoves(Position* __restrict__ pos, MoveList* __restrict__ ml)
    {
        MoveList pseudo_legal_ml;
        GeneratePseudoLegalMoves<colour_to_move, gen_type>(pos, &pseudo_legal_ml);

        for(size_t i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos->MakeMove(pseudo_legal_ml[i]);
            if(!InCheck<colour_to_move, gen_type>(pos))
            {
                ml->add(pseudo_legal_ml[i]);
            }
            pos->UnmakeMove(pseudo_legal_ml[i]);
        }
    }
};

#endif // #ifndef MOVEGEN_HPP
