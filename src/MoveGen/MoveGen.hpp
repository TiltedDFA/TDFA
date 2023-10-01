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
using Magics::FileOf;
/**
 * The reason that we'd need an array of [64][4][2187] despite the blocker configurations being the same in any direction
 * would be due to the target indexs recorded being different based on the diretions.
 * As of writing this (26/7/23 03:09) the only things that I think would differe between the directions in terms of how
 * we precompute the moves would be how the moves are recorded(target indexes vairing) 
*/
consteval std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
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
                const uint8_t fileofsq = FileOf(sq);

                uint64_t diag_attacks = 0ull;
                uint64_t anti_diag_attacks = 0ull;
                if(us & Magics::BBFileOf(sq))
                {
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((rank_combined >> current_file) & 1)) //empty
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Moves::ROOK, 1));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Moves::ROOK, 1));
                            break;
                        }
                    }
                    for(int8_t current_file = fileofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((rank_combined >> current_file) & 1)) 
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Moves::ROOK, 1));
                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Moves::ROOK, 1));
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
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Moves::ROOK, 1));
                            
                            if(Magics::IndexInBounds(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - rankofsq));
                            
                            if(Magics::IndexInBounds(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - rankofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Moves::ROOK, 1));
                            
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
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Moves::ROOK, 1));
                            
                            if(Magics::IndexInBounds(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::IndexToBB(sq - 9 * (rankofsq - current_file));
                            
                            if(Magics::IndexInBounds(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (rankofsq - current_file));

                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Moves::ROOK, 1));
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
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq,Magics::FindLS1B(diag_attacks),Moves::BISHOP,1));
                        diag_attacks = Magics::PopLS1B(diag_attacks);
                    }
                    while(anti_diag_attacks)
                    {
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq,Magics::FindLS1B(anti_diag_attacks),Moves::BISHOP,1));
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
    
    void GenerateAllMoves(const BB::Position&,Move**);
    
    BitBoard GenerateAllWhiteMoves(const BB::Position&,Move**);

    BitBoard GenerateAllBlackMoves(const BB::Position&,Move**);
    template<bool is_white> 
    constexpr bool InCheck()
    {
        return pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::KING>() & (is_white ? b_atks_ : w_atks_);
    }
    
    
    uint64_t Perft(int depth)
    {
        MoveList l;
        uint64_t nodes{};

        if(!depth) return 1ull;

        return nodes;
    }
    
    template<D direction>
    static constexpr move_info& GetMovesForSliding(uint8_t piece_sq, BitBoard us, BitBoard them) noexcept
    {
        return const_cast<move_info&>(SLIDING_ATTACK_CONFIG[piece_sq][static_cast<int>(direction)]
            [
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
                : Magics::CollapsedRanksIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])])
            ]);
    }
private:

    void WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    void BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    template<bool is_white>
    constexpr void BishopMoves(Move** move_list, BitBoard bishops)
    {
        if(!bishops) return;

        while(bishops)
        {
            const uint8_t bishop_index = Magics::FindLS1B(bishops);

            const move_info& move = GetMovesForSliding<D::DIAG>(bishop_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::BISHOP>(move.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            }
            
            const move_info& move1 = GetMovesForSliding<D::ADIAG>(bishop_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::BISHOP>(move1.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            }
            
            bishops = Magics::PopLS1B(bishops);
        }
    }

    template<bool is_white>
    constexpr void RookMoves(Move** move_list, BitBoard rooks)
    {
        if(!rooks) return;

        while(rooks)
        {
            const uint8_t rook_index = Magics::FindLS1B(rooks);

            const move_info& move = GetMovesForSliding<D::FILE>(rook_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetColour<is_white>(move.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            }

            const move_info& move1 = GetMovesForSliding<D::RANK>(rook_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count;++i)
            {
                *(*move_list)++ = Moves::SetColour<is_white>(move1.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            }
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<bool is_white>
    constexpr void KnightMoves(Move** move_list, BitBoard knights) noexcept
    {
        if(!knights) return;
        while(knights)
        {
            const uint8_t knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & (pos_.GetEmptySquares() 
                                        | (is_white ?   (pos_.GetPieces<false>() & ~pos_.GetPieces<true>()): 
                                                        (~pos_.GetPieces<false>() & pos_.GetPieces<true>())));
            while(possible_move)
            {
                const uint8_t attack_index = Magics::FindLS1B(possible_move);
                *(*move_list)++ = (is_white ?   Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,1):
                                                Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,0));
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(attack_index);
                possible_move = Magics::PopLS1B(possible_move);
            }
            knights = Magics::PopLS1B(knights);
        }
    }

    template<bool is_white>
    constexpr void QueenMoves(Move** move_list, BitBoard queens)
    {
        if(!queens) return;

        while(queens)
        {
            const uint8_t queen_index = Magics::FindLS1B(queens);
            
            const move_info& move = GetMovesForSliding<D::FILE>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            }

            const move_info& move1 = GetMovesForSliding<D::RANK>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move1.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            }

            const move_info& move2 = GetMovesForSliding<D::DIAG>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move2.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move2.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move2.encoded_move[i]));
            }

            const move_info& move3 = GetMovesForSliding<D::ADIAG>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move3.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move3.encoded_move[i]);
                (is_white ? w_atks_ : b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move3.encoded_move[i]));
            }
            queens = Magics::PopLS1B(queens);
        }
    }

    void WhiteKingMoves(Move** move_list, BitBoard king) noexcept;

    void BlackKingMoves(Move** move_list, BitBoard king) noexcept;
    
public:    
    constexpr static std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
    inline static BitBoard EnPassantTargetSquare;
private:
    BB::Position pos_;
    BitBoard w_atks_;
    BitBoard b_atks_;
};

#endif // #ifndef MOVEGEN_HPP