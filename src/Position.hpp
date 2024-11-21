#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "Board.hpp"
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
#include <vector>

struct StateInfo
{
public:
    constexpr StateInfo():
        castling_rights_(0),
        half_moves_(0),
        en_passant_sq_(Magics::EP_NULL),
        captured_piece(p_None),
        zobrist_key_(0){}
public:
    U8          castling_rights_;
    U8          half_moves_;
    U8          en_passant_sq_;
    Piece       captured_piece;
    ZobristKey  zobrist_key_;
};
class Position final : public Board
{
public:
    constexpr Position():
            Board(),
            info_({}),
            colour_to_move(White),
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
    Position& operator=(Position const& other)=default;
    void Reset()
    {
        ResetBoard();
        info_.castling_rights_  = 0;
        info_.half_moves_       = 0;
        info_.en_passant_sq_    = Magics::EP_NULL;
        info_.captured_piece    = p_None;
        info_.zobrist_key_      = 0;
        colour_to_move          = White;
        full_moves_             = 0;
        previous_state_info.clear();
    }

    void ImportFen(std::string_view fen);


    void MakeMove(Move m);

    void UnmakeMove(Move m);
    
    [[nodiscard]] constexpr BitBoard EmptySqs()const {return ~(Pieces(White) | Pieces(Black));}
    
    [[nodiscard]] constexpr BitBoard EnPasBB()const {return (info_.en_passant_sq_ != Magics::EP_NULL) ? Magics::SqToBB(info_.en_passant_sq_) : 0ull;}

    [[nodiscard]] constexpr Sq EnPasSq()const {return info_.en_passant_sq_;}

    [[nodiscard]] constexpr U8 CastlingRights()const { return info_.castling_rights_;}

    [[nodiscard]] constexpr bool WhiteToMove()const {return colour_to_move;}

    [[nodiscard]] constexpr ZobristKey ZKey()const {return info_.zobrist_key_;}

    [[nodiscard]] constexpr U8 HalfMoves()const {return info_.half_moves_;}

    [[nodiscard]] constexpr U16 FullMoves()const {return full_moves_;}


    /*
        This function is used in makemove to quickly find which piece is being attacked(which is necessary
        as different types of pieces are stored sperately) and removes the attacked piece from its given board.
        An implamented assumption is that the king can never be removed as no legal move should be able to do this.
    */
    bool IsOk()
    {
        return ((Pieces(White) & Pieces(Black)) == 0);
    }
    [[maybe_unused]] ZobristKey HashCurrentPostion();
private:
    StateInfo info_;
    Colour colour_to_move;
    U16 full_moves_;
    std::vector<StateInfo> previous_state_info;
};


#endif //#ifndef BITBOARD_HPP