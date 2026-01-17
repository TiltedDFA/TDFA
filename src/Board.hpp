//
// Created by Malik T on 14/09/2024.
//

#ifndef TDFA_BOARD_HPP
#define TDFA_BOARD_HPP

#include "Types.hpp"
#include <algorithm>
#include <array>
#include "Types.hpp"
#include "MagicConstants.hpp"


class Board
{
public:
    constexpr void ResetBoard()
    {
        std::ranges::fill(board_,     p_None);
        std::ranges::fill(by_colour_, 0ULL);
        std::ranges::fill(by_type_,   0ULL);
    }
    constexpr Board()
    {
        ResetBoard();
    }
    [[nodiscard]] constexpr BitBoard Pieces(const PieceType pt) const
    {
        return by_type_[pt];
    }
    [[nodiscard]] constexpr BitBoard Pieces(const Colour c) const
    {
        return by_colour_[c];
    }
    [[nodiscard]] constexpr BitBoard Pieces(Colour, Colour) const
    {
        return by_type_[pt_All];
    }
    template<typename... PieceTypes>
    [[nodiscard]] constexpr BitBoard Pieces(const PieceType p, PieceTypes... pt) const
    {
        return Pieces(p) | Pieces(pt...);
    }
    template<typename... PieceTypes>
    [[nodiscard]] constexpr BitBoard Pieces(const Colour c, PieceTypes... p) const
    {
        return Pieces(c) & Pieces(p...);
    }
    [[nodiscard]] constexpr Piece PieceOn(const Sq s) const
    {
        assert(Magics::ValidSq(s));
        return board_[s];
    }
    [[nodiscard, gnu::always_inline]]
    static constexpr inline Piece MakePiece(const Colour c, const PieceType t)
    {
        return Piece(t + c * 8);
    }
#ifndef NDEBUG
    constexpr void pedantic_check(Sq s, bool add, Piece p = Piece::p_None) const
    {
         const BitBoard sq_bb = Magics::SqToBB(s);
         const PieceType pt = Magics::TypeOf(p);
         assert(Magics::ValidSq(s));
         if (add)
         {
            const Colour c = Magics::ColourOf(p);
            assert(PieceOn(s) == p_None);                                  //no piece exists where we try to place it
            assert(pt != PieceType::pt_All && pt != PieceType::pt_None);   //valid piece type to move
            assert(!(by_type_[pt] & sq_bb));                               //no piece exists where we try to place it
            assert(!(by_type_[pt_All] & sq_bb));                           //no piece exists where we try to place it
            assert(!(by_colour_[c] & sq_bb));                              //no piece exists where we try to place it
         }
         else //remove/delete
         {
            assert(PieceOn(s) != p_None);                                 //piece exists where we try to place it
            assert(by_type_[pt] & sq_bb);                               //piece exists where we try to place it
            assert(by_type_[pt_All] & sq_bb);                           //piece exists where we try to place it
            assert(by_colour_[Magics::ColourOf(PieceOn(s))] & sq_bb);                              //piece exists where we try to place it
         }

    }
#else
    constexpr void pedantic_check(Sq s, bool add, Piece p = Piece::p_None) const
    {
        return;
    }
#endif
    constexpr void AddPiece(const Piece p, const Sq s)
    {
        using namespace Magics;
        pedantic_check(s, true, p);

        by_type_[TypeOf(p)]     |= SqToBB(s);
        by_type_[pt_All]        |= SqToBB(s);
        by_colour_[ColourOf(p)] |= SqToBB(s);

        board_[s] = p;
    }
    constexpr void RemovePiece(const Sq s)
    {
        using namespace Magics;

        const Piece p = board_[s];

        pedantic_check(s, false);

        by_type_[TypeOf(p)]     ^= SqToBB(s);
        by_type_[pt_All]        ^= SqToBB(s);
        by_colour_[ColourOf(p)] ^= SqToBB(s);

        board_[s] = p_None;
    }
    constexpr Piece PopPiece(const Sq s)
    {
        using namespace Magics;

        const Piece p = board_[s];
        RemovePiece(s);

        return p;
    }
    constexpr void MovePiece(const Sq from, const Sq to)
    {
        using namespace Magics;

        const Piece p = board_[from];
        pedantic_check(from, false);
        pedantic_check(to, true, p);

        const BitBoard move = SqToBB(from) | SqToBB(to);

        by_type_[pt_All]           ^= move;
        by_type_[TypeOf(p)]        ^= move;
        by_colour_[ColourOf(p)]    ^= move;

        board_[from]   = p_None;
        board_[to]     = p;

    }
private:
    Piece    board_[64];
    BitBoard by_colour_[2];
    BitBoard by_type_  [7]; // piece typee + all board
};

#endif //TDFA_BOARD_HPP