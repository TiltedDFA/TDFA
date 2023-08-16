#include "MoveGen.hpp"
using Magics::Shift;


MoveGen::MoveGen() 
    :white_pieces_(0ull),black_pieces_(0ull),empty_squares_(~(0ull))
{}
MoveList MoveGen::GenerateAllMoves(const BB::Position& pos)
{
    UpdateVariables(pos.GetPieces<true>(),pos.GetPieces<false>());
    MoveList moves;

    WhitePawnMoves(moves.Current(),white_pieces_,EnPassantTargetSquare);
    BlackPawnMoves(moves.Current(),black_pieces_,EnPassantTargetSquare);

    RookMoves<true>(moves.Current(),pos.GetSpecificPieces<BB::loc::WHITE,BB::loc::ROOK>());
    RookMoves<false>(moves.Current(),pos.GetSpecificPieces<BB::loc::BLACK,BB::loc::ROOK>());

    KnightMoves<true>(moves.Current(),pos.GetSpecificPieces<BB::loc::WHITE,BB::loc::KNIGHT>());
    KnightMoves<false>(moves.Current(),pos.GetSpecificPieces<BB::loc::BLACK,BB::loc::KNIGHT>());

    BishopMoves<true>(moves.Current(),pos.GetSpecificPieces<BB::loc::WHITE,BB::loc::BISHOP>());
    BishopMoves<false>(moves.Current(),pos.GetSpecificPieces<BB::loc::BLACK,BB::loc::BISHOP>());

    QueenMoves<true>(moves.Current(),pos.GetSpecificPieces<BB::loc::WHITE,BB::loc::QUEEN>());
    QueenMoves<false>(moves.Current(),pos.GetSpecificPieces<BB::loc::BLACK,BB::loc::QUEEN>());
    
    WhiteKingMoves(moves.Current(),pos.GetSpecificPieces<BB::loc::WHITE, BB::loc::KING>());
    BlackKingMoves(moves.Current(),pos.GetSpecificPieces<BB::loc::BLACK, BB::loc::KING>());
    return moves;
}
void MoveGen::UpdateVariables(BitBoard white_pieces, BitBoard black_pieces)
{
    white_pieces_ = white_pieces;
    black_pieces_ = black_pieces;
    empty_squares_ = ~(white_pieces | black_pieces);
}
void MoveGen::WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept
{
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = black_pieces_ | en_passant_target_sq;

    pawn_move = Shift<MD::NORTH>(pawns) & empty_squares_;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-8,index,Moves::PAWN,1);

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & empty_squares_ & Shift<MD::NORTH>(empty_squares_) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-16,index,Moves::PAWN,1);

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-9,index,Moves::PAWN,1); 

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-7,index,Moves::PAWN,1);
}
void MoveGen::BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept
{
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = white_pieces_ | en_passant_target_sq;

    pawn_move = Shift<MD::SOUTH>(pawns) & empty_squares_;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+8,index,Moves::PAWN,1);

    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & empty_squares_ & Shift<MD::NORTH>(empty_squares_) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+16,index,Moves::PAWN,1);

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+7,index,Moves::PAWN,1); 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+9,index,Moves::PAWN,1);
}
void MoveGen::WhiteKingMoves(Move** move_list, BitBoard king) noexcept
{
    const uint8_t king_index = Magics::FindLS1B(king);
    BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (empty_squares_ | (black_pieces_ & ~white_pieces_));
    while(king_attacks)
    {
        *(*move_list)++ = Moves::EncodeMove(king_index,Magics::FindLS1B(king_attacks),Moves::KING,1);
        king_attacks = Magics::PopLSB(king_attacks);
    }
}
void MoveGen::BlackKingMoves(Move** move_list, BitBoard king) noexcept
{
    const uint8_t king_index = Magics::FindLS1B(king);
    BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (empty_squares_ | (~black_pieces_ & white_pieces_));
    while(king_attacks)
    {
        *(*move_list)++ = Moves::EncodeMove(king_index,Magics::FindLS1B(king_attacks),Moves::KING,0);
        king_attacks = Magics::PopLSB(king_attacks);
    }
}