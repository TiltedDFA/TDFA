#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "BitBoard.hpp"
#include "Move.hpp"
#include "Debug.hpp"
#include "MoveList.hpp"
#include <iostream>

#if CONSTEVAL_TIT == 1
inline constexpr std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
#else
inline std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
#endif
{
    std::array<std::array<std::array<move_info, 2187>, 4>, 64> result{};
    for(U8 sq = 0; sq < 64; ++sq)
    {
        for(U16 us = 0; us < 256; ++us)
        {
            for(U16 them = 0; them < 256; ++them)
            {
                //skiping useless blocker configs 
                if(us & them || (((~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) & ((~us) & Magics::BBRankOf(sq) || them & Magics::BBRankOf(sq)))) continue;

                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                const U8 rank_combined = (us | them) & ~Magics::BBFileOf(sq);
                U8 other_combined = (us | them) & ~Magics::BBRankOf(sq); 

                const U8 rankofsq = Magics::RankOf(sq);
                const U8 fileofsq = Magics::FileOf(sq);

                U64 diag_attacks = 0ull;
                U64 anti_diag_attacks = 0ull;
                if(us & Magics::BBFileOf(sq))
                {
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((rank_combined >> current_file) & 1)) //empty
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Moves::ROOK));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Moves::ROOK));
                            break;
                        }
                    }
                    for(int8_t current_file = fileofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((rank_combined >> current_file) & 1)) 
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Moves::ROOK));
                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Moves::ROOK));
                            break;
                        }
                    }
                    U16 p1 = Magics::base_2_to_3_us[fileofsq][us & ~Magics::BBFileOf(sq)];
                    U16 p2 = 2 * Magics::base_2_to_3_us[fileofsq][them];
                    result.at(sq).at((U8)D::RANK).at(p1 + p2) = rank_attack_moves;      
                }
               
                //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                if(us & Magics::BBRankOf(sq))
                {
                    for(int8_t current_file = rankofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((other_combined >> current_file) & 1)) //empty
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Moves::ROOK));
                            
                            if(Magics::IndexInBounds(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - rankofsq));
                            
                            if(Magics::IndexInBounds(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - rankofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Moves::ROOK));
                            
                            if(Magics::IndexInBounds(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - rankofsq));
                                
                            if(Magics::IndexInBounds(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - rankofsq));
                            break;
                        }
                        
                    }
                    for(int8_t current_file = rankofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((other_combined >> current_file) & 1)) 
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Moves::ROOK));
                            
                            if(Magics::IndexInBounds(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::IndexToBB(sq - 9 * (rankofsq - current_file));
                            
                            if(Magics::IndexInBounds(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (rankofsq - current_file));

                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Moves::ROOK));
                            if(Magics::IndexInBounds(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::IndexToBB(sq - 9 * (rankofsq - current_file));

                            if(Magics::IndexInBounds(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (rankofsq - current_file));
                            break;
                        }
                    }

                    diag_attacks &= Magics::SLIDING_ATTACKS_MASK[sq][(int)D::DIAG];
                    anti_diag_attacks &= Magics::SLIDING_ATTACKS_MASK[sq][(int)D::ADIAG];
                    while(diag_attacks)
                    {
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, Magics::FindLS1B(diag_attacks),Moves::BISHOP));
                        diag_attacks = Magics::PopLS1B(diag_attacks);
                    }
                    while(anti_diag_attacks)
                    {
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, Magics::FindLS1B(anti_diag_attacks),Moves::BISHOP));
                        anti_diag_attacks = Magics::PopLS1B(anti_diag_attacks);
                    }
                    U16 p1 = Magics::base_2_to_3_us[rankofsq][us & ~Magics::BBRankOf(sq)];
                    U16 p2 = 2 * Magics::base_2_to_3_us[rankofsq][them];

                    result.at(sq).at((U8)D::FILE).at(p1 + p2) = file_attack_moves;
                    result.at(sq).at((U8)D::DIAG).at(p1 + p2) = diagonal_attack_moves;
                    result.at(sq).at((U8)D::ADIAG).at(p1 + p2) = anti_diagonal_attack_moves;
                }
            }
        }
    }
    return result;
}

#if CONSTEVAL_TIT == 1
    constexpr inline std::array<std::array<std::array<move_info,2187>, 4>, 64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
#else
    const inline std::array<std::array<std::array<move_info, 2187>, 4>, 64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
#endif
namespace MoveGen
{
    BitBoard GenerateAllWhiteAttacks(const BB::Position& pos);
   

    BitBoard GenerateAllBlackAttacks(const BB::Position& pos);
   

    template<D direction>
    static constexpr const move_info* GetMovesForSliding(U8 piece_sq, BitBoard us, BitBoard them)
    {
        const U16 index = 
            Magics::GetBaseThreeUsThem
            (
                (direction == D::RANK) ? 
                Magics::CollapsedFilesIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)]):
                Magics::CollapsedRanksIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)]),

                (direction == D::RANK) ? 
                Magics::CollapsedFilesIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)]):
                Magics::CollapsedRanksIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)]),

                (direction == D::RANK) ? Magics::FileOf(piece_sq)  : Magics::RankOf(piece_sq)
            );
        return &SLIDING_ATTACK_CONFIG.at(piece_sq).at(static_cast<int>(direction)).at(index);
    }

    void WhitePawnMoves(const BB::Position& pos, MoveList& ml) noexcept;

    void BlackPawnMoves(const BB::Position& pos, MoveList& ml) noexcept;

    template<bool is_white>
    constexpr void BishopMoves(const BB::Position& pos, MoveList& ml)
    {
        BitBoard bishops = pos.GetSpecificPieces<is_white, loc::BISHOP>();
        if(!bishops) return;

        while(bishops)
        {
            const U8 bishop_index = Magics::FindLS1B(bishops);

            const move_info* move = GetMovesForSliding<D::DIAG>(bishop_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move->count; ++i)
                ml.add(move->encoded_move[i]);
            
            const move_info* move1 = GetMovesForSliding<D::ADIAG>(bishop_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move1->count; ++i)
                ml.add(move1->encoded_move[i]);
            
            bishops = Magics::PopLS1B(bishops);
        }
    }
    
    template<bool is_white>
    constexpr void RookMoves(const BB::Position& pos, MoveList& ml)
    {
        BitBoard rooks = pos.GetSpecificPieces<is_white, loc::ROOK>();
        if(!rooks) return;

        while(rooks)
        {
            const U8 rook_index = Magics::FindLS1B(rooks);

            const move_info* move = GetMovesForSliding<D::FILE>(rook_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move->count; ++i) 
                ml.add(move->encoded_move[i]);
            
            const move_info* move1 = GetMovesForSliding<D::RANK>(rook_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move1->count; ++i) 
                ml.add(move1->encoded_move[i]);
            
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<bool is_white>
    constexpr void KnightMoves(const BB::Position& pos, MoveList& ml)
    {
        BitBoard knights = pos.GetSpecificPieces<is_white, loc::KNIGHT>();
        if(!knights) return;
        while(knights)
        {
            const U8 knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & ~pos.GetPieces<is_white>();
            while(possible_move)
            {
                const U8 attack_index = Magics::FindLS1B(possible_move);
                ml.add(Moves::EncodeMove(knight_index, attack_index, Moves::KNIGHT));
                possible_move = Magics::PopLS1B(possible_move);
            }
            knights = Magics::PopLS1B(knights);
        }
    }

    template<bool is_white>
    constexpr void QueenMoves(const BB::Position& pos, MoveList& ml)
    {
        BitBoard queens = pos.GetSpecificPieces<is_white, loc::QUEEN>();
        if(!queens) return;

        while(queens)
        {
            const U8 queen_index = Magics::FindLS1B(queens);
            
            const move_info* move = GetMovesForSliding<D::FILE>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move->count; ++i)
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move->encoded_move[i]));
            

            const move_info* move1 = GetMovesForSliding<D::RANK>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move1->count; ++i)
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move1->encoded_move[i]));
            

            const move_info* move2 = GetMovesForSliding<D::DIAG>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move2->count; ++i)
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move2->encoded_move[i]));
            

            const move_info* move3 = GetMovesForSliding<D::ADIAG>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move3->count; ++i)
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move3->encoded_move[i]));
            
            queens = Magics::PopLS1B(queens);
        }
    }

    template<bool is_white>
    void KingMoves(const BB::Position& pos, MoveList& ml) 
    {
        const U8 king_index = Magics::FindLS1B(pos.GetSpecificPieces<is_white, loc::KING>());
        BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & ~pos.GetPieces<is_white>();
        while(king_attacks)
        {
            ml.add(Moves::EncodeMove(king_index, Magics::FindLS1B(king_attacks), Moves::KING));
            king_attacks = Magics::PopLS1B(king_attacks);
        }
    }

    template<bool is_white>
    constexpr BitBoard PawnAttacks(const BB::Position& pos)
    {
        const BitBoard pawns = pos.GetSpecificPieces<is_white, loc::PAWN>();
        if(!pawns) return 0;
        if constexpr(is_white)
        {
            return (Magics::Shift<MD::NORTH_EAST>(pawns) | Magics::Shift<MD::NORTH_WEST>(pawns)) & ~pos.GetPieces<true>();
        }
        return (Magics::Shift<MD::SOUTH_EAST>(pawns) | Magics::Shift<MD::SOUTH_WEST>(pawns)) & ~pos.GetPieces<false>();
    }
    
    template<bool is_white>
    constexpr BitBoard KingAttacks(const BB::Position& pos)
    {
        const BitBoard king_index = Magics::FindLS1B(pos.GetSpecificPieces<is_white, loc::KING>());
        return Magics::KING_ATTACK_MASKS[king_index] & ~pos.GetPieces<is_white>();
    }
    
    template<bool is_white>
    constexpr BitBoard KnightAttacks(const BB::Position& pos)
    {
        BitBoard knights = pos.GetSpecificPieces<is_white, loc::KNIGHT>();
        if(!knights) return 0;
        BitBoard attacks{0};
        while(knights)
        {
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~pos.GetPieces<is_white>();
            while(possible_move)
            {
                attacks |= Magics::FindLS1B(possible_move);
                possible_move = Magics::PopLS1B(possible_move);
            }
            knights = Magics::PopLS1B(knights);
        }
        return attacks;
    }
    
    template<bool is_white>
    constexpr BitBoard BishopAttacks(const BB::Position& pos)
    {
        BitBoard bishops = pos.GetSpecificPieces<is_white, loc::BISHOP>();
        if(!bishops) return 0;
        BitBoard attacks{0};
        while(bishops)
        {
            const U8 bishop_index = Magics::FindLS1B(bishops);

            const move_info* move = GetMovesForSliding<D::DIAG>(bishop_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));
            
            const move_info* move1 = GetMovesForSliding<D::ADIAG>(bishop_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move1->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move1->encoded_move[i]));
            
            bishops = Magics::PopLS1B(bishops);
        }
        return attacks;
    }

    template<bool is_white>
    constexpr BitBoard RookAttacks(const BB::Position& pos)
    {
        BitBoard rooks = pos.GetSpecificPieces<is_white, loc::ROOK>();
        if(!rooks) return 0;
        BitBoard attacks{0};

        while(rooks)
        {
            const U8 rook_index = Magics::FindLS1B(rooks);

            const move_info* move = GetMovesForSliding<D::FILE>(rook_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move->count; ++i) 
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));
            
            const move_info* move1 = GetMovesForSliding<D::RANK>(rook_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move1->count; ++i) 
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move1->encoded_move[i]));
            
            rooks = Magics::PopLS1B(rooks);
        }
        return attacks;
    }
    
    template<bool is_white>
    constexpr BitBoard QueenAttacks(const BB::Position& pos)
    {
        BitBoard queens = pos.GetSpecificPieces<is_white, loc::QUEEN>();
        if(!queens) return 0;
        BitBoard attacks{0};

        while(queens)
        {
            const U8 queen_index = Magics::FindLS1B(queens);
            
            const move_info* move = GetMovesForSliding<D::FILE>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));
            

            const move_info* move1 = GetMovesForSliding<D::RANK>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move1->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move1->encoded_move[i]));
            

            const move_info* move2 = GetMovesForSliding<D::DIAG>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move2->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move2->encoded_move[i]));
            

            const move_info* move3 = GetMovesForSliding<D::ADIAG>(queen_index, pos.GetPieces<is_white>(), pos.GetPieces<!is_white>());
            for(U8 i{0}; i < move3->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move3->encoded_move[i]));
            
            queens = Magics::PopLS1B(queens);
        }
        return attacks;
    }

    template<bool is_white>
    constexpr void Castling(const BB::Position& pos, MoveList& ml, BitBoard attacks) noexcept
    {   
        if(!((is_white ? 0x0C : 0x03) & pos.info_.castling_rights_)) {return;} //checks for castling rights
        if(pos.GetSpecificPieces<is_white, loc::KING>() & attacks) {return;} //checks if king under attack

        BitBoard king_attacks = 0;
        const BitBoard whole_board = pos.GetPieces<true>() | pos.GetPieces<false>();
        const U8 king_index = (is_white ? 4 : 60);
        const U8 rank_looked_at = (is_white ? (whole_board & 0xFF) : whole_board >> 56);

        if // kingside
        (
            (pos.info_.castling_rights_ & (is_white ? 0x08 : 0x02)) // has rights
            &&
            !(rank_looked_at & 0x60) // not blocked 01100000 10010001
            &&
            !(0xFF & (is_white ? attacks : attacks >> 56) & 0x60) // not under attack by enemy
        )
        {
            king_attacks |= (is_white ? Magics::IndexToBB<6>() : Magics::IndexToBB<62>());
        }
        if //queenside
        (
            (pos.info_.castling_rights_ & (is_white ? 0x04 : 0x01)) // has rights
            && 
            !(rank_looked_at & 0x0E) // not blocked
            && 
            !(0xFF & (is_white ? attacks : attacks >> 56) & 0x0C) // not under attack by enemy
        )
        {
            king_attacks |= (is_white ? Magics::IndexToBB<2>() : Magics::IndexToBB<58>());
        }

        while(king_attacks)
        {
            ml.add(Moves::EncodeMove(king_index, Magics::FindLS1B(king_attacks), Moves::KING));
            attacks  |= Magics::IndexToBB(Magics::FindLS1B(king_attacks));
            king_attacks = Magics::PopLS1B(king_attacks);
        }
    }
    
    template<bool is_white>
    bool InCheck(const BB::Position& pos)
    {
        const BitBoard our_king = pos.GetSpecificPieces<is_white, loc::KING>();
        // us, them are variables used for sliding move gen with titboards.
        //since we want to generate moves for the opponent and see if they attack
        //our king we want the us and them variables to be inverted from our king in colour
        const BitBoard us = pos.GetPieces<!is_white>();
        const BitBoard them = pos.GetPieces<is_white>();

        //Bishop and half queen
        BitBoard bishop_queen = pos.GetSpecificPieces<!is_white, loc::QUEEN>() | pos.GetSpecificPieces<!is_white, loc::BISHOP>();
        while (bishop_queen)
        {
            const Sq piece_index = Magics::FindLS1B(bishop_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::DIAG>(piece_index, us, them));
            for(U8 i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            move = const_cast<move_info*>(GetMovesForSliding<D::ADIAG>(piece_index, us, them));
            for(U8 i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            bishop_queen = Magics::PopLS1B(bishop_queen);
        }
        
        //rook and other half of queen
        BitBoard rook_queen = pos.GetSpecificPieces<!is_white, loc::QUEEN>() | pos.GetSpecificPieces<!is_white, loc::ROOK>();
        while(rook_queen)
        {
            const Sq piece_index = Magics::FindLS1B(rook_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::FILE>(piece_index, us, them));
            for(U8 i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            move = const_cast<move_info*>(GetMovesForSliding<D::RANK>(piece_index, us, them));
            for(U8 i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            rook_queen = Magics::PopLS1B(rook_queen);
        }

        //knights
        BitBoard knights = pos.GetSpecificPieces<!is_white, loc::KNIGHT>();
        while(knights)
        {
            if(our_king & Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)]) return true;
            knights = Magics::PopLS1B(knights);
        }

        //pawns
        const BitBoard them_pawns = pos.GetSpecificPieces<!is_white, loc::PAWN>();
        if (is_white)
        {
            if(our_king & Magics::Shift<MD::SOUTH_EAST>(them_pawns)) return true;
            if(our_king & Magics::Shift<MD::SOUTH_WEST>(them_pawns)) return true;
        }
        else
        {
            if(our_king & Magics::Shift<MD::NORTH_EAST>(them_pawns)) return true;
            if(our_king & Magics::Shift<MD::NORTH_WEST>(them_pawns)) return true;
        }

        // king attacks
        if(our_king & Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos.GetSpecificPieces<!is_white, loc::KING>())])
            return true;
                    
        return false;
    }

    template<bool is_white>
    constexpr void GeneratePseudoLegalMoves(const BB::Position& pos, MoveList& ml)
    {
        KingMoves<is_white>(pos, ml);
        QueenMoves<is_white>(pos, ml);
        BishopMoves<is_white>(pos, ml);
        KnightMoves<is_white>(pos, ml);
        RookMoves<is_white>(pos, ml);
        (is_white ? WhitePawnMoves(pos, ml) : BlackPawnMoves(pos, ml));
        Castling<is_white>(pos, ml, is_white ? GenerateAllBlackAttacks(pos) : GenerateAllWhiteAttacks(pos));
    }
    
    template<bool is_white>
    void GenerateLegalMoves(BB::Position& pos, MoveList& ml)
    {        
        //pseudo legal
        MoveList pseudo_legal_ml;
        GeneratePseudoLegalMoves<is_white>(pos, pseudo_legal_ml);
        
        //filtering
        for(U8 i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos.MakeMove(pseudo_legal_ml[i]);

            if(!InCheck<is_white>(pos))
            { 
                ml.add(pseudo_legal_ml[i]);
            }
            pos.UnmakeMove();
        }
    }
};

#endif // #ifndef MOVEGEN_HPP