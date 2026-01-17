#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveList.hpp"

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

    template<AttackDirection direction>
    constexpr move_info const* GetMovesForSliding(Sq piece_sq, BitBoard us, BitBoard them) noexcept
    {
        if constexpr(direction == Rank)
        {
            const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            const U8 us_collapsed   = Magics::CollapsedFilesIndex(us   & attack_mask);
            const U8 them_collapsed = Magics::CollapsedFilesIndex(them & attack_mask);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, file_of_attacker);
            assert(lookup_index <= 2187);

            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else if constexpr(direction == File)
        {
            const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 file_of_attacker = Magics::FileOf(piece_sq);
            const U8 us_collapsed   = Magics::CollapsedRanksIndex(us   & attack_mask, file_of_attacker);
            const U8 them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask, file_of_attacker);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            assert(lookup_index <= 2187);

            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
        }
        else //direction == Diag || direction == Anti Diag
        {
            const BitBoard attack_mask = Magics::SLIDING_ATTACKS_MASK[piece_sq][direction];
            const U8 rank_of_attacker = Magics::RankOf(piece_sq);
            const U8 us_collapsed   = Magics::CollapsedRanksIndex(us   & attack_mask);
            const U8 them_collapsed = Magics::CollapsedRanksIndex(them & attack_mask);
            const U16 lookup_index = Magics::GetBaseThreeUsThem(us_collapsed, them_collapsed, rank_of_attacker);
            assert(lookup_index <= 2187);

            return &SLIDING_ATTACK_CONFIG _AT(piece_sq)_AT(direction)_AT(lookup_index);
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
            const U8 bishop_index = Magics::PopNRetLS1B(bishops);

            move_info const* move = GetMovesForSliding<Diagonal>(bishop_index, us, them);
            ml->merge(move);

            move = GetMovesForSliding<AntiDiagonal>(bishop_index, us, them);
            ml->merge(move);
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
            const U8 rook_index = Magics::PopNRetLS1B(rooks);

            move_info const* move = GetMovesForSliding<File>(rook_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Rank>(rook_index, us, them);
            ml->merge(move);
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
            const U8 queen_index = Magics::PopNRetLS1B(queens);
            
            move_info const* move = GetMovesForSliding<File>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Rank>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Diagonal>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<AntiDiagonal>(queen_index, us, them);
            ml->merge(move);
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
    
    template<Colour colour_to_move>
    inline bool SquareAttacked(Position const* pos, const Sq sq)
    {
        const BitBoard us = pos->Pieces(!colour_to_move);
        const BitBoard them = pos->Pieces(colour_to_move);
        const BitBoard bishop_queen = pos->Pieces(colour_to_move, pt_Bishop, pt_Queen);
        const BitBoard rook_queen = pos->Pieces(colour_to_move, pt_Rook, pt_Queen);
        const BitBoard sq_bb = Magics::SqToBB(sq);

        if(GetMovesForSliding<Diagonal>(sq, us, them)->attacks_ & bishop_queen) return true;
        if(GetMovesForSliding<AntiDiagonal>(sq, us, them)->attacks_ & bishop_queen) return true;
        if(GetMovesForSliding<File>(sq, us, them)->attacks_ & rook_queen) return true;
        if(GetMovesForSliding<Rank>(sq, us, them)->attacks_ & rook_queen) return true;

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
    inline bool InCheck(Position const* pos)
    {
        const Sq king_sq = Magics::FindLS1B(pos->Pieces(colour_to_move, pt_King));
        return SquareAttacked<!colour_to_move>(pos, king_sq);
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
};

#endif // #ifndef MOVEGEN_HPP
