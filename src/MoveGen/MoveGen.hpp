#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"
using Magics::file_of;
/**
 * Dir[0] = file atks
 * Dir[1] = rank atks
 * Dir[2] = cross atks
 * Dir[3] = anti-cross atks
 */
/**
 * The reason that we'd need an array of [64][4][2187] despite the blocker configurations being the same in any direction
 * would be due to the target indexs recorded being different based on the diretions.
 * As of writing this (26/7/23 03:09) the only things that I think would differe between the directions in terms of how
 * we precompute the moves would be how the moves are recorded(target indexes vairing) 
*/
static consteval std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
{
    std::array<std::array<std::array<move_info,2187>,4>,64> result{};
    for(uint8_t sq = 0; sq < 64; ++sq)
    {
        for(uint64_t us = 0; us < 256;++us)
        {
            for(uint64_t them = 0; them < 256;++them)
            {
                if(us & them) continue;
                //if((~us) & file_of(sq)) continue;


                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                uint8_t combined = (us | them) & ~file_of(sq);

                for(int8_t current_file = file_of(sq) + 1; current_file < 8; ++current_file)
                {
                    if(!((combined >> current_file) & 1)) //empty
                    {
                        //if(Magics::IndexInBounds(sq + 8 * (current_file - file_of(sq))))
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - file_of(sq)), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - file_of(sq)), Moves::ROOK, 1));

                       // if(Magics::IndexInBounds(sq + 9 * (current_file - file_of(sq)))) 
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 9 * (current_file - file_of(sq)), Moves::BISHOP, 1));
                        
                       // if(Magics::IndexInBounds(sq + 7 * (current_file - file_of(sq)))) 
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 7 * (current_file - file_of(sq)), Moves::BISHOP, 1));

                        continue;
                    }

                    if((us >> current_file) & 1) break; //our piece

                    if((them >> current_file) & 1) //their piece
                    {
                       // if(Magics::IndexInBounds(sq + 8 * (current_file - file_of(sq))))
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - file_of(sq)), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - file_of(sq)), Moves::ROOK, 1));

                        //if(Magics::IndexInBounds(sq + 9 * (current_file - file_of(sq))))
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 9 * (current_file - file_of(sq)), Moves::BISHOP, 1)); 
                        
                      //  if(Magics::IndexInBounds(sq + 7 * (current_file - file_of(sq)))) 
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 7 * (current_file - file_of(sq)), Moves::BISHOP, 1));

                        break;
                    }

                }
                for(int8_t current_file = file_of(sq) - 1; current_file > - 1 ; --current_file)
                {
                    if(!((combined >> current_file) & 1)) 
                    {
                      //  if(Magics::IndexInBounds(sq - 8 * (file_of(sq) - current_file)))
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (file_of(sq) - current_file), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (file_of(sq) - current_file), Moves::ROOK, 1));

                     //   if(Magics::IndexInBounds(sq - 9 * (current_file - file_of(sq)))) 
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 9 * (file_of(sq) - current_file), Moves::BISHOP, 1));

                        //if(Magics::IndexInBounds(sq - 7 * (current_file - file_of(sq)))) 
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 7 * (file_of(sq) - current_file), Moves::BISHOP, 1));

                        continue;
                    }

                    if((us >> current_file) & 1) break;

                    if((them >> current_file) & 1)
                    {
                       // if(Magics::IndexInBounds(sq - 8 * (file_of(sq) - current_file)))
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (file_of(sq) - current_file), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (file_of(sq) - current_file), Moves::ROOK, 1));

                      //  if(Magics::IndexInBounds(sq - 9 * (current_file - file_of(sq)))) 
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 9 * (file_of(sq) - current_file), Moves::BISHOP, 1));

                      //  if(Magics::IndexInBounds(sq - 7 * (current_file - file_of(sq)))) 
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 7 * (file_of(sq) - current_file), Moves::BISHOP, 1));

                        break;
                    }

                }

                uint16_t p1 = Magics::base_2_to_3[file_of(sq)][us & Magics::SLIDING_ATTACKS_MASK[file_of(sq)][(int)D::RANK]];
                uint16_t p2 = 2 * Magics::base_2_to_3[file_of(sq)][them & Magics::SLIDING_ATTACKS_MASK[file_of(sq)][(int)D::RANK]];
                uint16_t index = p1 + p2;

                result.at(sq).at(0).at(index) = file_attack_moves;
                result.at(sq).at(1).at(index) = rank_attack_moves;
                result.at(sq).at(2).at(index) = diagonal_attack_moves;
                result.at(sq).at(3).at(index) = anti_diagonal_attack_moves;
            }
        }
    }
    return result;
}

class MoveGen
{
public:
    MoveGen();
    [[nodiscard]] MoveList GenerateAllMoves(const BB::Position& pos);
private:
    void UpdateVariables(BitBoard white_pieces, BitBoard black_pieces);

    void WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    void BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    template<bool is_white>
    void BishopMoves(Move** move_list,BitBoard bishops)
    {
        if(!bishops) return;

        while(bishops)
        {
            const uint8_t bishop_index = Magics::FindLS1B(bishops);
            //file 
            uint16_t us = Magics::base_2_to_3[Magics::rank_of(bishop_index)][Magics::CollapsedRanksIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[bishop_index][static_cast<int>(D::DIAG)])];
            uint16_t them = 2 * Magics::base_2_to_3[Magics::rank_of(bishop_index)][Magics::CollapsedRanksIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[bishop_index][static_cast<int>(D::DIAG)])];
            move_info move = SLIDING_ATTACK_CONFIG[bishop_index][static_cast<int>(D::DIAG)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white,Moves::BISHOP>(move.encoded_move[i]);
            }
            //rank
            us = Magics::base_2_to_3[Magics::rank_of(bishop_index)][Magics::CollapsedRanksIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[bishop_index][static_cast<int>(D::ADIAG)])];
            them = 2 * Magics::base_2_to_3[Magics::rank_of(bishop_index)][Magics::CollapsedRanksIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[bishop_index][static_cast<int>(D::ADIAG)])];
            move = SLIDING_ATTACK_CONFIG[bishop_index][static_cast<int>(D::ADIAG)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white,Moves::BISHOP>(move.encoded_move[i]);
            }
            bishops = Magics::PopLSB(bishops);
        }
    }
    template<bool is_white>
    void RookMoves(Move** move_list, BitBoard rooks)
    {
        if(!rooks) return;

        while(rooks)
        {
            const uint8_t rook_index = Magics::FindLS1B(rooks);
            //file 
            uint16_t us = Magics::base_2_to_3[Magics::file_of(rook_index)][Magics::CollapsedRanksIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[rook_index][static_cast<int>(D::FILE)])];
            uint16_t them = 2 * Magics::base_2_to_3[Magics::file_of(rook_index)][Magics::CollapsedRanksIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[rook_index][static_cast<int>(D::FILE)])];
            move_info move = SLIDING_ATTACK_CONFIG[rook_index][static_cast<int>(D::FILE)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetColour<is_white>(move.encoded_move[i]);
            }
            //rank
            us = Magics::base_2_to_3[Magics::rank_of(rook_index)][Magics::CollapsedFilesIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[rook_index][static_cast<int>(D::RANK)])];
            them = 2 * Magics::base_2_to_3[Magics::rank_of(rook_index)][Magics::CollapsedFilesIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[rook_index][static_cast<int>(D::RANK)])];
            move = SLIDING_ATTACK_CONFIG[rook_index][static_cast<int>(D::RANK)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetColour<is_white>(move.encoded_move[i]);
            }
            rooks = Magics::PopLSB(rooks);
        }
    }
    template<bool is_white>
    void KnightMoves(Move** move_list, BitBoard knights) noexcept
    {
        if(!knights) return;
        while(knights)
        {
            const uint8_t knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & (empty_squares_ 
                                        | (is_white ?   (black_pieces_ & ~white_pieces_): 
                                                        (~black_pieces_ & white_pieces_)));
            while(possible_move)
            {
                const uint8_t attack_index = Magics::FindLS1B(possible_move);
                *(*move_list)++ = (is_white ?   Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,1):
                                                Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,0));
                possible_move = Magics::PopLSB(possible_move);
            }
            knights = Magics::PopLSB(knights);
        }
    }
    template<bool is_white>
    void QueenMoves(Move** move_list,BitBoard queens)
    {
        if(!queens) return;

        while(queens)
        {
            const uint8_t queen_index = Magics::FindLS1B(queens);
            //file 
            uint16_t us = Magics::base_2_to_3[Magics::rank_of(queen_index)][Magics::CollapsedFilesIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::FILE)])];
            uint16_t them = 2 * Magics::base_2_to_3[Magics::rank_of(queen_index)][Magics::CollapsedFilesIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::FILE)])];
            move_info move = SLIDING_ATTACK_CONFIG[queen_index][static_cast<int>(D::FILE)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white,Moves::QUEEN>(move.encoded_move[i]);
            }
            //rank
            us = Magics::base_2_to_3[Magics::file_of(queen_index)][Magics::CollapsedRanksIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::RANK)])];
            them = 2 * Magics::base_2_to_3[Magics::file_of(queen_index)][Magics::CollapsedRanksIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::RANK)])];
            move = SLIDING_ATTACK_CONFIG[queen_index][static_cast<int>(D::RANK)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white,Moves::QUEEN>(move.encoded_move[i]);
            }
            us = Magics::base_2_to_3[Magics::rank_of(queen_index)][Magics::CollapsedRanksIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::DIAG)])];
            them = 2 * Magics::base_2_to_3[Magics::rank_of(queen_index)][Magics::CollapsedRanksIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::DIAG)])];
            move = SLIDING_ATTACK_CONFIG[queen_index][static_cast<int>(D::DIAG)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white,Moves::QUEEN>(move.encoded_move[i]);
            }
            us = Magics::base_2_to_3[Magics::rank_of(queen_index)][Magics::CollapsedRanksIndex(((is_white) ? white_pieces_ : black_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::ADIAG)])];
            them = 2 * Magics::base_2_to_3[Magics::rank_of(queen_index)][Magics::CollapsedRanksIndex(((is_white) ? black_pieces_ : white_pieces_) & Magics::SLIDING_ATTACKS_MASK[queen_index][static_cast<int>(D::ADIAG)])];
            move = SLIDING_ATTACK_CONFIG[queen_index][static_cast<int>(D::ADIAG)][us+them];
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white,Moves::QUEEN>(move.encoded_move[i]);
            }
            queens = Magics::PopLSB(queens);
        }
    }
    void WhiteKingMoves(Move** move_list, BitBoard king) noexcept;

    void BlackKingMoves(Move** move_list, BitBoard king) noexcept;
    
public:    
    constexpr static std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
    inline static BitBoard EnPassantTargetSquare;
private:
    BitBoard white_pieces_;
    BitBoard black_pieces_;
    BitBoard empty_squares_;
};

#endif // #ifndef MOVEGEN_HPP