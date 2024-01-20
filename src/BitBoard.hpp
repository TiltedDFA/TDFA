#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "BoardUtils.hpp"
#include <iostream>
#include <string_view>
#include <stack>
#include <cstring>
#include <cassert>
#include "ZobristConstants.hpp"


struct StateInfo
{
public:
    constexpr StateInfo(): castling_rights_(0xF), half_moves_(0), en_passant_sq_(0){}
    
    constexpr StateInfo& operator=(const StateInfo& p)
    {
        castling_rights_ = p.castling_rights_;
        half_moves_ = p.half_moves_;
        en_passant_sq_ = p.en_passant_sq_;
        return *this;
    }
public: 
    // top 4 bits are ignored XXXX WkWqBkBq where Wx and Bx represents sides and colours
    // 1 = can castle 0 = can't castle
    U8 castling_rights_;
    U8 half_moves_;
    U8 en_passant_sq_;
};
class Position
{
public:
    constexpr Position():
        pieces_(),
        info_({}),
        whites_turn_(true),
        full_moves_(0),
        position_key_(0){}
    
    Position(std::string_view fen)
    {
        ImportFen(fen);
        HashCurrentPostion();
    }
    
    constexpr Position(const Position& p)
    {
        std::memcpy(pieces_, p.pieces_, sizeof(pieces_));
        whites_turn_ = p.whites_turn_;
        full_moves_ = p.full_moves_;
        position_key_ = p.position_key_;
        info_.castling_rights_ = p.info_.castling_rights_;
        info_.en_passant_sq_ = p.info_.en_passant_sq_;
        info_.half_moves_ = p.info_.half_moves_;
    }
    
    constexpr Position& operator=(const Position& p)
    {
        std::memcpy(pieces_, p.pieces_, sizeof(pieces_));
        whites_turn_ = p.whites_turn_;
        full_moves_ = p.full_moves_;
        position_key_ = p.position_key_;
        info_.castling_rights_ = p.info_.castling_rights_;
        info_.en_passant_sq_ = p.info_.en_passant_sq_;
        info_.half_moves_ = p.info_.half_moves_;
        return *this;
    }
    
    void Reset()
    {
        std::memset(pieces_, 0, sizeof(pieces_));
        info_.castling_rights_ = 0x00;
        whites_turn_ = true;
        info_.en_passant_sq_ = 0;
        info_.half_moves_ = 0;
        full_moves_ = 0;

        while(!previous_pos_info.empty())
            previous_pos_info.pop();
        position_key_ = 0;
    }    

    void ImportFen(std::string_view fen);

    void MakeMove(Move m);
    
    void UnmakeMove()
    {
        *this = previous_pos_info.top();
        previous_pos_info.pop();
    }
    
    template<bool is_white>
    constexpr BitBoard PiecesByColour()const
    {
        return 
            pieces_[is_white][loc::KING]  |
            pieces_[is_white][loc::QUEEN] |
            pieces_[is_white][loc::BISHOP]|
            pieces_[is_white][loc::KNIGHT]|
            pieces_[is_white][loc::ROOK]  |
            pieces_[is_white][loc::PAWN];
    }
    
    template<bool is_white, PieceType type>
    constexpr BitBoard Pieces()const
    {
        return pieces_[is_white][type];
    }
    
    constexpr BitBoard Pieces(bool is_white, PieceType type)const
    {
        assert(type < 6);
        return pieces_[(is_white ? loc::WHITE : loc::BLACK)][type];
    }
    
    constexpr BitBoard EmptySqs()const {return ~(PiecesByColour<true>() | PiecesByColour<false>());}
    
    constexpr BitBoard EnPasBB()const {return (info_.en_passant_sq_) ? Magics::SqToBB(info_.en_passant_sq_) : 0ull;}

    constexpr Sq EnPasSq()const {return info_.en_passant_sq_;}

    constexpr U8 CastlingRights()const { return info_.castling_rights_;}

    constexpr bool WhiteToMove()const {return whites_turn_;}

    constexpr ZobristKey ZKey()const {return position_key_;}

    const U8 HalfMoves()const {return info_.half_moves_;}

    const U16 FullMoves()const {return full_moves_;}

    template<bool is_white>
    constexpr PieceType TypeAtSq(Sq sq)const
    {
        if      (pieces_[is_white][loc::QUEEN]  &   Magics::SqToBB(sq))  return Moves::QUEEN;
        else if (pieces_[is_white][loc::BISHOP] &   Magics::SqToBB(sq))  return Moves::BISHOP;
        else if (pieces_[is_white][loc::KNIGHT] &   Magics::SqToBB(sq))  return Moves::KNIGHT;
        else if (pieces_[is_white][loc::ROOK]   &   Magics::SqToBB(sq))  return Moves::ROOK;
        else if (pieces_[is_white][loc::PAWN]   &   Magics::SqToBB(sq))  return Moves::PAWN;
        else                                                                return Moves::KING;
    }

    /*
        This function is used in makemove to quickly find which piece is being attacked(which is necessary
        as different types of pieces are stored sperately) and removes the attacked piece from its given board.
        An implamented assumption is that the king can never be removed as no legal move should be able to do this.
    */
    template<bool is_white>
    constexpr PieceType RemoveIntersectingPiece(BitBoard attacked_sq)
    {
        assert(Magics::PopCnt(attacked_sq) == 1);
        if (pieces_[is_white][loc::QUEEN] & attacked_sq)
        {
            pieces_[is_white][loc::QUEEN] ^= attacked_sq;
            return loc::QUEEN;
        }
        if (pieces_[is_white][loc::BISHOP] & attacked_sq)
        {
            pieces_[is_white][loc::BISHOP] ^= attacked_sq;
            return loc::BISHOP;
        }
        if (pieces_[is_white][loc::KNIGHT] & attacked_sq)
        {
            pieces_[is_white][loc::KNIGHT] ^= attacked_sq;
            return loc::KNIGHT;
        }
        if (pieces_[is_white][loc::ROOK] & attacked_sq)
        {
            pieces_[is_white][loc::ROOK] ^= attacked_sq;
            return loc::ROOK;
        }
        pieces_[is_white][loc::PAWN] ^= attacked_sq; 
        return loc::PAWN;
    }

    void HashCurrentPostion();
private:
    void UpdateCastlingRights();
private:
    BitBoard pieces_[2][6];                         
    StateInfo info_;                                
    bool whites_turn_;                              
    U16 full_moves_;                                
    ZobristKey position_key_;                        
    static std::stack<Position> previous_pos_info;
};


#endif //#ifndef BITBOARD_HPP