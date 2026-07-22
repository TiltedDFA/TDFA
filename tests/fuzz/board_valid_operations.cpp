#include "Board.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace
{
[[noreturn]] void fail() { std::abort(); }

constexpr std::array<Piece, 12> pieces{
    p_WhiteKing, p_WhiteQueen, p_WhiteBishop, p_WhiteKnight, p_WhiteRook, p_WhitePawn,
    p_BlackKing, p_BlackQueen, p_BlackBishop, p_BlackKnight, p_BlackRook, p_BlackPawn,
};
constexpr std::array<PieceType, 6> types{
    pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
};
constexpr std::array<Colour, 12> piece_colours{
    White, White, White, White, White, White,
    Black, Black, Black, Black, Black, Black,
};
constexpr std::array<PieceType, 12> piece_types{
    pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
    pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
};

void check(const Board& board, const std::array<Piece, 64>& model)
{
    std::array<BitBoard, 2> by_colour{};
    std::array<BitBoard, 6> by_type{};
    for (unsigned square = 0; square < 64; ++square)
    {
        const Piece expected = model[square];
        if (board.PieceOn(static_cast<Sq>(square)) != expected)
            fail();
        if (expected == p_None)
            continue;
        std::size_t semantic = 0;
        while (semantic < pieces.size() && pieces[semantic] != expected)
            ++semantic;
        if (semantic == pieces.size())
            fail();
        const unsigned colour = piece_colours[semantic] == White ? 0U : 1U;
        unsigned type = 0;
        while (type < types.size() && types[type] != piece_types[semantic])
            ++type;
        if (type == types.size())
            fail();
        const BitBoard bit = BitBoard{1} << square;
        by_colour[colour] |= bit;
        by_type[type] |= bit;
    }
    if (board.Pieces(White) != by_colour[0] || board.Pieces(Black) != by_colour[1])
        fail();
    if (board.Pieces(pt_All) != (by_colour[0] | by_colour[1]))
        fail();
    for (unsigned type = 0; type < types.size(); ++type)
        if (board.Pieces(types[type]) != by_type[type])
            fail();
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    Board board;
    std::array<Piece, 64> model{};
    model.fill(p_None);
    check(board, model);

    for (std::size_t index = 0; index + 2 < size; index += 3)
    {
        const unsigned operation = data[index] % 5U;
        const unsigned square = data[index + 1] % 64U;
        const Piece piece = pieces[data[index + 2] % pieces.size()];
        if (operation == 4U)
        {
            board.ResetBoard();
            model.fill(p_None);
        }
        else if (model[square] == p_None)
        {
            board.AddPiece(piece, static_cast<Sq>(square));
            model[square] = piece;
        }
        else if (operation == 0U)
        {
            board.RemovePiece(static_cast<Sq>(square));
            model[square] = p_None;
        }
        else if (operation == 1U)
        {
            const Piece removed = board.PopPiece(static_cast<Sq>(square));
            if (removed != model[square])
                fail();
            model[square] = p_None;
        }
        else
        {
            unsigned target = data[index + 2] % 64U;
            for (unsigned step = 0; step < 64 && model[target] != p_None; ++step)
                target = (target + 1U) % 64U;
            if (target != square && model[target] == p_None)
            {
                const Piece moving = model[square];
                board.MovePiece(static_cast<Sq>(square), static_cast<Sq>(target));
                model[square] = p_None;
                model[target] = moving;
            }
        }
        check(board, model);
    }
    return 0;
}
