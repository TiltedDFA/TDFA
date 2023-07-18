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
    Move moves[7];
    uint16_t count;
};
static consteval std::array<std::array<BitBoard,4>,64> PrecomputeMask()
{
    std::array<std::array<BitBoard,4>,64> r_val;
    for(uint8_t i = 0; i < 64;++i)
    {
        uint8_t rank = Magics::rank_of(i);
        uint8_t file = Magics::file_of(i);
        BitBoard cross_attacks{0ull};
        BitBoard anti_cross_attacks{0ull};

        for(int8_t r = rank + 1, f = file + 1; r < 8 && f < 8;++r,++f)     cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 + f));
        for(int8_t r = rank - 1, f = file - 1; r >= 0 && f >= 0;--r,--f)   cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 + f));
        for(int8_t r = rank + 1, f = file - 1; r < 8 && f >= 0;++r, --f)   anti_cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 +f));
        for(int8_t r = rank - 1, f = file + 1; r >= 0 && f < 8;--r, ++f)   anti_cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 +f));
        
        r_val[i][0] = (Magics::FILE_ABB << file) & ~Magics::IndexToBB(i); //Rook file attacks
        r_val[i][1] = (Magics::RANK_1BB << (8*rank)) & ~Magics::IndexToBB(i); // Rook Rank attacks
        r_val[i][2] = cross_attacks & ~Magics::IndexToBB(i); //Bishop cross attacks
        r_val[i][3] = anti_cross_attacks & ~Magics::IndexToBB(i); //Bishop anti cross attacks
    }
    return r_val;
}
/**
 * Dir[0] = file atks
 * Dir[1] = rank atks
 * Dir[2] = cross atks
 * Dir[3] = anti-cross atks
 */
static consteval std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
{
    std::array<std::array<std::array<move_info,2187>,4>,64> result{};
    for(int sq = 0; sq < 64; ++sq)
    {
        for(int dir = 0; dir < 4;++dir)
        {
            for(int atk_config = 0; atk_config < 2187;++atk_config)
            {
                move_info move{};
                result.at(sq).at(dir).at(atk_config) = move;
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
    
    static constexpr std::array<std::array<BitBoard,4>,64> SLIDING_ATTACKS_MASK = PrecomputeMask(); 
    
    static constexpr std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
private:
    BitBoard white_pieces_;
    BitBoard black_pieces_;
    BitBoard empty_squares_;
};

#endif // #ifndef MOVEGEN_HPP