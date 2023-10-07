#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
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
            en_passant_target_sq_(0),
            half_moves_(0),
            full_moves_(0),
            w_atks_(0),
            b_atks_(0)
        {
            for(uint8_t i = 0; i < 2; ++i)
                for(uint8_t j = 0; j < 6;++j) pieces_[i][j] = 0ull;
        }
        
        Position(std::string_view fen)
        {
            ImportFen(fen);
        }
        
        constexpr Position(Position& p)
        {
            for(int i = 0 ; i < 2;++i)
                for(int j = 0; j < 6;++i)    
                    this->pieces_[i][j] = p.pieces_[i][j];

            this->castling_rights_ = p.castling_rights_;
            this->whites_turn_ = p.whites_turn_;
            this->en_passant_target_sq_ = p.en_passant_target_sq_;
            this->half_moves_ = p.half_moves_;
            this->full_moves_ = p.full_moves_;
            this->w_atks_ = p.w_atks_;
            this->b_atks_ = p.b_atks_;
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
            this->w_atks_ = p.w_atks_;
            this->b_atks_ = p.b_atks_;
            return *this;
        }
        
        constexpr void ResetBoard()
        {
            for(uint8_t i = 0; i < 2; ++i)
                    for(uint8_t j = 0; j < 6;++j) pieces_[i][j] = 0ull;

            castling_rights_ = 0x0F;
            whites_turn_ = true;
            en_passant_target_sq_ = 0;
            half_moves_ = 0;
            full_moves_ = 0;
            w_atks_ = 0;
            b_atks_ = 0;    
        }    
        //returns true/false depending on success of fen importing
        [[maybe_unused]]bool ImportFen(std::string_view fen);

        constexpr void MakeMove(Move m, PromType promotion)
        {
            //switch turns
            whites_turn_ = !whites_turn_;

            uint8_t start;
            uint8_t target;
            PieceType p_type;
            bool is_white;
            Moves::DecodeMove(m, start, target, p_type, is_white);
            
            full_moves_ += !is_white;

            if((uint8_t)promotion) //if not NOPROMO
            {
                pieces_[is_white][loc::PAWN] &= ~Magics::IndexToBB(start);
                pieces_[is_white][(uint8_t)promotion] |= Magics::IndexToBB(target);
                half_moves_ = 0;
                return;
            }
            
            if((GetPieces<true>() | GetPieces<false>()) & Magics::IndexToBB(target))
                half_moves_ = 0;
            else ++half_moves_;

            //updates en passant state
            if(p_type == Moves::PAWN) 
            {
                if (std::abs(target - start) == 16)
                {
                   en_passant_target_sq_ = (is_white ? target - 8 : target + 8);
                }             
                half_moves_ = 0;
            }
            else
            {
                en_passant_target_sq_ = 0;
            }

            if(p_type == Moves::KING)
            {
                castling_rights_ &= is_white ? 0x03 : 0x0C;

                if(is_white && start - target == 2) //queen side
                {
                    pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<2>();
                    pieces_[loc::WHITE][loc::ROOK] &= ~Magics::IndexToBB<0>();
                    pieces_[loc::WHITE][loc::ROOK] |= Magics::IndexToBB<3>();
                    return;
                }
                else if(is_white && target - start == 2) //king side
                {
                    pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<6>();
                    pieces_[loc::WHITE][loc::ROOK] &= ~Magics::IndexToBB<7>();
                    pieces_[loc::WHITE][loc::ROOK] |= Magics::IndexToBB<5>();
                    return;
                }
                else if(start - target == 2) //queen side
                {
                    pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<58>();
                    pieces_[loc::BLACK][loc::ROOK] &= ~Magics::IndexToBB<56>();
                    pieces_[loc::BLACK][loc::ROOK] |= Magics::IndexToBB<59>();
                    return;
                }   
                else if(target - start == 2)
                {
                    pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<62>();
                    pieces_[loc::BLACK][loc::ROOK] &= ~Magics::IndexToBB<63>();
                    pieces_[loc::BLACK][loc::ROOK] |= Magics::IndexToBB<61>();
                    return;
                }
            } 
            else if(p_type == Moves::ROOK)
            {
                switch(start)
                {
                case 0:
                    castling_rights_ &= 0x0B;
                    break;
                case 7: 
                    castling_rights_ &= 0x07;
                    break;
                case 63:
                    castling_rights_ &= 0x0D;
                    break;
                case 56:
                    castling_rights_ &= 0x0E;
                    break;
                default:
                    break;
                }
            }
            pieces_[is_white][p_type] &= ~Magics::IndexToBB(start);
            pieces_[is_white][p_type] |= Magics::IndexToBB(target);
        }

        template<bool is_white>
        constexpr void UnmakeMove()
        {
        }
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
        
        constexpr BitBoard GetEnPassantBB()const
        {
            return (en_passant_target_sq_) ? Magics::IndexToBB(en_passant_target_sq_) : 0ull;
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
        BitBoard w_atks_;
        BitBoard b_atks_;
    };
}

#endif //#ifndef BITBOARD_HPP