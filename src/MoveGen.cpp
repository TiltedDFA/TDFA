#include "MoveGen.hpp"
#include <algorithm>
using Magics::Shift;

void MoveGen::WhitePawnMoves(Position const* pos, MoveList* ml) noexcept
{
    BitBoard pawns = pos->Pieces<true, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos->PiecesByColour<false>() | (pos->EnPasBB() & ~Magics::RANK_3BB);

    pawn_move = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & ~Magics::RANK_8BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index - 8, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & Magics::RANK_8BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);

        ml->add(Moves::EncodeMove(index - 8, index, Moves::PROM_QUEEN));
        ml->add(Moves::EncodeMove(index - 8, index, Moves::PROM_ROOK));
        ml->add(Moves::EncodeMove(index - 8, index, Moves::PROM_BISHOP));
        ml->add(Moves::EncodeMove(index - 8, index, Moves::PROM_KNIGHT));

        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos->EmptySqs() & Shift<MD::NORTH>(pos->EmptySqs()) & Magics::RANK_4BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index - 16, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index > 55)
        {   
            ml->add(Moves::EncodeMove(index - 9, index, Moves::PROM_QUEEN));
            ml->add(Moves::EncodeMove(index - 9, index, Moves::PROM_ROOK));
            ml->add(Moves::EncodeMove(index - 9, index, Moves::PROM_BISHOP));
            ml->add(Moves::EncodeMove(index - 9, index, Moves::PROM_KNIGHT));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 9, index, Moves::PAWN)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index > 55)
        {   
            ml->add(Moves::EncodeMove(index - 7, index, Moves::PROM_QUEEN));
            ml->add(Moves::EncodeMove(index - 7, index, Moves::PROM_ROOK));
            ml->add(Moves::EncodeMove(index - 7, index, Moves::PROM_BISHOP));
            ml->add(Moves::EncodeMove(index - 7, index, Moves::PROM_KNIGHT));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 7, index, Moves::PAWN)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }
}

void MoveGen::BlackPawnMoves(Position const* pos, MoveList* ml) noexcept
{
    BitBoard pawns = pos->Pieces<false, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos->PiecesByColour<true>() | (pos->EnPasBB() & ~Magics::RANK_6BB);

    pawn_move = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & ~Magics::RANK_1BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 8, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    } 

    pawn_move = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & Magics::RANK_1BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 8, index, Moves::PROM_QUEEN));
        ml->add(Moves::EncodeMove(index + 8, index, Moves::PROM_ROOK));
        ml->add(Moves::EncodeMove(index + 8, index, Moves::PROM_BISHOP));
        ml->add(Moves::EncodeMove(index + 8, index, Moves::PROM_KNIGHT));
        pawn_move = Magics::PopLS1B(pawn_move);
    } 
    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos->EmptySqs() & Shift<MD::SOUTH>(pos->EmptySqs()) & Magics::RANK_5BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 16, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index < 8)
        {   
            ml->add(Moves::EncodeMove(index + 7, index, Moves::PROM_QUEEN));
            ml->add(Moves::EncodeMove(index + 7, index, Moves::PROM_ROOK));
            ml->add(Moves::EncodeMove(index + 7, index, Moves::PROM_BISHOP));
            ml->add(Moves::EncodeMove(index + 7, index, Moves::PROM_KNIGHT));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 7, index, Moves::PAWN)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index < 8)
        {  
            ml->add(Moves::EncodeMove(index + 9, index, Moves::PROM_QUEEN));
            ml->add(Moves::EncodeMove(index + 9, index, Moves::PROM_ROOK));
            ml->add(Moves::EncodeMove(index + 9, index, Moves::PROM_BISHOP));
            ml->add(Moves::EncodeMove(index + 9, index, Moves::PROM_KNIGHT));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 9, index, Moves::PAWN)); 
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    } 
}