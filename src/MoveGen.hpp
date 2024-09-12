#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "BitBoard.hpp"
#include "Move.hpp"
#include "MoveList.hpp"

extern std::array<std::array<std::array<move_info, 2187>, 4>, 64> SLIDING_ATTACK_CONFIG;
namespace MoveGen
{
    constexpr void GenerateMovesFromBB(BitBoard b, MoveList* ml, const Sq from, const PieceType type)
    {
        while(b)
        {
            ml->add(Moves::EncodeMove(from, Magics::FindLS1B(b), type));
            b = Magics::PopLS1B(b);
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

    template<bool is_white>
    constexpr void BishopMoves(Position const* pos, MoveList* ml)
    {
        BitBoard bishops = pos->Pieces<is_white, Bishop>();
        if(!bishops) return;

        const BitBoard us = pos->PiecesByColour<is_white>();
        const BitBoard them = pos->PiecesByColour<!is_white>();

        while(bishops)
        {
            const U8 bishop_index = Magics::FindLS1B(bishops);

            move_info const* move = GetMovesForSliding<Diagonal>(bishop_index, us, them);
            for(U8 i{0}; i < move->count_; ++i)
                ml->add(move->encoded_move_[i]);
            
            move = GetMovesForSliding<AntiDiagonal>(bishop_index, us, them);
            for(U8 i{0}; i < move->count_; ++i)
                ml->add(move->encoded_move_[i]);
            
            bishops = Magics::PopLS1B(bishops);
        }
    }
    
    template<bool is_white>
    constexpr void RookMoves(Position const* pos, MoveList* ml)
    {
        BitBoard rooks = pos->Pieces<is_white, Rook>();
        if(!rooks) return;

        const BitBoard us = pos->PiecesByColour<is_white>();
        const BitBoard them = pos->PiecesByColour<!is_white>();
        
        while(rooks)
        {
            const U8 rook_index = Magics::FindLS1B(rooks);

            move_info const* move = GetMovesForSliding<File>(rook_index, us, them);
            for(U8 i{0}; i < move->count_; ++i) 
                ml->add(move->encoded_move_[i]);
            
            move = GetMovesForSliding<Rank>(rook_index, us, them);
            for(U8 i{0}; i < move->count_; ++i) 
                ml->add(move->encoded_move_[i]);
            
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<bool is_white>
    constexpr void QueenMoves(Position const* pos, MoveList* ml)
    {
        BitBoard queens = pos->Pieces<is_white, Queen>();
        if(!queens) return;

        const BitBoard us = pos->PiecesByColour<is_white>();
        const BitBoard them = pos->PiecesByColour<!is_white>();

        while(queens)
        {
            const U8 queen_index = Magics::FindLS1B(queens);
            
            move_info const* move = GetMovesForSliding<File>(queen_index, us, them);
            for(U8 i{0}; i < move->count_; ++i)
                ml->add(Moves::SetPieceType<Queen>(move->encoded_move_[i]));
            
            move = GetMovesForSliding<Rank>(queen_index, us, them);
            for(U8 i{0}; i < move->count_; ++i)
                ml->add(Moves::SetPieceType<Queen>(move->encoded_move_[i]));
            
            move = GetMovesForSliding<Diagonal>(queen_index, us, them);
            for(U8 i{0}; i < move->count_; ++i)
                ml->add(Moves::SetPieceType<Queen>(move->encoded_move_[i]));
            
            move = GetMovesForSliding<AntiDiagonal>(queen_index, us, them);
            for(U8 i{0}; i < move->count_; ++i)
                ml->add(Moves::SetPieceType<Queen>(move->encoded_move_[i]));
            
            queens = Magics::PopLS1B(queens);
        }
    }

    template<bool is_white>
    constexpr void KnightMoves(Position const* pos, MoveList* ml)
    {
        BitBoard knights = pos->Pieces<is_white, Knight>();
        if(!knights) return;
        while(knights)
        {
            const U8 knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & ~pos->PiecesByColour<is_white>();
            GenerateMovesFromBB(possible_move, ml, knight_index, Knight);
            knights = Magics::PopLS1B(knights);
        }
    }

    template<bool is_white>
    void KingMoves(Position const* pos, MoveList* ml) 
    {
        const U8 king_index = Magics::FindLS1B(pos->Pieces<is_white, King>());
        BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & ~pos->PiecesByColour<is_white>();
        GenerateMovesFromBB(king_attacks, ml, king_index, King);
    }

    template<bool is_white>
    constexpr BitBoard PawnAttacks(Position const* pos)
    {
        const BitBoard pawns = pos->Pieces<is_white, Pawn>();
        if(!pawns) return 0;
        if constexpr(is_white)
        {
            return (Magics::Shift<NORTH_EAST>(pawns) | Magics::Shift<NORTH_WEST>(pawns)) & ~pos->PiecesByColour<true>();
        }
        return (Magics::Shift<SOUTH_EAST>(pawns) | Magics::Shift<SOUTH_WEST>(pawns)) & ~pos->PiecesByColour<false>();
    }
    
    template<bool is_white>
    constexpr BitBoard KingAttacks(Position const* pos)
    {
        const BitBoard king_index = Magics::FindLS1B(pos->Pieces<is_white, King>());
        return (Magics::KING_ATTACK_MASKS[king_index] & ~pos->PiecesByColour<is_white>());
    }
    
    template<bool is_white>
    constexpr BitBoard KnightAttacks(Position const* pos)
    {
        BitBoard knights = pos->Pieces<is_white, Knight>();
        if(!knights) return 0;

        BitBoard attacks{0};
        const BitBoard them = ~pos->PiecesByColour<is_white>();

        while(knights)
        {
            attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & them;
            knights = Magics::PopLS1B(knights);
        }
        return attacks;
    }
    
    template<bool is_white>
    constexpr BitBoard BishopAttacks(Position const* pos)
    {
        BitBoard bishops = pos->Pieces<is_white, Bishop>();
        if(!bishops) return 0;

        BitBoard attacks{0};
        const BitBoard us = pos->PiecesByColour<is_white>();
        const BitBoard them = pos->PiecesByColour<!is_white>();

        while(bishops)
        {
            const U8 bishop_index = Magics::FindLS1B(bishops);

            attacks |= GetMovesForSliding<Diagonal      >(bishop_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<AntiDiagonal  >(bishop_index, us, them)->attacks_;
            
            bishops = Magics::PopLS1B(bishops);
        }
        return attacks;
    }

    template<bool is_white>
    constexpr BitBoard RookAttacks(Position const* pos)
    {
        BitBoard rooks = pos->Pieces<is_white, Rook>();
        if(!rooks) return 0;
        BitBoard attacks{0};

        const BitBoard us = pos->PiecesByColour<is_white>();
        const BitBoard them = pos->PiecesByColour<!is_white>();

        while(rooks)
        {
            const U8 rook_index = Magics::FindLS1B(rooks);
            attacks |= GetMovesForSliding<File>(rook_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Rank>(rook_index, us, them)->attacks_;
            rooks = Magics::PopLS1B(rooks);
        }
        return attacks;
    }

    template<bool is_white>
    constexpr BitBoard QueenAttacks(Position const* pos)
    {
        BitBoard queens = pos->Pieces<is_white, Queen>();
        if(!queens) return 0;

        BitBoard attacks{0};
        const BitBoard us   = pos->PiecesByColour<is_white>();
        const BitBoard them = pos->PiecesByColour<!is_white>();

        while(queens)
        {
            const U8 queen_index = Magics::FindLS1B(queens);
            
            attacks |= GetMovesForSliding<File          >(queen_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Rank          >(queen_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Diagonal      >(queen_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<AntiDiagonal  >(queen_index, us, them)->attacks_;

            queens = Magics::PopLS1B(queens);
        }
        return attacks;
    }
    
    template<bool is_white>
    bool InCheck(Position const* pos)
    {
        const BitBoard our_king = pos->Pieces<is_white, King>();
        // us, them are variables used for sliding move gen with titboards.
        //since we want to generate moves for the opponent and see if they attack
        //our king we want the us and them variables to be inverted from our king in colour
        const BitBoard us   = pos->PiecesByColour<!is_white>();
        const BitBoard them = pos->PiecesByColour<is_white>();

        //Bishop and half queen
        BitBoard bishop_queen = pos->Pieces<!is_white, Queen>() | pos->Pieces<!is_white, Bishop>();
        while (bishop_queen)
        {
            const Sq piece_index = Magics::FindLS1B(bishop_queen);
            if(our_king & GetMovesForSliding<Diagonal       >(piece_index, us, them)->attacks_) return true;
            if(our_king & GetMovesForSliding<AntiDiagonal   >(piece_index, us, them)->attacks_) return true;
            bishop_queen = Magics::PopLS1B(bishop_queen);
        }
        
        //rook and other half of queen
        BitBoard rook_queen = pos->Pieces<!is_white, Queen>() | pos->Pieces<!is_white, Rook>();
        while(rook_queen)
        {
            const Sq piece_index = Magics::FindLS1B(rook_queen);

            if(our_king & GetMovesForSliding<File>(piece_index, us, them)->attacks_) return true;
            if(our_king & GetMovesForSliding<Rank>(piece_index, us, them)->attacks_) return true;

            rook_queen = Magics::PopLS1B(rook_queen);
        }

        //knights
        BitBoard knights = pos->Pieces<!is_white, Knight>();
        while(knights)
        {
            if(our_king & Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)]) return true;
            knights = Magics::PopLS1B(knights);
        }

        //pawns
        const BitBoard them_pawns = pos->Pieces<!is_white, Pawn>();
        if constexpr (is_white)
        {
            if(our_king & Magics::Shift<SOUTH_EAST>(them_pawns)) return true;
            if(our_king & Magics::Shift<SOUTH_WEST>(them_pawns)) return true;
        }
        else
        {
            if(our_king & Magics::Shift<NORTH_EAST>(them_pawns)) return true;
            if(our_king & Magics::Shift<NORTH_WEST>(them_pawns)) return true;
        }

        // king attacks
        return (our_king & Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos->Pieces<!is_white, King>())]);
    }

    template<bool is_white>
    BitBoard GenerateAllAttacks(Position const* pos)
    {
        return (QueenAttacks<is_white>(pos) | BishopAttacks<is_white>(pos) |
                RookAttacks<is_white>(pos)  | KnightAttacks<is_white>(pos) |
                PawnAttacks<is_white>(pos)  | KingAttacks<is_white>(pos));
    }

    template<bool is_white>
    constexpr void Castling(Position const* pos, MoveList* ml) noexcept
    {
        if(!((is_white ? 0x0C : 0x03) & pos->CastlingRights())) {return;} //checks for castling rights
        if(InCheck<is_white>(pos)) {return;} //checks if king under attack

        const BitBoard enemy_attacks = GenerateAllAttacks<!is_white>(pos);

        const BitBoard whole_board = pos->PiecesByColour<true>() | pos->PiecesByColour<false>();
        const U8 king_index = (is_white ? 4 : 60);
        const U8 rank_looked_at = U8(is_white ? (whole_board & 0xFF) : whole_board >> 56);

        if // kingside
        (
            (pos->CastlingRights() & (is_white ? 0x08 : 0x02)) // has rights
            &&
            !(rank_looked_at & 0x60) // not blocked 01100000 10010001
            &&
            !(0xFF & (is_white ? enemy_attacks : enemy_attacks >> 56) & 0x60) // not under attack by enemy
        )
        {
            if constexpr(is_white)
                ml->add(Moves::EncodeMove(king_index, 6, King));
            else
                ml->add(Moves::EncodeMove(king_index, 62, King));

        }
        if //queenside
        (
            (pos->CastlingRights() & (is_white ? 0x04 : 0x01)) // has rights
            &&
            !(rank_looked_at & 0x0E) // not blocked
            &&
            !(0xFF & (is_white ? enemy_attacks : enemy_attacks >> 56) & 0x0C) // not under attack by enemy
        )
        {
            if constexpr(is_white)
                ml->add(Moves::EncodeMove(king_index, 2, King));
            else
                ml->add(Moves::EncodeMove(king_index, 58, King));
        }
    }

    template<bool is_white>
    constexpr void GeneratePseudoLegalMoves(Position const* __restrict__  pos, MoveList* __restrict__ ml)
    {
        KingMoves<is_white>(pos, ml);
        QueenMoves<is_white>(pos, ml);
        BishopMoves<is_white>(pos, ml);
        KnightMoves<is_white>(pos, ml);
        RookMoves<is_white>(pos, ml);
        (is_white ? WhitePawnMoves(pos, ml) : BlackPawnMoves(pos, ml));
        Castling<is_white>(pos, ml);
    }
    
    template<bool is_white>
    void GenerateLegalMoves(Position* __restrict__  pos, MoveList* __restrict__  ml)
    {
        //pseudo legal
        MoveList pseudo_legal_ml;
        GeneratePseudoLegalMoves<is_white>(pos, &pseudo_legal_ml);
        
        //filtering
        for(size_t i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos->MakeMove(pseudo_legal_ml[i]);

            if(!InCheck<is_white>(pos))
            {
                ml->add(pseudo_legal_ml[i]);
            }
            pos->UnmakeMove(pseudo_legal_ml[i]);
        }
    }
};

#endif // #ifndef MOVEGEN_HPP
