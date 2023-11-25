#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"
#include <iostream>


static std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
{
    std::array<std::array<std::array<move_info,2187>,4>,64> result{};
    for(uint8_t sq = 0; sq < 64; ++sq)
    {
        for(uint16_t us = 0; us < 256;++us)
        {
            for(uint16_t them = 0; them < 256;++them)
            {
                //skiping useless blocker configs 
                if(us & them || (((~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) & ((~us) & Magics::BBRankOf(sq) || them & Magics::BBRankOf(sq)))) continue;

                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                const uint8_t rank_combined = (us | them) & ~Magics::BBFileOf(sq);
                uint8_t other_combined = (us | them) & ~Magics::BBRankOf(sq); 

                const uint8_t rankofsq = Magics::RankOf(sq);
                const uint8_t fileofsq = Magics::FileOf(sq);

                uint64_t diag_attacks = 0ull;
                uint64_t anti_diag_attacks = 0ull;
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
                    uint16_t p1 = Magics::base_2_to_3[fileofsq][us & ~Magics::BBFileOf(sq)];
                    uint16_t p2 = 2 * Magics::base_2_to_3[fileofsq][them];
                    result.at(sq).at((uint8_t)D::RANK).at(p1 + p2) = rank_attack_moves;      
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
                    uint16_t p1 = Magics::base_2_to_3[rankofsq][us & ~Magics::BBRankOf(sq)];
                    uint16_t p2 = 2 * Magics::base_2_to_3[rankofsq][them];

                    result.at(sq).at((uint8_t)D::FILE).at(p1 + p2) = file_attack_moves;
                    result.at(sq).at((uint8_t)D::DIAG).at(p1 + p2) = diagonal_attack_moves;
                    result.at(sq).at((uint8_t)D::ADIAG).at(p1 + p2) = anti_diagonal_attack_moves;
                }
            }
        }
    }
    return result;
}

class MoveGen
{
public:
    MoveGen();

    [[nodiscard]] constexpr BitBoard GenerateAllWhiteAttacks(const BB::Position& pos) const
    {
        BitBoard attacks{0ull};
        
        const BitBoard us = pos.GetPieces<true>();
        const BitBoard them = pos.GetPieces<false>();

        //King
        attacks |=  Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos.GetSpecificPieces<true, loc::KING>())] 
                    & (~pos.GetPieces<true>());
        //Sliding
        BitBoard bishop_queen = pos.GetSpecificPieces<true, loc::QUEEN>() | pos.GetSpecificPieces<true, loc::BISHOP>();
        while (bishop_queen)
        {
            const Sq piece_index = Magics::FindLS1B(bishop_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::DIAG>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            move = const_cast<move_info*>(GetMovesForSliding<D::ADIAG>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            bishop_queen = Magics::PopLS1B(bishop_queen);
        }
        BitBoard rook_queen = pos.GetSpecificPieces<true, loc::QUEEN>() | pos.GetSpecificPieces<true, loc::ROOK>();
        while(rook_queen)
        {
            const Sq piece_index = Magics::FindLS1B(rook_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::FILE>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            move = const_cast<move_info*>(GetMovesForSliding<D::RANK>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            rook_queen = Magics::PopLS1B(rook_queen);
        }
        //knights
        BitBoard knights = pos.GetSpecificPieces<true, loc::KNIGHT>();
        while(knights)
        {
            attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~us;
            knights = Magics::PopLS1B(knights);
        }
        //pawns
        const BitBoard pawns = pos.GetSpecificPieces<true, loc::PAWN>();
        attacks |= Magics::Shift<MD::NORTH_EAST>(pawns) & ~us;
        attacks |= Magics::Shift<MD::NORTH_WEST>(pawns) & ~us;
        return attacks;
    }  

    [[nodiscard]] constexpr BitBoard GenerateAllBlackAttacks(const BB::Position& pos) const
    {
        BitBoard attacks{0ull};
        
        const BitBoard us = pos.GetPieces<false>();
        const BitBoard them = pos.GetPieces<true>();
        
        //King
        attacks |=  Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos.GetSpecificPieces<false, loc::KING>())] 
                    & (~pos.GetPieces<false>());
        //Sliding
        BitBoard bishop_queen = pos.GetSpecificPieces<false, loc::QUEEN>() | pos.GetSpecificPieces<false, loc::BISHOP>();
        while (bishop_queen)
        {
            const Sq piece_index = Magics::FindLS1B(bishop_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::DIAG>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            move = const_cast<move_info*>(GetMovesForSliding<D::ADIAG>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            bishop_queen = Magics::PopLS1B(bishop_queen);
        }
        BitBoard rook_queen = pos.GetSpecificPieces<false, loc::QUEEN>() | pos.GetSpecificPieces<false, loc::ROOK>();
        while(rook_queen)
        {
            const Sq piece_index = Magics::FindLS1B(rook_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::FILE>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            move = const_cast<move_info*>(GetMovesForSliding<D::RANK>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                    attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

            rook_queen = Magics::PopLS1B(rook_queen);
        }
        //knights
        BitBoard knights = pos.GetSpecificPieces<false, loc::KNIGHT>();
        while(knights)
        {
            attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~us;
            knights = Magics::PopLS1B(knights);
        }
        //pawns
        const BitBoard pawns = pos.GetSpecificPieces<false, loc::PAWN>();
        attacks |= Magics::Shift<MD::SOUTH_EAST>(pawns) & ~us;
        attacks |= Magics::Shift<MD::SOUTH_WEST>(pawns) & ~us;
        return attacks;
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
            for(uint8_t i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            move = const_cast<move_info*>(GetMovesForSliding<D::ADIAG>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            bishop_queen = Magics::PopLS1B(bishop_queen);
        }
        
        //rook and other half of queen
        BitBoard rook_queen = pos.GetSpecificPieces<!is_white, loc::QUEEN>() | pos.GetSpecificPieces<!is_white, loc::ROOK>();
        while(rook_queen)
        {
            const Sq piece_index = Magics::FindLS1B(rook_queen);

            move_info* move = const_cast<move_info*>(GetMovesForSliding<D::FILE>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
                if(our_king & Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]))) return true;

            move = const_cast<move_info*>(GetMovesForSliding<D::RANK>(piece_index, us, them));
            for(uint8_t i{0}; i < move->count; ++i)
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
    void GenerateLegalMoves(const BB::Position& pos, MoveList& ml)
    {
        pos_ = pos;
        
        //pseudo legal
        MoveList pseudo_legal_ml;
        KingMoves<is_white>(pseudo_legal_ml);
        QueenMoves<is_white>(pseudo_legal_ml);
        BishopMoves<is_white>(pseudo_legal_ml);
        KnightMoves<is_white>(pseudo_legal_ml);
        RookMoves<is_white>(pseudo_legal_ml);
        (is_white ? WhitePawnMoves(pseudo_legal_ml) : BlackPawnMoves(pseudo_legal_ml));
        Castling<is_white>(pseudo_legal_ml, is_white ? GenerateAllBlackAttacks(pos) : GenerateAllWhiteAttacks(pos));
        
        //filtering
        for(uint8_t i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos_.MakeMove(pseudo_legal_ml[i]);

            if(!(pos_.GetPieces<true>() & pos_.GetPieces<false>()) && !InCheck<is_white>(pos_))
            // if(!InCheck<is_white>(pos_))
            // if(!(pos_.GetSpecificPieces<is_white, loc::KING>() & (is_white ? GenerateAllBlackAttacks(pos_) : GenerateAllWhiteAttacks(pos_))))
            { 
                ml.add(pseudo_legal_ml[i]);
            }
            pos_.UnmakeMove();
        }
    }
    
    template<D direction>
    static constexpr const move_info* GetMovesForSliding(uint8_t piece_sq, BitBoard us, BitBoard them) noexcept
    {
        const uint16_t index = 
                //us
                (Magics::base_2_to_3
                [(direction == D::RANK) ? Magics::FileOf(piece_sq) 
                : Magics::RankOf(piece_sq)]
                [(direction == D::RANK) ? Magics::CollapsedFilesIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])
                : Magics::CollapsedRanksIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])])
                +
                //them
                (2 * Magics::base_2_to_3
                [(direction == D::RANK) ? Magics::FileOf(piece_sq) 
                : Magics::RankOf(piece_sq)]
                [(direction == D::RANK) ? Magics::CollapsedFilesIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])
                : Magics::CollapsedRanksIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])]);

        return &SLIDING_ATTACK_CONFIG.at(piece_sq).at(static_cast<int>(direction)).at(index);
    }
private:

    void WhitePawnMoves(MoveList& ml) noexcept;

    void BlackPawnMoves(MoveList& ml) noexcept;

    template<bool is_white>
    constexpr void BishopMoves(MoveList& ml)
    {
        BitBoard bishops = pos_.GetSpecificPieces<is_white, loc::BISHOP>();
        if(!bishops) return;

        while(bishops)
        {
            const uint8_t bishop_index = Magics::FindLS1B(bishops);

            const move_info* move = GetMovesForSliding<D::DIAG>(bishop_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move->count; ++i)
            {
                ml.add(move->encoded_move[i]);
            }
            
            const move_info* move1 = GetMovesForSliding<D::ADIAG>(bishop_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move1->count; ++i)
            {
                ml.add(move1->encoded_move[i]);
            }
            
            bishops = Magics::PopLS1B(bishops);
        }
    }
    template<bool is_white>
    constexpr void RookMoves(MoveList& ml)
    {
        BitBoard rooks = pos_.GetSpecificPieces<is_white, loc::ROOK>();
        if(!rooks) return;

        while(rooks)
        {
            const uint8_t rook_index = Magics::FindLS1B(rooks);

            const move_info* move = GetMovesForSliding<D::FILE>(rook_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move->count; ++i)
            {
                ml.add(move->encoded_move[i]);
            }

            const move_info* move1 = GetMovesForSliding<D::RANK>(rook_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move1->count; ++i)
            {
                ml.add(move1->encoded_move[i]);
            }
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<bool is_white>
    constexpr void KnightMoves(MoveList& ml) noexcept
    {
        BitBoard knights = pos_.GetSpecificPieces<is_white, loc::KNIGHT>();
        if(!knights) return;
        while(knights)
        {
            const uint8_t knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & ~pos_.GetPieces<is_white>();
            while(possible_move)
            {
                const uint8_t attack_index = Magics::FindLS1B(possible_move);
                ml.add((is_white ?   Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT)
                                    :Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT)));
                possible_move = Magics::PopLS1B(possible_move);
            }
            knights = Magics::PopLS1B(knights);
        }
    }

    template<bool is_white>
    constexpr void QueenMoves(MoveList& ml)
    {
        BitBoard queens = pos_.GetSpecificPieces<is_white, loc::QUEEN>();
        if(!queens) return;

        while(queens)
        {
            const uint8_t queen_index = Magics::FindLS1B(queens);
            
            const move_info* move = GetMovesForSliding<D::FILE>(queen_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move->count; ++i)
            {
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move->encoded_move[i]));
            }

            const move_info* move1 = GetMovesForSliding<D::RANK>(queen_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move1->count; ++i)
            {
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move1->encoded_move[i]));
            }

            const move_info* move2 = GetMovesForSliding<D::DIAG>(queen_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move2->count; ++i)
            {
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move2->encoded_move[i]));
            }

            const move_info* move3 = GetMovesForSliding<D::ADIAG>(queen_index, pos_.GetPieces<is_white>(), pos_.GetPieces<!is_white>());
            for(uint8_t i{0}; i < move3->count; ++i)
            {
                ml.add(Moves::SetPieceType<Moves::QUEEN>(move3->encoded_move[i]));
            }
            queens = Magics::PopLS1B(queens);
        }
    }

    template<bool is_white>
    void KingMoves(MoveList& ml) noexcept
    {
        const uint8_t king_index = Magics::FindLS1B(pos_.GetSpecificPieces<is_white, loc::KING>());
        BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & ~pos_.GetPieces<is_white>();
        while(king_attacks)
        {
            ml.add(Moves::EncodeMove(king_index, Magics::FindLS1B(king_attacks), Moves::KING));
            king_attacks = Magics::PopLS1B(king_attacks);
        }
    }

    template<bool is_white>
    constexpr void Castling(MoveList& ml, BitBoard attacks) noexcept
    {   
        if(!((is_white ? 0x0C : 0x03) & pos_.info_.castling_rights_)) {return;} //checks for castling rights
        if(pos_.GetSpecificPieces<is_white, loc::KING>() & attacks) {return;} //checks if king under attack

        BitBoard king_attacks = 0;
        const BitBoard whole_board = pos_.GetPieces<true>() | pos_.GetPieces<false>();
        const uint8_t king_index = (is_white ? 4 : 60);
        const uint8_t rank_looked_at = (is_white ? (whole_board & 0xFF) : whole_board >> 56);

        if // kingside
        (
            (pos_.info_.castling_rights_ & (is_white ? 0x08 : 0x02)) // has rights
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
            (pos_.info_.castling_rights_ & (is_white ? 0x04 : 0x01)) // has rights
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
    
public:    
    inline static std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
private:
    BB::Position pos_;
};

#endif // #ifndef MOVEGEN_HPP