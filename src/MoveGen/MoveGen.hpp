#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"
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
                if((us & them)) continue;

                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                uint8_t combined = (us | them) & ~Magics::file_of(sq);

                for(int8_t current_file = Magics::file_of(sq) + 1; current_file < 8; ++current_file)
                {
                    if(!((combined >> current_file) & 1)) //empty
                    {
                        if(Magics::IndexInBounds(sq + 8 * (current_file - Magics::file_of(sq))))
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - Magics::file_of(sq)), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - Magics::file_of(sq)), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq + 9 * (current_file - Magics::file_of(sq)))) 
                            diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 9 * (current_file - Magics::file_of(sq)), Moves::BISHOP, 1));
                        
                        if(Magics::IndexInBounds(sq + 7 * (current_file - Magics::file_of(sq)))) 
                            anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 7 * (current_file - Magics::file_of(sq)), Moves::BISHOP, 1));

                        continue;
                    }

                    if((us >> current_file) & 1) break; //our piece

                    if((them >> current_file) & 1) //their piece
                    {
                        if(Magics::IndexInBounds(sq + 8 * (current_file - Magics::file_of(sq))))
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - Magics::file_of(sq)), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - Magics::file_of(sq)), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq + 9 * (current_file - Magics::file_of(sq))))
                            diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 9 * (current_file - Magics::file_of(sq)), Moves::BISHOP, 1)); 
                        
                        if(Magics::IndexInBounds(sq + 7 * (current_file - Magics::file_of(sq)))) 
                            anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq + 7 * (current_file - Magics::file_of(sq)), Moves::BISHOP, 1));

                        break;
                    }

                }
                for(int8_t current_file = Magics::file_of(sq) - 1; current_file > - 1 ; --current_file)
                {
                    if(!((combined >> current_file) & 1)) 
                    {
                        if(Magics::IndexInBounds(sq - 8 * (Magics::file_of(sq) - current_file)))
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (Magics::file_of(sq) - current_file), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (Magics::file_of(sq) - current_file), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq - 9 * (current_file - Magics::file_of(sq)))) 
                            diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 9 * (Magics::file_of(sq) - current_file), Moves::BISHOP, 1));

                        if(Magics::IndexInBounds(sq - 7 * (current_file - Magics::file_of(sq)))) 
                            anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 7 * (Magics::file_of(sq) - current_file), Moves::BISHOP, 1));

                        continue;
                    }

                    if((us >> current_file) & 1) break;

                    if((them >> current_file) & 1)
                    {
                        if(Magics::IndexInBounds(sq - 8 * (Magics::file_of(sq) - current_file)))
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (Magics::file_of(sq) - current_file), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (Magics::file_of(sq) - current_file), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq - 9 * (current_file - Magics::file_of(sq)))) 
                            diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 9 * (Magics::file_of(sq) - current_file), Moves::BISHOP, 1));

                        if(Magics::IndexInBounds(sq - 7 * (current_file - Magics::file_of(sq)))) 
                            anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, sq - 7 * (Magics::file_of(sq) - current_file), Moves::BISHOP, 1));

                        break;
                    }

                }

                uint16_t p1 = Magics::base_2_to_3[Magics::file_of(sq)][us & Magics::SLIDING_ATTACKS_MASK[Magics::file_of(sq)][1]];
                uint16_t p2 = 2 * Magics::base_2_to_3[Magics::file_of(sq)][them & Magics::SLIDING_ATTACKS_MASK[Magics::file_of(sq)][1]];
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
    void UpdateVariables(BitBoard white_pieces, BitBoard black_pieces);

    void WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    void BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    template<bool is_white>
    void BishopMoves(Move** move_list,BitBoard bishops)
    {

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

    void WhiteKingMoves(Move** move_list, BitBoard king) noexcept;

    void BlackKingMoves(Move** move_list, BitBoard king) noexcept;
    
public:    
    constexpr static std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
private:
    BitBoard white_pieces_;
    BitBoard black_pieces_;
    BitBoard empty_squares_;
};

#endif // #ifndef MOVEGEN_HPP