#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include <string_view>
namespace BB
{
    //This namespace is used to contain the accessing indexes for the pieces in 
    //the pieces array
    namespace loc
    {
        constexpr uint8_t WHITE = 0;
        constexpr uint8_t BLACK = 1;
        constexpr uint8_t KING  = 0;
        constexpr uint8_t QUEEN = 1;
        constexpr uint8_t BISHOP= 2;
        constexpr uint8_t KNIGHT= 3;
        constexpr uint8_t ROOK  = 4;
        constexpr uint8_t PAWN  = 5; 
    }
    struct Position
    {
    public:
        constexpr Position():
        pieces_(),
        castling_rights_(0xF),
        whites_turn_(true),
        en_passant_target_sq_(65),
        half_moves_(0),
        full_moves_(0)
        {
            for(uint8_t i = 0; i < 2; ++i)
                for(uint8_t j = 0; j < 6;++j) pieces_[i][j] = 0ull;
        }
        constexpr void ResetBoard()
        {
            for(uint8_t i = 0; i < 2; ++i)
                    for(uint8_t j = 0; j < 6;++j) pieces_[i][j] = 0ull;
            castling_rights_ = 0x0F;
            whites_turn_ = true;
            en_passant_target_sq_ = 65;
            half_moves_ = 0;
            full_moves_ = 0;    
        }    
        //returns true/false depending on success of fen importing
        [[maybe_unused]]bool ImportFen(std::string_view fen);

        template<bool is_white>
        constexpr BitBoard GetPieces()const
        {
            BitBoard combined_board{0ull};
            for(uint8_t i = 0; i < 6;++i) combined_board |= is_white ? pieces_[loc::WHITE][i] : pieces_[loc::BLACK][i];
            return combined_board;
        }
        template<uint8_t colour, uint8_t piecetype>
        constexpr BitBoard GetSpecificPieces()const
        {
            assert(colour < 2 && piecetype < 6);
            return pieces_[colour][piecetype];
        }
    public:
        BitBoard pieces_[2][6];
        // top 4 bits are ignored XXXX WkWqBkBq where Wx and Bx represents sides and colours
        // 1 = can castle 0 = can't castle
        uint8_t castling_rights_;
        bool whites_turn_;
        uint8_t en_passant_target_sq_;
        uint8_t half_moves_;
        uint16_t full_moves_;
    };
}

#endif //#ifndef BITBOARD_HPP