#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include <string_view>
namespace BB
{
    //This namespace is used to contain the accessing indexes for the pieces in 
    //the pieces array
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
        Position(std::string_view fen)
        {
            ImportFen(fen);
        }
        Position(Position& p)
        {
            for(int i = 0 ; i < 2;++i)
                for(int j = 0; j < 6;++i)    
                    this->pieces_[i][j] = p.pieces_[i][j];
            this->castling_rights_ = p.castling_rights_;
            this->whites_turn_ = p.whites_turn_;

            this->en_passant_target_sq_ = p.en_passant_target_sq_;
            this->half_moves_ = p.half_moves_;
            this->full_moves_ = p.full_moves_;
        }
        Position& operator=(const Position& p)
        {
            for(int i = 0 ; i < 2;++i)
                for(int j = 0; j < 6;++j)    
                    this->pieces_[i][j] = p.pieces_[i][j];
            this->castling_rights_ = p.castling_rights_;
            this->whites_turn_ = p.whites_turn_;

            this->en_passant_target_sq_ = p.en_passant_target_sq_;
            this->half_moves_ = p.half_moves_;
            this->full_moves_ = p.full_moves_;
            return *this;
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
            return 
                pieces_[is_white ? loc::WHITE : loc::BLACK][loc::KING]  |
                pieces_[is_white ? loc::WHITE : loc::BLACK][loc::QUEEN] |
                pieces_[is_white ? loc::WHITE : loc::BLACK][loc::BISHOP]|
                pieces_[is_white ? loc::WHITE : loc::BLACK][loc::KNIGHT]|
                pieces_[is_white ? loc::WHITE : loc::BLACK][loc::ROOK]  |
                pieces_[is_white ? loc::WHITE : loc::BLACK][loc::PAWN];
        }
        
        template<uint8_t colour, uint8_t piecetype>
        constexpr BitBoard GetSpecificPieces()const
        {
            assert(colour < 2 && piecetype < 6);
            return pieces_[colour][piecetype];
        }
        constexpr BitBoard GetEmptySquares()const
        {
            return ~(GetPieces<true>() | GetPieces<false>());
        }
        constexpr uint8_t GetEnPassantSq()const 
        {
            return en_passant_target_sq_;
        }
        constexpr BitBoard GetEnPassantBB()const
        {
            return (en_passant_target_sq_ == 65) ? 0ull : Magics::IndexToBB(en_passant_target_sq_);
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