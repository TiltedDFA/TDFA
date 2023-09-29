#include "MoveGen.hpp"
#include <algorithm>
using Magics::Shift;


MoveGen::MoveGen() 
    :pos_(),w_atks_(0ull),b_atks_(0ull)
    {}
void MoveGen::GenerateAllMoves(const BB::Position& pos, Move** move_list)
{
    pos_ = pos;
    w_atks_ = 0ull;
    b_atks_ = 0ull;

    WhitePawnMoves(move_list, pos_.GetPieces<true>(), EnPassantTargetSquare);
    BlackPawnMoves(move_list, pos_.GetPieces<false>(), EnPassantTargetSquare);

    RookMoves<true>(move_list, pos_.GetSpecificPieces<loc::WHITE, loc::ROOK>());
    RookMoves<false>(move_list, pos_.GetSpecificPieces<loc::BLACK, loc::ROOK>());

    KnightMoves<true>(move_list, pos_.GetSpecificPieces<loc::WHITE, loc::KNIGHT>());
    KnightMoves<false>(move_list, pos_.GetSpecificPieces<loc::BLACK, loc::KNIGHT>());

    BishopMoves<true>(move_list, pos_.GetSpecificPieces<loc::WHITE, loc::BISHOP>());
    BishopMoves<false>(move_list, pos_.GetSpecificPieces<loc::BLACK, loc::BISHOP>());

    QueenMoves<true>(move_list, pos_.GetSpecificPieces<loc::WHITE, loc::QUEEN>());
    QueenMoves<false>(move_list, pos_.GetSpecificPieces<loc::BLACK, loc::QUEEN>());
    
    WhiteKingMoves(move_list, pos_.GetSpecificPieces<loc::WHITE, loc::KING>());
    BlackKingMoves(move_list, pos_.GetSpecificPieces<loc::BLACK, loc::KING>());
}
void MoveGen::WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept
{
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<false>() | en_passant_target_sq;

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares();
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            *(*move_list)++ = Moves::EncodeMove(index-8,index,Moves::PAWN,1);
            w_atks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            *(*move_list)++ = Moves::EncodeMove(index-16,index,Moves::PAWN,1);
            w_atks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1)  
        {
            *(*move_list)++ = Moves::EncodeMove(index-9,index,Moves::PAWN,1);
            w_atks_  |= Magics::IndexToBB(index);
        }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            *(*move_list)++ = Moves::EncodeMove(index-7,index,Moves::PAWN,1);
            w_atks_  |= Magics::IndexToBB(index);
        }
}
void MoveGen::BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept
{
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<true>() | en_passant_target_sq;

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares();
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1)
        {
            *(*move_list)++ = Moves::EncodeMove(index+8,index,Moves::PAWN,0);
            b_atks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            *(*move_list)++ = Moves::EncodeMove(index+16,index,Moves::PAWN,0);
            b_atks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            *(*move_list)++ = Moves::EncodeMove(index+7,index,Moves::PAWN,0); 
            b_atks_  |= Magics::IndexToBB(index);
        } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) 
        {
            *(*move_list)++ = Moves::EncodeMove(index+9,index,Moves::PAWN,0);
            b_atks_  |= Magics::IndexToBB(index);
        } 

}
void MoveGen::WhiteKingMoves(Move** move_list, BitBoard king) noexcept
{
    const uint8_t king_index = Magics::FindLS1B(king);
    BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (pos_.GetEmptySquares() | (pos_.GetPieces<false>() & ~pos_.GetPieces<true>()));
    while(king_attacks)
    {
        *(*move_list)++ = Moves::EncodeMove(king_index,Magics::FindLS1B(king_attacks),Moves::KING,1);
        w_atks_  |= Magics::IndexToBB(Magics::FindLS1B(king_attacks));
        king_attacks = Magics::PopLS1B(king_attacks);
    }
}
void MoveGen::BlackKingMoves(Move** move_list, BitBoard king) noexcept
{
    const uint8_t king_index = Magics::FindLS1B(king);
    BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (pos_.GetEmptySquares() | (~pos_.GetPieces<false>() & pos_.GetPieces<true>()));
    while(king_attacks)
    {
        *(*move_list)++ = Moves::EncodeMove(king_index,Magics::FindLS1B(king_attacks),Moves::KING,0);
        b_atks_  |= Magics::IndexToBB(Magics::FindLS1B(king_attacks));
        king_attacks = Magics::PopLS1B(king_attacks);
    }
}