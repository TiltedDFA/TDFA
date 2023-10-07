#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include <iostream>
#include <string_view>
#include <stack>
namespace BB
{
    //This namespace is used to contain the accessing indexes for the pieces in 
    //the pieces array
    struct PosInfo
    {
    public:
        constexpr PosInfo():
            castling_rights_(0xF), half_moves_(0), en_passant_target_sq_(0){}
        constexpr PosInfo& operator=(const PosInfo& p)
        {
            this->castling_rights_ = p.castling_rights_;
            this->half_moves_ = p.half_moves_;
            this->en_passant_target_sq_ = p.en_passant_target_sq_;
            return *this;
        }
    public: 
        // top 4 bits are ignored XXXX WkWqBkBq where Wx and Bx represents sides and colours
        // 1 = can castle 0 = can't castle
        uint8_t castling_rights_;
        uint8_t half_moves_;
        uint8_t en_passant_target_sq_;
    };
    struct Position
    {
    public:
        constexpr Position():
            pieces_(),
            info_({}),
            whites_turn_(true),
            full_moves_(0)
        {
            for(uint8_t i = 0; i < 2; ++i)
                for(uint8_t j = 0; j < 6; ++j) pieces_[i][j] = 0ull;
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

            this->info_.castling_rights_ = p.info_.castling_rights_;
            this->whites_turn_ = p.whites_turn_;
            this->info_.en_passant_target_sq_ = p.info_.en_passant_target_sq_;
            this->info_.half_moves_ = p.info_.half_moves_;
            this->full_moves_ = p.full_moves_;
        }
        
        constexpr Position& operator=(const Position& p)
        {
            for(int i = 0 ; i < 2;++i)
                for(int j = 0; j < 6;++j)    
                    this->pieces_[i][j] = p.pieces_[i][j];

            this->info_.castling_rights_ = p.info_.castling_rights_;
            this->whites_turn_ = p.whites_turn_;
            this->info_.en_passant_target_sq_ = p.info_.en_passant_target_sq_;
            this->info_.half_moves_ = p.info_.half_moves_;
            this->full_moves_ = p.full_moves_;
            return *this;
        }
        
        void ResetBoard()
        {
            for(uint8_t i = 0; i < 2; ++i)
                    for(uint8_t j = 0; j < 6;++j) pieces_[i][j] = 0ull;

            info_.castling_rights_ = 0x0F;
            whites_turn_ = true;
            info_.en_passant_target_sq_ = 0;
            info_.half_moves_ = 0;
            full_moves_ = 0;

            while(!previous_pos_info.empty())
                previous_pos_info.pop();    
        }    
        //returns true/false depending on success of fen importing
        [[maybe_unused]]bool ImportFen(std::string_view fen);

        void MakeMove(Move m, PromType promotion)
        {
            //switch turns
            whites_turn_ = !whites_turn_;

            previous_pos_info.push(info_);

            uint8_t start;
            uint8_t target;
            PieceType p_type;
            bool is_white;
            Moves::DecodeMove(m, start, target, p_type, is_white);
            if(!is_white)
            {
                std::cout << std::to_string(start) << '\t' <<  std::to_string(target) << '\t' << std::to_string(p_type) << std::endl;
            }
            
            full_moves_ += !is_white;

            if((uint8_t)promotion) //if not NOPROMO
            {
                pieces_[is_white][loc::PAWN] &= ~Magics::IndexToBB(start);
                pieces_[is_white][(uint8_t)promotion] |= Magics::IndexToBB(target);
                info_.half_moves_ = 0;
                return;
            }
            
            if((GetPieces<true>() | GetPieces<false>()) & Magics::IndexToBB(target))
                info_.half_moves_ = 0;
            else
                ++info_.half_moves_;

            //updates en passant state
            if(p_type == Moves::PAWN) 
            {
                if (std::abs(target - start) == 16)
                {
                  info_.en_passant_target_sq_ = (is_white ? target - 8 : target + 8);
                }             
                info_.half_moves_ = 0;
            }
            else
            {
                info_.en_passant_target_sq_ = 0;
            }

            if(p_type == Moves::KING)
            {
                info_.castling_rights_ &= is_white ? 0x03 : 0x0C;

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
                    info_.castling_rights_ &= 0x0B;
                    break;
                case 7: 
                    info_.castling_rights_ &= 0x07;
                    break;
                case 63:
                    info_.castling_rights_ &= 0x0D;
                    break;
                case 56:
                    info_.castling_rights_ &= 0x0E;
                    break;
                default:
                    break;
                }
            }
            pieces_[is_white][p_type] &= ~Magics::IndexToBB(start);
            pieces_[is_white][p_type] |= Magics::IndexToBB(target);
        }
        
        void UnmakeMove(Move m, PromType promotion)
        {
            whites_turn_ = !whites_turn_;

            info_ = previous_pos_info.top();
            previous_pos_info.pop();

            uint8_t start;
            uint8_t target;
            PieceType p_type;
            bool is_white;
            Moves::DecodeMove(m, start, target, p_type, is_white);

            if((uint8_t)promotion) //if not NOPROMO
            {
                pieces_[is_white][loc::PAWN] |= ~Magics::IndexToBB(start);
                pieces_[is_white][(uint8_t)promotion] &= Magics::IndexToBB(target);
                return;
            }

            pieces_[is_white][p_type] &= ~Magics::IndexToBB(target);
            pieces_[is_white][p_type] |= Magics::IndexToBB(start);
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
            return (info_.en_passant_target_sq_) ? Magics::IndexToBB(info_.en_passant_target_sq_) : 0ull;
        }

    public:
        BitBoard pieces_[2][6];
        PosInfo info_;
        bool whites_turn_;
        uint16_t full_moves_;
        static std::stack<PosInfo> previous_pos_info;
    };
}

#endif //#ifndef BITBOARD_HPP