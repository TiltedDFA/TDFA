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
            ml->add(Moves::EncodeMove(from, Magics::FindLS1B(b), type));
            b = Magics::PopLS1B(b);
        }
    }

    template<AttackDirection direction>
    constexpr move_info const* GetMovesForSliding(Sq piece_sq, BitBoard us, BitBoard them) 
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

    void WhitePawnMoves(Position const* pos, MoveList* ml) ;

    void BlackPawnMoves(Position const* pos, MoveList* ml) ;

    template<Colour current_turn>
    constexpr void BishopMoves(Position const* pos, MoveList* ml)
    {
        BitBoard bishops = pos->Pieces(current_turn, pt_Bishop);
        if(!bishops) return;

        const BitBoard us = pos->Pieces(current_turn);
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));

        while(bishops)
        {
            const U8 bishop_index = Magics::FindLS1B(bishops);

            move_info const* move = GetMovesForSliding<Diagonal>(bishop_index, us, them);
            ml->merge(move);

            move = GetMovesForSliding<AntiDiagonal>(bishop_index, us, them);
            ml->merge(move);
            
            bishops = Magics::PopLS1B(bishops);
        }
    }
    
    template<Colour current_turn>
    constexpr void RookMoves(Position const* pos, MoveList* ml)
    {
        BitBoard rooks = pos->Pieces(current_turn, pt_Rook);
        if(!rooks) return;

        const BitBoard us = pos->Pieces(current_turn);
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));
        
        while(rooks)
        {
            const U8 rook_index = Magics::FindLS1B(rooks);

            move_info const* move = GetMovesForSliding<File>(rook_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Rank>(rook_index, us, them);
            ml->merge(move);
            
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<Colour current_turn>
    constexpr void QueenMoves(Position const* pos, MoveList* ml)
    {
        BitBoard queens = pos->Pieces(current_turn, pt_Queen);
        if(!queens) return;

        const BitBoard us = pos->Pieces(current_turn);
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));

        while(queens)
        {
            const U8 queen_index = Magics::FindLS1B(queens);
            
            move_info const* move = GetMovesForSliding<File>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Rank>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<Diagonal>(queen_index, us, them);
            ml->merge(move);
            
            move = GetMovesForSliding<AntiDiagonal>(queen_index, us, them);
            ml->merge(move);
            
            queens = Magics::PopLS1B(queens);
        }
    }

    template<Colour current_turn>
    constexpr void KnightMoves(Position const* pos, MoveList* ml)
    {
        BitBoard knights = pos->Pieces(current_turn, pt_Knight);
        if(!knights) return;
        while(knights)
        {
            const U8 knight_index = Magics::FindLS1B(knights);
            const BitBoard possible_quiet_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & ~(pos->Pieces(current_turn) | pos->Pieces(Colour(current_turn ^ Black)));
            const BitBoard possible_capture_moves = Magics::KNIGHT_ATTACK_MASKS[knight_index] & pos->Pieces(Colour(current_turn ^ Black));
            GenerateMovesFromBB(possible_quiet_move, ml, knight_index, mt_Quiet);
            GenerateMovesFromBB(possible_capture_moves, ml, knight_index, mt_Capture);
            knights = Magics::PopLS1B(knights);
        }
    }

    template<Colour current_turn>
    void KingMoves(Position const* pos, MoveList* ml) 
    {
        const U8 king_index = Magics::FindLS1B(pos->Pieces(current_turn, pt_King));
        const BitBoard possible_quiet_move = Magics::KING_ATTACK_MASKS[king_index] & ~(pos->Pieces(current_turn) | pos->Pieces(Colour(current_turn ^ Black)));
        const BitBoard possible_capture_moves = Magics::KING_ATTACK_MASKS[king_index] & pos->Pieces(Colour(current_turn ^ Black));
        GenerateMovesFromBB(possible_quiet_move, ml, king_index, mt_Quiet);
        GenerateMovesFromBB(possible_capture_moves, ml, king_index, mt_Capture);
    }

    template<Colour current_turn>
    constexpr BitBoard PawnAttacks(Position const* pos)
    {
        const BitBoard pawns = pos->Pieces(current_turn, pt_Pawn);
        if(!pawns) return 0;
        if constexpr(current_turn == White)
        {
            return (Magics::Shift<NORTH_EAST>(pawns) | Magics::Shift<NORTH_WEST>(pawns)) & ~pos->Pieces(White);
        }
        return (Magics::Shift<SOUTH_EAST>(pawns) | Magics::Shift<SOUTH_WEST>(pawns)) & ~pos->Pieces(Black);
    }
    
    template<Colour current_turn>
    constexpr BitBoard KingAttacks(Position const* pos)
    {
        const BitBoard king_index = Magics::FindLS1B(pos->Pieces(current_turn, pt_King));
        return (Magics::KING_ATTACK_MASKS[king_index] & ~pos->Pieces(current_turn));
    }
    
    template<Colour current_turn>
    constexpr BitBoard KnightAttacks(Position const* pos)
    {
        BitBoard knights = pos->Pieces(current_turn, pt_Knight);
        if(!knights) return 0;

        BitBoard attacks{0};
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));

        while(knights)
        {
            attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & them;
            knights = Magics::PopLS1B(knights);
        }
        return attacks;
    }
    
    template<Colour current_turn>
    constexpr BitBoard BishopAttacks(Position const* pos)
    {
        BitBoard bishops = pos->Pieces(current_turn, pt_Bishop);
        if(!bishops) return 0;

        BitBoard attacks{0};
        const BitBoard us = pos->Pieces(current_turn);
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));

        while(bishops)
        {
            const U8 bishop_index = Magics::FindLS1B(bishops);

            attacks |= GetMovesForSliding<Diagonal      >(bishop_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<AntiDiagonal  >(bishop_index, us, them)->attacks_;
            
            bishops = Magics::PopLS1B(bishops);
        }
        return attacks;
    }

    template<Colour current_turn>
    constexpr BitBoard RookAttacks(Position const* pos)
    {
        BitBoard rooks = pos->Pieces(current_turn, pt_Rook);
        if(!rooks) return 0;
        BitBoard attacks{0};

        const BitBoard us = pos->Pieces(current_turn);
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));

        while(rooks)
        {
            const U8 rook_index = Magics::FindLS1B(rooks);
            attacks |= GetMovesForSliding<File>(rook_index, us, them)->attacks_;
            attacks |= GetMovesForSliding<Rank>(rook_index, us, them)->attacks_;
            rooks = Magics::PopLS1B(rooks);
        }
        return attacks;
    }

    template<Colour current_turn>
    constexpr BitBoard QueenAttacks(Position const* pos)
    {
        BitBoard queens = pos->Pieces(current_turn, pt_Queen);
        if(!queens) return 0;

        BitBoard attacks{0};
        const BitBoard us = pos->Pieces(current_turn);
        const BitBoard them = pos->Pieces(Colour(current_turn ^ Black));

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
    
    template<Colour current_turn>
    bool InCheck(Position const* pos)
    {
        const BitBoard our_king = pos->Pieces(current_turn, pt_King);
        // us, them are variables used for sliding move gen with titboards.
        //since we want to generate moves for the opponent and see if they attack
        //our king we want the us and them variables to be inverted from our king in colour
        const BitBoard us = pos->Pieces(Colour(current_turn ^ Black));
        const BitBoard them = pos->Pieces(current_turn);

        //Bishop and half queen
        BitBoard bishop_queen = pos->Pieces(current_turn, pt_Queen, pt_Bishop);
        while (bishop_queen)
        {
            const Sq piece_index = Magics::FindLS1B(bishop_queen);
            if(our_king & GetMovesForSliding<Diagonal       >(piece_index, us, them)->attacks_) return true;
            if(our_king & GetMovesForSliding<AntiDiagonal   >(piece_index, us, them)->attacks_) return true;
            bishop_queen = Magics::PopLS1B(bishop_queen);
        }
        
        //rook and other half of queen
        BitBoard rook_queen = pos->Pieces(current_turn, pt_Queen, pt_Rook);
        while(rook_queen)
        {
            const Sq piece_index = Magics::FindLS1B(rook_queen);

            if(our_king & GetMovesForSliding<File>(piece_index, us, them)->attacks_) return true;
            if(our_king & GetMovesForSliding<Rank>(piece_index, us, them)->attacks_) return true;

            rook_queen = Magics::PopLS1B(rook_queen);
        }

        //knights
        BitBoard knights = pos->Pieces(current_turn, pt_Knight);
        while(knights)
        {
            if(our_king & Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)]) return true;
            knights = Magics::PopLS1B(knights);
        }

        //pawns
        const BitBoard them_pawns = pos->Pieces(Colour(current_turn ^ Black), pt_Pawn);
        if constexpr (current_turn == White)
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
        return (our_king & Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos->Pieces(Colour(current_turn ^ Black), pt_King))]);
    }

    template<Colour current_turn>
    BitBoard GenerateAllAttacks(Position const* pos)
    {
        return (QueenAttacks<current_turn>(pos) | BishopAttacks<current_turn>(pos) |
                RookAttacks<current_turn>(pos)  | KnightAttacks<current_turn>(pos) |
                PawnAttacks<current_turn>(pos)  | KingAttacks<current_turn>(pos));
    }

    template<Colour current_turn>
    constexpr void Castling(Position const* pos, MoveList* ml) 
    {
        if(!((current_turn  == White ? 0x0C : 0x03) & pos->CastlingRights())) {return;} //checks for castling rights
        if(InCheck<current_turn>(pos)) {return;} //checks if king under attack

        const BitBoard enemy_attacks = (current_turn == White ? GenerateAllAttacks<Black>(pos) : GenerateAllAttacks<White>(pos));

        const BitBoard whole_board = pos->Pieces(White) | pos->Pieces(Black);
        const U8 king_index = (current_turn  == White ? 4 : 60);
        const U8 rank_looked_at = U8(current_turn  == White ? (whole_board & 0xFF) : whole_board >> 56);

        if // kingside
        (
            (pos->CastlingRights() & (current_turn  == White ? 0x08 : 0x02)) // has rights
            &&
            !(rank_looked_at & 0x60) // not blocked 01100000 10010001
            &&
            !(0xFF & (current_turn  == White ? enemy_attacks : enemy_attacks >> 56) & 0x60) // not under attack by enemy
        )
        {
            if constexpr(current_turn  == White)
                ml->add(Moves::EncodeMove(king_index, 6, mt_Castling));
            else
                ml->add(Moves::EncodeMove(king_index, 62, mt_Castling));

        }
        if //queenside
        (
            (pos->CastlingRights() & (current_turn  == White ? 0x04 : 0x01)) // has rights
            &&
            !(rank_looked_at & 0x0E) // not blocked
            &&
            !(0xFF & (current_turn  == White ? enemy_attacks : enemy_attacks >> 56) & 0x0C) // not under attack by enemy
        )
        {
            if constexpr(current_turn  == White)
                ml->add(Moves::EncodeMove(king_index, 2, mt_Castling));
            else
                ml->add(Moves::EncodeMove(king_index, 58, mt_Castling));
        }
    }

    template<Colour current_turn>
    constexpr void GeneratePseudoLegalMoves(Position const* __restrict__  pos, MoveList* __restrict__ ml)
    {
        KingMoves<current_turn>(pos, ml);
        QueenMoves<current_turn>(pos, ml);
        BishopMoves<current_turn>(pos, ml);
        KnightMoves<current_turn>(pos, ml);
        RookMoves<current_turn>(pos, ml);
        (current_turn == White ? WhitePawnMoves(pos, ml) : BlackPawnMoves(pos, ml));
        Castling<current_turn>(pos, ml);
    }
    
    template<Colour current_turn>
    void GenerateLegalMoves(Position* __restrict__  pos, MoveList* __restrict__  ml)
    {
        //pseudo legal
        MoveList pseudo_legal_ml;
        GeneratePseudoLegalMoves<current_turn>(pos, &pseudo_legal_ml);
        
        //filtering
        for(size_t i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos->MakeMove(pseudo_legal_ml[i]);

            if(!InCheck<current_turn>(pos))
            {
                ml->add(pseudo_legal_ml[i]);
            }
            pos->UnmakeMove(pseudo_legal_ml[i]);
        }
    }
};

#endif // #ifndef MOVEGEN_HPP
