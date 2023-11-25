#include "MoveGen.hpp"
#include <algorithm>
using Magics::Shift;


MoveGen::MoveGen() 
    :pos_(){}
void MoveGen::WhitePawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<true, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<false>() | (pos_.GetEnPassantBB() & ~Magics::RANK_3BB);

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares() & ~Magics::RANK_8BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PAWN));
        }

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares() & Magics::RANK_8BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_BISHOP));
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_QUEEN));
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_ROOK));
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_KNIGHT));
        }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 16, index, Moves::PAWN));
        }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)  
        {
            if(index > 55)
            {   
                ml.add(Moves::EncodeMove(index - 9, index, Moves::PROM_BISHOP));
                ml.add(Moves::EncodeMove(index - 9, index, Moves::PROM_QUEEN));
                ml.add(Moves::EncodeMove(index - 9, index, Moves::PROM_ROOK));
                ml.add(Moves::EncodeMove(index - 9, index, Moves::PROM_KNIGHT));
            }
            else
            {
                ml.add(Moves::EncodeMove(index - 9, index, Moves::PAWN)); 
            }
        }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            if(index > 55)
            {   
                ml.add(Moves::EncodeMove(index - 7, index, Moves::PROM_BISHOP));
                ml.add(Moves::EncodeMove(index - 7, index, Moves::PROM_QUEEN));
                ml.add(Moves::EncodeMove(index - 7, index, Moves::PROM_ROOK));
                ml.add(Moves::EncodeMove(index - 7, index, Moves::PROM_KNIGHT));
            }
            else
            {
                ml.add(Moves::EncodeMove(index - 7, index, Moves::PAWN)); 
            }
        }
}
void MoveGen::BlackPawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<false,loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<true>() | (pos_.GetEnPassantBB() & ~Magics::RANK_6BB);

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares() & ~Magics::RANK_1BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)
        {
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PAWN));
        } 

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares() & Magics::RANK_1BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)
        {
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_BISHOP));
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_QUEEN));
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_ROOK));
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_KNIGHT));
        } 
    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::SOUTH>(pos_.GetEmptySquares()) & Magics::RANK_5BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 16, index, Moves::PAWN));
        } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            if(index < 8)
            {   
                ml.add(Moves::EncodeMove(index + 7, index, Moves::PROM_BISHOP));
                ml.add(Moves::EncodeMove(index + 7, index, Moves::PROM_QUEEN));
                ml.add(Moves::EncodeMove(index + 7, index, Moves::PROM_ROOK));
                ml.add(Moves::EncodeMove(index + 7, index, Moves::PROM_KNIGHT));
            }
            else
            {
                ml.add(Moves::EncodeMove(index + 7, index, Moves::PAWN)); 
            }
        } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            if(index < 8)
            {  
                ml.add(Moves::EncodeMove(index + 9, index, Moves::PROM_BISHOP));
                ml.add(Moves::EncodeMove(index + 9, index, Moves::PROM_QUEEN));
                ml.add(Moves::EncodeMove(index + 9, index, Moves::PROM_ROOK));
                ml.add(Moves::EncodeMove(index + 9, index, Moves::PROM_KNIGHT));
            }
            else
            {
                ml.add(Moves::EncodeMove(index + 9, index, Moves::PAWN)); 
            }
        } 
}