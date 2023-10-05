#include "MoveGen.hpp"
#include <algorithm>
using Magics::Shift;


MoveGen::MoveGen() 
    :pos_(),w_atks_(0ull),b_atks_(0ull)
    {}
void MoveGen::GenerateAllMoves(const BB::Position& pos, MoveList& ml)
{
    pos_ = pos;
    w_atks_ = 0ull;
    b_atks_ = 0ull;

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
    w_atks_ = 0ull;
    WhitePawnMoves(ml);
    BishopMoves<true>(ml);
    RookMoves<true>(ml);
    KnightMoves<true>(ml);
    QueenMoves<true>(ml);
    KingMoves<true>(ml);
    Castling<true>(ml); // can't do this!!!
    return w_atks_;
}
BitBoard MoveGen::GenerateAllBlackMoves(const BB::Position& p, MoveList& ml)
{
    pos_ = p;
    b_atks_ = 0ull;
    BlackPawnMoves(ml);
    BishopMoves<false>(ml);
    RookMoves<false>(ml);
    KnightMoves<false>(ml);
    QueenMoves<false>(ml);
    KingMoves<false>(ml);
    Castling<false>(ml);
    return b_atks_;
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
            w_atks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 16, index, Moves::PAWN, 1));
            w_atks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1)  
        {
            ml.add(Moves::EncodeMove(index - 9, index, Moves::PAWN, 1));
            w_atks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index - 7, index, Moves::PAWN, 1));
            w_atks_  |= Magics::IndexToBB(index);
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
            b_atks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::SOUTH>(pos_.GetEmptySquares()) & Magics::RANK_5BB;
    for(int index = Magics::FindLS1B(pawn_move); index < 64; ++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 16, index, Moves::PAWN, 0));
            b_atks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 7, index, Moves::PAWN, 0)); 
            b_atks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move); index < 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            ml.add(Moves::EncodeMove(index + 9, index, Moves::PAWN, 0));
            b_atks_  |= Magics::IndexToBB(index);
        } 
}