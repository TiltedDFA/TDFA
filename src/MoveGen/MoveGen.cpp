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

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares();
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PAWN, 1));
        }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 16, index, Moves::PAWN, 1));
        }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)  
        {
            ml.add(Moves::EncodeMove(index - 9, index, Moves::PAWN, 1));
        }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 7, index, Moves::PAWN, 1));
        }
}
void MoveGen::BlackPawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<false,loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<true>() | (pos_.GetEnPassantBB() & ~Magics::RANK_6BB);

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares();
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)
        {
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PAWN, 0));
        } 

    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::SOUTH>(pos_.GetEmptySquares()) & Magics::RANK_5BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 16, index, Moves::PAWN, 0));
        } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 7, index, Moves::PAWN, 0)); 
        } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 9, index, Moves::PAWN, 0));
        } 
}