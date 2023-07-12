#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include <string_view>
namespace BB
{
    struct Boards
    {
    public:
        consteval Boards():
        white_kings_(0ull), white_queens_(0ull), white_rooks_(0ull), white_bishops_(0ull), white_knights_(0ull), white_pawns_(0ull),
        black_kings_(0ull), black_queens_(0ull), black_rooks_(0ull), black_bishops_(0ull), black_knights_(0ull), black_pawns_(0ull){}
    public:
        BitBoard white_kings_;
        BitBoard white_queens_;
        BitBoard white_rooks_;
        BitBoard white_bishops_;
        BitBoard white_knights_;
        BitBoard white_pawns_;

        BitBoard black_kings_;
        BitBoard black_queens_;
        BitBoard black_rooks_;
        BitBoard black_bishops_;
        BitBoard black_knights_;
        BitBoard black_pawns_;
    };
    struct Position
    {
    public:
        consteval Position():
            boards_(),
            castling_rights_(0xF),
            whites_turn_(true),
            en_passant_target_sq_(65),
            half_moves_(0),
            full_moves_(0){}
        consteval void ResetBoard()
        {
            boards_ = Boards();
            castling_rights_ = 0x0F;
            whites_turn_ = true;
            en_passant_target_sq_ = 65;
            half_moves_ = 0;
            full_moves_ = 0;    
        }    
        //returns true/false depending on success of fen importing
        bool ImportFen(std::string_view fen);
    public:
        Boards boards_;
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