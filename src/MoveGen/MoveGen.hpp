#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"
struct move_info
{
    Move encoded_move[7];
    uint16_t count;
};

/**
 * Dir[0] = file atks
 * Dir[1] = rank atks
 * Dir[2] = cross atks
 * Dir[3] = anti-cross atks
 */
static consteval std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
{
    std::array<std::array<std::array<move_info,2187>,4>,64> result{};
    for(uint8_t sq = 0; sq < 64; ++sq)
    {
        for(uint16_t us = 0; us < 256;++us)
        {
            for(uint16_t them = 0; them < 256;++them)
            {
                if((us & them) != 0) continue;
                move_info move{};
                uint8_t combined = (us | them) & ~Magics::file_of(sq);
                for(int8_t current_file = Magics::file_of(sq) + 1; current_file < 8;++current_file)
                {
                    if(!((combined >> current_file)&1)) 
                    {
                        move.encoded_move[move.count] = Moves::EncodeMove(sq,sq+current_file,Moves::ROOK,1);
                        ++move.count;
                        continue;
                    }
                    if((us >> current_file)&1) break;
                    move.encoded_move[move.count] = Moves::EncodeMove(sq,sq+current_file,Moves::ROOK,1);
                    break;
                }
                for(int8_t current_file = Magics::file_of(sq) - 1; current_file > -1;--current_file)
                {
                    if(!((combined >> current_file)&1)) 
                    {
                        move.encoded_move[move.count] = Moves::EncodeMove(sq,sq-current_file,Moves::ROOK,1);
                        ++move.count;
                        continue;
                    }
                    if((us >> current_file)&1) break;
                    move.encoded_move[move.count] = Moves::EncodeMove(sq,sq-current_file,Moves::ROOK,1);
                    break;
                }
                const int index = Magics::base_2_to_3[Magics::CollapsedFilesIndex(us & Magics::SLIDING_ATTACKS_MASK[sq][0])] + 2 * Magics::base_2_to_3[Magics::CollapsedFilesIndex(them & Magics::SLIDING_ATTACKS_MASK[sq][0])];
                result.at(sq).at(0).at(index) = move;
            }
        }
        /*
        for(int dir = 0; dir < 4;++dir)
        {
            for(int atk_config = 0; atk_config < 2187;++atk_config)
            {
                move_info move{};
                result.at(sq).at(dir).at(atk_config) = move;
            }
        }
        */
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
    static constexpr std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
private:
    BitBoard white_pieces_;
    BitBoard black_pieces_;
    BitBoard empty_squares_;
};

#endif // #ifndef MOVEGEN_HPP