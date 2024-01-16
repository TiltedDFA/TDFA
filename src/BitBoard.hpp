#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include <iostream>
#include <string_view>
#include <stack>
#include "ZobristConstants.hpp"

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
            full_moves_(0),
            postion_key_(0)
        {
            for(U8 i = 0; i < 2; ++i)
                for(U8 j = 0; j < 6; ++j) pieces_[i][j] = 0ull;
        }
        
        Position(std::string_view fen)
        {
            ImportFen(fen);
            HashCurrentPostion();
        }
        
        constexpr Position(const Position& p)
        {
            for(int i = 0 ; i < 2; ++i)
                for(int j = 0; j < 6; ++j)    
                    pieces_[i][j] = p.pieces_[i][j];

            whites_turn_ = p.whites_turn_;
            full_moves_ = p.full_moves_;
            postion_key_ = p.postion_key_;
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
            postion_key_ = p.postion_key_;
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
            postion_key_ = 0;
        }    
        //returns true/false depending on success of fen importing
        [[maybe_unused]]bool ImportFen(std::string_view fen);

        void MakeMove(Move m);
        
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
        
        constexpr BitBoard GetEmptySquares()const {return ~(GetPieces<true>() | GetPieces<false>());}
        
        constexpr BitBoard GetEnPassantBB()const {return (info_.en_passant_target_sq_) ? Magics::IndexToBB(info_.en_passant_target_sq_) : 0ull;}

        constexpr Sq GetEnPassantSq()const {return info_.en_passant_target_sq_;}

        constexpr U8 GetRawCastling()const { return info_.castling_rights_;}

        template<bool is_white>
        constexpr PieceType GetTypeAtSq(Sq sq)const
        {
            if      (pieces_[is_white][loc::QUEEN]  &   Magics::IndexToBB(sq))  return Moves::QUEEN;
            else if (pieces_[is_white][loc::BISHOP] &   Magics::IndexToBB(sq))  return Moves::BISHOP;
            else if (pieces_[is_white][loc::KNIGHT] &   Magics::IndexToBB(sq))  return Moves::KNIGHT;
            else if (pieces_[is_white][loc::ROOK]   &   Magics::IndexToBB(sq))  return Moves::ROOK;
            else if (pieces_[is_white][loc::PAWN]   &   Magics::IndexToBB(sq))  return Moves::PAWN;
            else                                                                return Moves::KING;
        }

        /*
            This function is used in makemove to quickly find which piece is being attacked(which is necessary
            as different types of pieces are stored sperately) and removes the attacked piece from its given board.
            An implamented assumption is that the king can never be removed as no legal move should be able to do this.
        */
        template<bool is_white>
        [[maybe_unused]]constexpr PieceType RemoveIntersectingPiece(BitBoard attacked_sq)
        {
            if (pieces_[is_white][loc::QUEEN] & attacked_sq)
            {
                pieces_[is_white][loc::QUEEN] ^= attacked_sq;
                return loc::QUEEN;
            }
            if (pieces_[is_white][loc::BISHOP] & attacked_sq)
            {
                pieces_[is_white][loc::BISHOP] ^= attacked_sq;
                return loc::QUEEN;
            }
            if (pieces_[is_white][loc::KNIGHT] & attacked_sq)
            {
                pieces_[is_white][loc::KNIGHT] ^= attacked_sq;
                return loc::QUEEN;
            }
            if (pieces_[is_white][loc::ROOK] & attacked_sq)
            {
                pieces_[is_white][loc::ROOK] ^= attacked_sq;
                return loc::QUEEN;
            }
            pieces_[is_white][loc::PAWN] ^= attacked_sq; 
            return loc::QUEEN;
        }

        void HashCurrentPostion();
    public:
        BitBoard pieces_[2][6];
        PosInfo info_;
        bool whites_turn_;
        int16_t full_moves_;
        ZobristKey postion_key_;
        static std::stack<Position> previous_pos_info;
    };
}

#endif //#ifndef BITBOARD_HPP