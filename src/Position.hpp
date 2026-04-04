#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "BoardUtils.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "Types.hpp"
#include "ZobristConstants.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <string_view>
#include "Board.hpp"

struct StateInfo
{
public:
    constexpr StateInfo():
        castling_rights_(0),
        half_moves_(0),
        en_passant_sq_(Magics::EP_NULL),
        moved_type_(pt_None),
        captured_type_(p_None),
        zobrist_key_(0){}
public:
    U8          castling_rights_;
    U8          half_moves_;
    U8          en_passant_sq_;
    U8          moved_type_;    // PieceType stored as U8 for packing
    U8          captured_type_; // Piece stored as U8 for packing (Piece is now U8 anyway)
    // 3 bytes padding to align zobrist_key_
    ZobristKey  zobrist_key_;
};
static constexpr int MAX_GAME_PLY = 1024;

class Position final : public Board
{
public:
    constexpr Position():
        Board(),
        info_({}),
        turn_(White),
        full_moves_(0),
        state_sp_(0),
        state_stack_{}
    {}

    Position(std::string_view fen) : Position()
    {
        ImportFen(fen);
        HashCurrentPostion();
    }
    void Reset()
    {
        ResetBoard();
        info_.castling_rights_  = 0;
        info_.half_moves_       = 0;
        info_.en_passant_sq_    = Magics::EP_NULL;
        info_.captured_type_    = p_None;
        info_.zobrist_key_      = 0;
        turn_                   = White;
        full_moves_             = 0;
        state_sp_               = 0;
    }

    void ImportFen(std::string_view fen);

    void MakeMove(Move m);
    
    void UnmakeMove(Move m);

    
    constexpr BitBoard EmptySqs()const {return ~(Pieces(White, Black));}
    
    constexpr BitBoard EnPasBB()const {return (info_.en_passant_sq_ != Magics::EP_NULL) ? Magics::SqToBB(info_.en_passant_sq_) : 0ull;}

    constexpr Sq EnPasSq()const {return info_.en_passant_sq_;}

    constexpr U8 CastlingRights()const { return info_.castling_rights_;}

    constexpr Colour ColourToMove()const {return turn_;}

    constexpr ZobristKey ZKey()const {return info_.zobrist_key_;}

    constexpr U8 HalfMoves()const {return info_.half_moves_;}

    constexpr U16 FullMoves()const {return full_moves_;}


    /*
        This function is used in makemove to quickly find which piece is being attacked(which is necessary
        as different types of pieces are stored sperately) and removes the attacked piece from its given board.
        An implamented assumption is that the king can never be removed as no legal move should be able to do this.
    */
    bool IsOk() const
    {
        return ((Pieces(White) & Pieces(Black)) == 0ULL);
    }

    bool IsRepetition() const
    {
        for (int i = state_sp_ - 2; i >= 0; i -= 2)
        {
            if (state_stack_[i].zobrist_key_ == info_.zobrist_key_)
                return true;
        }
        return false;
    }

    void MakeNullMove()
    {
        state_stack_[state_sp_++] = info_;
        if (info_.en_passant_sq_ != Magics::EP_NULL)
        {
            info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
            info_.en_passant_sq_ = Magics::EP_NULL;
        }
        info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;
        turn_ = !turn_;
    }

    void UnmakeNullMove()
    {
        turn_ = !turn_;
        info_ = state_stack_[--state_sp_];
    }

    ZobristKey HashCurrentPostion();

private:
    void UpdateCastlingRights();
private:
    StateInfo info_;
    Colour turn_;
    U16 full_moves_;
    int state_sp_;
    StateInfo state_stack_[MAX_GAME_PLY];
};


#endif //#ifndef BITBOARD_HPP