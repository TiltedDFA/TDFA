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
    constexpr void AddPiece(const Piece p, const Sq s)
    {
        using namespace Magics;
        const BitBoard sq_bb = SqToBB(s);
        by_type_[TypeOf(p)]     |= sq_bb;
        by_type_[pt_All]        |= sq_bb;
        by_colour_[ColourOf(p)] |= sq_bb;
        board_[s] = p;
    }
    constexpr void RemovePiece(const Sq s)
    {
        using namespace Magics;
        const Piece p = board_[s];
        const BitBoard sq_bb = SqToBB(s);
        by_type_[TypeOf(p)]     ^= sq_bb;
        by_type_[pt_All]        ^= sq_bb;
        by_colour_[ColourOf(p)] ^= sq_bb;
        board_[s] = p_None;
    }
    constexpr Piece PopPiece(const Sq s)
    {
        const Piece p = board_[s];
        RemovePiece(s);
        return p;
    }
    constexpr void MovePiece(const Sq from, const Sq to)
    {
        using namespace Magics;
        const Piece p = board_[from];
        const BitBoard move = SqToBB(from) | SqToBB(to);
        by_type_[pt_All]           ^= move;
        by_type_[TypeOf(p)]        ^= move;
        by_colour_[ColourOf(p)]    ^= move;
        board_[from]   = p_None;
        board_[to]     = p;
    }
    // Bitboard-based piece type lookup (avoids mailbox dependency)
    [[nodiscard]] constexpr PieceType PieceTypeOn(const Sq s) const
    {
        const BitBoard bb = Magics::SqToBB(s);
        if (by_type_[pt_Pawn]   & bb) return pt_Pawn;
        if (by_type_[pt_Knight] & bb) return pt_Knight;
        if (by_type_[pt_King]   & bb) return pt_King;
        if (by_type_[pt_Bishop] & bb) return pt_Bishop;
        if (by_type_[pt_Rook]   & bb) return pt_Rook;
        return pt_Queen;
    }

    // Known-type move: caller provides type+colour, skips re-reading mailbox
    constexpr void MovePieceFast(const Sq from, const Sq to, const PieceType pt, const Colour c)
    {
        const BitBoard move = Magics::SqToBB(from) | Magics::SqToBB(to);
        by_type_[pt_All] ^= move;
        by_type_[pt]     ^= move;
        by_colour_[c]    ^= move;
        board_[to]   = MakePiece(c, pt);
        board_[from] = p_None;
    }
    // Known-type remove: caller provides type+colour
    constexpr void RemovePieceFast(const Sq s, const PieceType pt, const Colour c)
    {
        const BitBoard sq_bb = Magics::SqToBB(s);
        by_type_[pt_All] ^= sq_bb;
        by_type_[pt]     ^= sq_bb;
        by_colour_[c]    ^= sq_bb;
        board_[s] = p_None;
    }
    // Known-type add: caller provides type+colour
    constexpr void AddPieceFast(const PieceType pt, const Colour c, const Sq s)
    {
        const BitBoard sq_bb = Magics::SqToBB(s);
        by_type_[pt_All] |= sq_bb;
        by_type_[pt]     |= sq_bb;
        by_colour_[c]    |= sq_bb;
        board_[s] = MakePiece(c, pt);
    }
    // Rebuild mailbox from bitboards (call before search/eval that needs PieceOn)
    constexpr void RebuildMailbox()
    {
        std::ranges::fill(board_, p_None);
        for (Colour c = White; c <= Black; c = Colour(c + 1))
            for (PieceType pt = pt_King; pt <= pt_Pawn; pt = PieceType(pt + 1))
            {
                BitBoard bb = Pieces(c, pt);
                while (bb)
                {
                    const Sq sq = Magics::FindLS1B(bb);
                    board_[sq] = MakePiece(c, pt);
                    bb = Magics::PopLS1B(bb);
                }
            }
    }
private:
    Piece    board_[64];
    BitBoard by_colour_[2];
    BitBoard by_type_  [7]; // piece type + all board
};

#endif //TDFA_BOARD_HPP
