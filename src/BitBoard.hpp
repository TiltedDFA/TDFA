#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "BoardUtils.hpp"
#include <iostream>
#include <string_view>
#include <vector>
#include <cstring>
#include <cassert>
#include "ZobristConstants.hpp"

struct StateInfo
{
public:
    constexpr StateInfo():
        castling_rights_(0),
        half_moves_(0),
        en_passant_sq_(255),
        captured_type_(NullPiece),
        zobrist_key_(0){}
public: 
    U8          castling_rights_;
    U8          half_moves_;
    U8          en_passant_sq_;
    PieceType   captured_type_;
    ZobristKey  zobrist_key_;
};
class Position
{
public:
    constexpr Position():
        pieces_(),
        info_({}),
        whites_turn_(true),
        full_moves_(0),
        previous_state_info({})
    {
        previous_state_info.reserve(MAX_MOVES);
    }
    
    Position(std::string_view fen) : Position()
    {
        ImportFen(fen);
        HashCurrentPostion();
    }

#if DEVELOPER_MODE == 1

    bool operator==(const Position& other)
    {
        for(int i = 0; i < 2;++i)
        {
            for(int j = 0; j < 6;++j)
            {
                if(this->pieces_[i][j] != other.pieces_[i][j]) return false;
            }
        }
        if(info_.castling_rights_  != other.info_.castling_rights_  ) return false;
        if(info_.half_moves_       != other.info_.half_moves_       ) return false;
        if(info_.en_passant_sq_    != other.info_.en_passant_sq_    ) return false;
        if(info_.captured_type_    != other.info_.captured_type_    ) return false;
        if(info_.zobrist_key_      != other.info_.zobrist_key_      ) return false;
        if(whites_turn_            != other.whites_turn_            ) return false;
        if(full_moves_             != other.full_moves_             ) return false;
        return true;
    }
#endif

    constexpr Position(const Position& p)
    {
        std::memcpy(pieces_, p.pieces_, sizeof(pieces_));
        info_.castling_rights_  = p.info_.castling_rights_;
        info_.half_moves_       = p.info_.half_moves_;
        info_.en_passant_sq_    = p.info_.en_passant_sq_;
        info_.captured_type_    = p.info_.captured_type_;
        info_.zobrist_key_      = p.info_.zobrist_key_;
        whites_turn_            = p.whites_turn_;
        full_moves_             = p.full_moves_;
    }
    
    constexpr Position& operator=(const Position& p)
    {
        std::memcpy(pieces_, p.pieces_, sizeof(pieces_));
        info_.castling_rights_  = p.info_.castling_rights_;
        info_.half_moves_       = p.info_.half_moves_;
        info_.en_passant_sq_    = p.info_.en_passant_sq_;
        info_.captured_type_    = p.info_.captured_type_;
        info_.zobrist_key_      = p.info_.zobrist_key_;
        whites_turn_            = p.whites_turn_;
        full_moves_             = p.full_moves_;
        return *this;
    }
    
    void Reset()
    {
        std::memset(pieces_, 0, sizeof(pieces_));
        info_.castling_rights_  = 0;
        info_.half_moves_       = 0;
        info_.en_passant_sq_    = 255;
        info_.captured_type_    = Moves::BAD_MOVE;
        info_.zobrist_key_      = 0;
        whites_turn_            = true;
        full_moves_             = 0;
        previous_state_info.clear();
    }    

    void ImportFen(std::string_view fen);

    void MakeMove(Move m);
    
    void UnmakeMove(Move m);
    
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
    
    template<bool is_white, U8 type>
    constexpr BitBoard Pieces()const
    {
        return pieces_[is_white][type];
    }
    
    constexpr BitBoard Pieces(bool is_white, U8 type)const
    {
        assert(type < 6);
        return pieces_[(is_white ? loc::WHITE : loc::BLACK)][type];
    }
    
    constexpr BitBoard EmptySqs()const {return ~(PiecesByColour<true>() | PiecesByColour<false>());}
    
    constexpr BitBoard EnPasBB()const {return (info_.en_passant_sq_ != 255) ? Magics::SqToBB(info_.en_passant_sq_) : 0ull;}

    constexpr Sq EnPasSq()const {return info_.en_passant_sq_;}

    constexpr U8 CastlingRights()const { return info_.castling_rights_;}

    constexpr bool WhiteToMove()const {return whites_turn_;}

    constexpr ZobristKey ZKey()const {return info_.zobrist_key_;}

    constexpr U8 HalfMoves()const {return info_.half_moves_;}

    constexpr U16 FullMoves()const {return full_moves_;}

    template<bool is_white>
    constexpr PieceType TypeAtSq(Sq sq)const
    {
        if      (pieces_[is_white][loc::QUEEN]  &   Magics::SqToBB(sq))  return Queen;
        else if (pieces_[is_white][loc::BISHOP] &   Magics::SqToBB(sq))  return Bishop;
        else if (pieces_[is_white][loc::KNIGHT] &   Magics::SqToBB(sq))  return Knight;
        else if (pieces_[is_white][loc::ROOK]   &   Magics::SqToBB(sq))  return Rook;
        else if (pieces_[is_white][loc::PAWN]   &   Magics::SqToBB(sq))  return Pawn;
        else                                                             return King;
    }

    /*
        This function is used in makemove to quickly find which piece is being attacked(which is necessary
        as different types of pieces are stored sperately) and removes the attacked piece from its given board.
        An implamented assumption is that the king can never be removed as no legal move should be able to do this.
    */
    template<bool is_white>
    constexpr PieceType RemovePiece(BitBoard attacked_sq)
    {
        assert(Magics::PopCnt(attacked_sq) == 1);
        if (pieces_[is_white][loc::QUEEN] & attacked_sq)
        {
            pieces_[is_white][loc::QUEEN] &= ~attacked_sq;
            return Queen;
        }
        if (pieces_[is_white][loc::BISHOP] & attacked_sq)
        {
            pieces_[is_white][loc::BISHOP] &= ~attacked_sq;
            return Bishop;
        }
        if (pieces_[is_white][loc::KNIGHT] & attacked_sq)
        {
            pieces_[is_white][loc::KNIGHT] &= ~attacked_sq;
            return Knight;
        }
        if (pieces_[is_white][loc::ROOK] & attacked_sq)
        {
            pieces_[is_white][loc::ROOK] &= ~attacked_sq;
            return Rook;
        }
        pieces_[is_white][loc::PAWN] &= ~attacked_sq; 
        return Pawn;
    }
    bool IsOk()
    {
        return ((PiecesByColour<true>() & PiecesByColour<false>()) == 0);
    }
    void HashCurrentPostion();
private:
    void UpdateCastlingRights();
private:
    BitBoard pieces_[2][6];                         
    StateInfo info_;                                
    bool whites_turn_;                              
    U16 full_moves_;                                                      
    std::vector<StateInfo> previous_state_info;
};


#endif //#ifndef BITBOARD_HPP