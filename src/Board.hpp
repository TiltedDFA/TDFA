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
     inline constexpr void ResetBoard()
     {
        std::fill(std::begin(board_),     std::end(board_),     p_None);
        std::fill(std::begin(by_colour_), std::end(by_colour_), 0ULL);
        std::fill(std::begin(by_type_),   std::end(by_type_),   0ULL);
     }
    constexpr Board()
    {
        ResetBoard();
    }


    [[nodiscard]] constexpr BitBoard Pieces(PieceType pt) const
    {
        return by_type_[pt];
    }
    [[nodiscard]] constexpr BitBoard Pieces(Colour c) const
    {
        return by_colour_[c];
    }
    template<typename... PieceTypes>
    [[nodiscard]] constexpr BitBoard Pieces(PieceType p, PieceTypes... pt) const
    {
        return Pieces(p) | Pieces(pt...);
    }
    template<typename... PieceTypes>
    [[nodiscard]] constexpr BitBoard Pieces(Colour c, PieceTypes... p) const
    {
        return Pieces(c) & Pieces(p...);
    }
    [[nodiscard]] constexpr Piece PieceOn(Sq s) const
    {
        assert(Magics::ValidSq(s));
        return board_[s];
    }
    [[nodiscard, gnu::always_inline]]
    static constexpr inline Piece MakePiece(Colour c, PieceType t)
    {
        return Piece(t + c * 8);
    }
    constexpr void AddPiece(Piece p, Sq s)
    {
        using namespace Magics;

        board_[s] = p;

        by_type_[TypeOf(p)]     |= SqToBB(s);
        by_type_[pt_All]        |= SqToBB(s);
        by_colour_[ColourOf(p)] |= SqToBB(s);
    }
    constexpr void RemovePiece(Sq s)
    {
        using namespace Magics;

        const Piece p = board_[s];

        by_type_[TypeOf(p)]     ^= SqToBB(s);
        by_type_[pt_All]        ^= SqToBB(s);
        by_colour_[ColourOf(p)] ^= SqToBB(s);

        board_[s] = p_None;
    }
    constexpr Piece PopPiece(Sq s)
    {
        using namespace Magics;

        const Piece p = board_[s];

        by_type_[TypeOf(p)]     ^= SqToBB(s);
        by_type_[pt_All]        ^= SqToBB(s);
        by_colour_[ColourOf(p)] ^= SqToBB(s);

        board_[s] = p_None;
        return p;
    }
    constexpr void MovePiece(Sq from, Sq to)
    {
        using namespace Magics;
        assert(board_[from] != p_None);

        const Piece p = board_[from];
        const BitBoard move = SqToBB(from) | SqToBB(to);

        by_type_[pt_All]           ^= move;
        by_type_[TypeOf(p)]        ^= move;
        by_colour_[ColourOf(p)]    ^= move;

        board_[from]   = p_None;
        board_[to]     = p;

    }
private:
    Piece    board_[64]{};
    BitBoard by_colour_[2]{};
    BitBoard by_type_  [7]{}; // pieces + all board
};

#endif //TDFA_BOARD_HPP
