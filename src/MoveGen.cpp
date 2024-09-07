#include "MoveGen.hpp"
using Magics::Shift;

void MoveGen::WhitePawnMoves(Position const* pos, MoveList* ml) noexcept
{
    const BitBoard pawns = pos->Pieces<true, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move;
    const BitBoard capturable_squares = pos->PiecesByColour<false>() | (pos->EnPasBB() & ~Magics::RANK_3BB);

    pawn_move = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & ~Magics::RANK_8BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index - 8, index, Pawn));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & Magics::RANK_8BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);

        ml->add(Moves::EncodeMove(index - 8, index, PromQueen));
        ml->add(Moves::EncodeMove(index - 8, index, PromRook));
        ml->add(Moves::EncodeMove(index - 8, index, PromBishop));
        ml->add(Moves::EncodeMove(index - 8, index, PromKnight));

        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos->EmptySqs() & Shift<MD::NORTH>(pos->EmptySqs()) & Magics::RANK_4BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index - 16, index, Pawn));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index > 55)
        {
            ml->add(Moves::EncodeMove(index - 9, index, PromQueen));
            ml->add(Moves::EncodeMove(index - 9, index, PromRook));
            ml->add(Moves::EncodeMove(index - 9, index, PromBishop));
            ml->add(Moves::EncodeMove(index - 9, index, PromKnight));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 9, index, Pawn)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index > 55)
        {
            ml->add(Moves::EncodeMove(index - 7, index, PromQueen));
            ml->add(Moves::EncodeMove(index - 7, index, PromRook));
            ml->add(Moves::EncodeMove(index - 7, index, PromBishop));
            ml->add(Moves::EncodeMove(index - 7, index, PromKnight));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 7, index, Pawn)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }
}

void MoveGen::BlackPawnMoves(Position const* pos, MoveList* ml) noexcept
{
    const BitBoard pawns = pos->Pieces<false, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move;
    const BitBoard capturable_squares = pos->PiecesByColour<true>() | (pos->EnPasBB() & ~Magics::RANK_6BB);

    pawn_move = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & ~Magics::RANK_1BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 8, index, Pawn));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & Magics::RANK_1BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 8, index, PromQueen));
        ml->add(Moves::EncodeMove(index + 8, index, PromRook));
        ml->add(Moves::EncodeMove(index + 8, index, PromBishop));
        ml->add(Moves::EncodeMove(index + 8, index, PromKnight));
        pawn_move = Magics::PopLS1B(pawn_move);
    }
    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos->EmptySqs() & Shift<MD::SOUTH>(pos->EmptySqs()) & Magics::RANK_5BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 16, index, Pawn));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index < 8)
        {
            ml->add(Moves::EncodeMove(index + 7, index, PromQueen));
            ml->add(Moves::EncodeMove(index + 7, index, PromRook));
            ml->add(Moves::EncodeMove(index + 7, index, PromBishop));
            ml->add(Moves::EncodeMove(index + 7, index, PromKnight));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 7, index, Pawn)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index < 8)
        {
            ml->add(Moves::EncodeMove(index + 9, index, PromQueen));
            ml->add(Moves::EncodeMove(index + 9, index, PromRook));
            ml->add(Moves::EncodeMove(index + 9, index, PromBishop));
            ml->add(Moves::EncodeMove(index + 9, index, PromKnight));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 9, index, Pawn)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }
}