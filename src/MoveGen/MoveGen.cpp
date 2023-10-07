#include "MoveGen.hpp"
#include <algorithm>
using Magics::Shift;


MoveGen::MoveGen() 
    :pos_(),attacks_(0){}
void MoveGen::GenerateAllMoves(const BB::Position& pos, MoveList& ml)
{
    pos_ = pos;
    attacks_ = 0;

    WhitePawnMoves(ml);
    BlackPawnMoves(ml);

    BishopMoves<true>(ml);
    BishopMoves<false>(ml);

    RookMoves<true>(ml);
    RookMoves<false>(ml);

    KnightMoves<true>(ml);
    KnightMoves<false>(ml);

    QueenMoves<true>(ml);
    QueenMoves<false>(ml);
    
    KingMoves<true>(ml);
    KingMoves<false>(ml);

    Castling<true>(ml);
    Castling<false>(ml);
}
BitBoard MoveGen::GenerateAllWhiteMoves(const BB::Position& p, MoveList& ml)
{
    pos_ = p;
    attacks_ = 0;
    WhitePawnMoves(ml);
    BishopMoves<true>(ml);
    RookMoves<true>(ml);
    KnightMoves<true>(ml);
    QueenMoves<true>(ml);
    KingMoves<true>(ml);
    Castling<true>(ml); // can't do this!!!
    return attacks_;
}
BitBoard MoveGen::GenerateAllBlackMoves(const BB::Position& p, MoveList& ml)
{
    pos_ = p;
    attacks_ = 0;
    BlackPawnMoves(ml);
    BishopMoves<false>(ml);
    RookMoves<false>(ml);
    KnightMoves<false>(ml);
    QueenMoves<false>(ml);
    KingMoves<false>(ml);
    Castling<false>(ml);
    return attacks_;
}
void MoveGen::WhitePawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<loc::WHITE, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<false>() | pos_.GetEnPassantBB();

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares();
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 8, index, Moves::PAWN, 1));
            attacks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 16, index, Moves::PAWN, 1));
            attacks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)  
        {
            ml.add(Moves::EncodeMove(index - 9, index, Moves::PAWN, 1));
            attacks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 7, index, Moves::PAWN, 1));
            attacks_  |= Magics::IndexToBB(index);
        }
}
void MoveGen::BlackPawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<loc::BLACK,loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<true>() | pos_.GetEnPassantBB();

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares();
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)
        {
            ml.add(Moves::EncodeMove(index + 8, index, Moves::PAWN, 0));
            attacks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::SOUTH>(pos_.GetEmptySquares()) & Magics::RANK_5BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 16, index, Moves::PAWN, 0));
            attacks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 7, index, Moves::PAWN, 0)); 
            attacks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 9, index, Moves::PAWN, 0));
            attacks_  |= Magics::IndexToBB(index);
        } 
}