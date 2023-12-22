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
    struct PosInfo
    {
    public:
        constexpr PosInfo():
            castling_rights_(0xF), half_moves_(0), en_passant_target_sq_(0){}
        constexpr PosInfo& operator=(const PosInfo& p)
        {
            castling_rights_ = p.castling_rights_;
            half_moves_ = p.half_moves_;
            en_passant_target_sq_ = p.en_passant_target_sq_;
            return *this;
        }
    public: 
        // top 4 bits are ignored XXXX WkWqBkBq where Wx and Bx represents sides and colours
        // 1 = can castle 0 = can't castle
        U8 castling_rights_;
        U8 half_moves_;
        U8 en_passant_target_sq_;
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
            for(U8 i = 0; i < 2; ++i)
                for(U8 j = 0; j < 6; ++j) pieces_[i][j] = 0ull;
        }
        
        Position(std::string_view fen)
        {
            ImportFen(fen);
        }
        
        constexpr Position(const Position& p)
        {
            for(int i = 0 ; i < 2; ++i)
                for(int j = 0; j < 6; ++j)    
                    pieces_[i][j] = p.pieces_[i][j];

            whites_turn_ = p.whites_turn_;
            full_moves_ = p.full_moves_;
            info_.castling_rights_ = p.info_.castling_rights_;
            info_.en_passant_target_sq_ = p.info_.en_passant_target_sq_;
            info_.half_moves_ = p.info_.half_moves_;
        }
        
        constexpr Position& operator=(const Position& p)
        {
            for(int i = 0; i < 2; ++i)
                for(int j = 0; j < 6; ++j)    
                    pieces_[i][j] = p.pieces_[i][j];

            whites_turn_ = p.whites_turn_;
            full_moves_ = p.full_moves_;
            info_.castling_rights_ = p.info_.castling_rights_;
            info_.en_passant_target_sq_ = p.info_.en_passant_target_sq_;
            info_.half_moves_ = p.info_.half_moves_;
            return *this;
        }
        
        void ResetBoard()
        {
            for(U8 i = 0; i < 2; ++i)
                    for(U8 j = 0; j < 6; ++j) pieces_[i][j] = 0ull;

            info_.castling_rights_ = 0x00;
            whites_turn_ = true;
            info_.en_passant_target_sq_ = 0;
            info_.half_moves_ = 0;
            full_moves_ = 0;

            while(!previous_pos_info.empty())
                previous_pos_info.pop();    
        }    
        //returns true/false depending on success of fen importing
        [[maybe_unused]]bool ImportFen(std::string_view fen);

        void MakeMove(Move m)
        {
            previous_pos_info.push(*this);
            //switch turns
            whites_turn_ = !whites_turn_;


            Sq start;
            Sq target;
            PieceType p_type;
            const bool is_white{!whites_turn_};
            Moves::DecodeMove(m, start, target, p_type);
            
            const BitBoard target_bb{Magics::IndexToBB(target)};
            
            full_moves_ += !is_white;

            if(Moves::IsPromotionMove(m)) //if not NOPROMO
            {
                pieces_[is_white][loc::PAWN] &= ~Magics::IndexToBB(start);

                if((is_white ? GetPieces<false>() : GetPieces<true>()) & target_bb)
                {
                    if(target_bb & (Magics::IndexToBB<0>()  | Magics::IndexToBB<7>() | 
                                Magics::IndexToBB<56>() | Magics::IndexToBB<63>()))
                    {
                        switch(target)
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
                    is_white ? RemoveIntersectingPiece<false>(target_bb) 
                            : RemoveIntersectingPiece<true>(target_bb);
                }
                pieces_[is_white][Moves::GetTypePromotingTo(m)] |= target_bb;
                info_.half_moves_ = 0;
                return;
            }
            
            //removes the piece we take
            if((is_white ? GetPieces<false>() : GetPieces<true>()) & target_bb)
            {
                //if removed piece is a rook
                if(target_bb & (Magics::IndexToBB<0>()  | Magics::IndexToBB<7>() | 
                                Magics::IndexToBB<56>() | Magics::IndexToBB<63>()))
                {
                    switch(target)
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
                info_.half_moves_ = 0;
                is_white ? RemoveIntersectingPiece<false>(target_bb) 
                         : RemoveIntersectingPiece<true>(target_bb);
            }
            else
                ++info_.half_moves_;

            //updates en passant state
            if(p_type == Moves::PAWN) 
            {
                //remove en pessant taken piece
                if(target == info_.en_passant_target_sq_)
                {
                    info_.en_passant_target_sq_ = 0; 
                    pieces_[is_white][p_type] ^= Magics::IndexToBB(start) | target_bb; // move our pawn
                    if(is_white)
                    {
                        RemoveIntersectingPiece<false>(Magics::IndexToBB(target-8));
                    }
                    else
                    {
                        RemoveIntersectingPiece<true>(Magics::IndexToBB(target+8));
                    }
                    return;
                }
                else if (std::abs(target - start) == 16)
                {
                    info_.en_passant_target_sq_ = (is_white ? target - 8 : target + 8);
                }       
                else
                {
                    info_.en_passant_target_sq_ = 0;
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

                switch(start | (target << 6))
                {
                    case 132: //queen
                        pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<2>();
                        pieces_[loc::WHITE][loc::ROOK] ^= Magics::IndexToBB<0>() | Magics::IndexToBB<3>();
                        return;
                    case 388: //king
                        pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<6>();
                        pieces_[loc::WHITE][loc::ROOK] ^= Magics::IndexToBB<7>() | Magics::IndexToBB<5>();
                        return;
                    case 3772: //queen
                        pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<58>();
                        pieces_[loc::BLACK][loc::ROOK] ^= Magics::IndexToBB<56>() | Magics::IndexToBB<59>();
                        return;
                    case 4028: //king
                        pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<62>();
                        pieces_[loc::BLACK][loc::ROOK] &= ~Magics::IndexToBB<63>();
                        pieces_[loc::BLACK][loc::ROOK] |= Magics::IndexToBB<61>();
                        return;
                    default:
                        break;
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
            pieces_[is_white][p_type] ^= Magics::IndexToBB(start) | target_bb;
        }
        
        void UnmakeMove()
        {
            *this = previous_pos_info.top();
            previous_pos_info.pop();
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
        
        template<bool is_white, PieceType type>
        constexpr BitBoard GetSpecificPieces()const
        {
            return pieces_[(is_white ? loc::WHITE : loc::BLACK)][type];
        }
        constexpr BitBoard GetSpecificPieces(bool is_white, PieceType type)const
        {
#if DEVELOPER_MODE == 1
            assert(type < 6);
#endif
            return pieces_[(is_white ? loc::WHITE : loc::BLACK)][type];
        }
        constexpr BitBoard GetEmptySquares()const
        {
            return ~(GetPieces<true>() | GetPieces<false>());
        }
        
        constexpr BitBoard GetEnPassantBB()const
        {
            return (info_.en_passant_target_sq_) ? Magics::IndexToBB(info_.en_passant_target_sq_) : 0ull;
        }
        constexpr Sq GetEnPassantSq()const
        {
            return info_.en_passant_target_sq_;
        }
        constexpr U8 GetRawCastling()const
        {
            return info_.castling_rights_;
        }

        /*
            This function is used in makemove to quickly find which piece is being attacked(which is necessary
            as different types of pieces are stored sperately) and removes the attacked piece from its given board.
            An implamented assumption is that the king can never be removed as no legal move should be able to do this.
        */
        template<bool is_white>
        constexpr void RemoveIntersectingPiece(BitBoard attacked_sq)
        {
            if (pieces_[is_white][loc::QUEEN] & attacked_sq)        pieces_[is_white][loc::QUEEN] ^= attacked_sq;
            else if (pieces_[is_white][loc::BISHOP] & attacked_sq)  pieces_[is_white][loc::BISHOP] ^= attacked_sq;
            else if (pieces_[is_white][loc::KNIGHT] & attacked_sq)  pieces_[is_white][loc::KNIGHT] ^= attacked_sq;
            else if (pieces_[is_white][loc::ROOK] & attacked_sq)    pieces_[is_white][loc::ROOK] ^= attacked_sq;
            else                                                    pieces_[is_white][loc::PAWN] ^= attacked_sq;
        }
    public:
        BitBoard pieces_[2][6];
        PosInfo info_;
        bool whites_turn_;
        int16_t full_moves_;
        static std::stack<Position> previous_pos_info;
    };
}

#endif //#ifndef BITBOARD_HPP