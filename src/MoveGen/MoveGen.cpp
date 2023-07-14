#include "MoveGen.hpp"
using Magics::Shift;

MoveGen* MoveGen::instance_ = nullptr;

MoveGen::MoveGen() 
    :white_pieces_(0ull),black_pieces_(0ull),empty_squares_(~(0ull))
{
    assert(instance_ == nullptr);
    instance_ = this;
}
void MoveGen::UpdateVariables(BitBoard white_pieces, BitBoard black_pieces, BitBoard empty_squares)
{
    white_pieces_ = white_pieces;
    black_pieces_ = black_pieces;
    empty_squares_ = empty_squares;
}
void MoveGen::WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept
{
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = black_pieces_ | en_passant_target_sq;

    pawn_move = Shift<MD::NORTH>(pawns) & empty_squares_;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-8,index,Magics::PAWN,1);

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & empty_squares_ & Shift<MD::NORTH>(empty_squares_) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-16,index,Magics::PAWN,1);

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-9,index,Magics::PAWN,1); 

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index-7,index,Magics::PAWN,1);
}
void MoveGen::BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept
{
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = white_pieces_ | en_passant_target_sq;

    pawn_move = Shift<MD::SOUTH>(pawns) & empty_squares_;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+8,index,Magics::PAWN,1);

    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & empty_squares_ & Shift<MD::NORTH>(empty_squares_) & Magics::RANK_4BB;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+16,index,Magics::PAWN,1);

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+7,index,Magics::PAWN,1); 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    for(int index = Magics::FindLS1B(pawn_move);index< 64;++index)
        if ((pawn_move >> index) & 1) *(*move_list)++ = Moves::EncodeMove(index+9,index,Magics::PAWN,1);
}
void MoveGen::WhiteKnightMoves(Move** move_list, BitBoard knights) noexcept
{
    if(!knights) return;
    while(knights)
    {
        const uint8_t knight_index = Magics::FindLS1B(knights);
        BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & (empty_squares_ 
                                 | (black_pieces_ & ~white_pieces_));
        while(possible_move)
        {
            const uint8_t attack_index = Magics::FindLS1B(possible_move);
            *(*move_list)++ = Moves::EncodeMove(knight_index,attack_index,Magics::KNIGHT,1);
            possible_move = Magics::PopLSB(possible_move);
        }
        knights = Magics::PopLSB(knights);
    }
}
void MoveGen::BlackKnightMoves(Move** move_list, BitBoard knights) noexcept
{
    if(!knights) return;
    while(knights)
    {
        const uint8_t knight_index = Magics::FindLS1B(knights);
        BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & (empty_squares_ 
                                 | (~black_pieces_ & white_pieces_));
        while(possible_move)
        {
            const uint8_t attack_index = Magics::FindLS1B(possible_move);
            *(*move_list)++ = Moves::EncodeMove(knight_index,attack_index,Magics::KNIGHT,0);
            possible_move = Magics::PopLSB(possible_move);
        }
        knights = Magics::PopLSB(knights);
    }
}
void MoveGen::WhiteKingMoves(Move** move_list, BitBoard king) noexcept
{
    const uint8_t king_index = Magics::FindLS1B(king);
    BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (empty_squares_ | (black_pieces_ & ~white_pieces_));
    while(king_attacks)
    {
        *(*move_list)++ = Moves::EncodeMove(king_index,Magics::FindLS1B(king_attacks),Magics::KING,1);
        king_attacks = Magics::PopLSB(king_attacks);
    }
}
void MoveGen::BlackKingMoves(Move** move_list, BitBoard king) noexcept
{
    const uint8_t king_index = Magics::FindLS1B(king);
    BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (empty_squares_ | (~black_pieces_ & white_pieces_));
    while(king_attacks)
    {
        *(*move_list)++ = Moves::EncodeMove(king_index,Magics::FindLS1B(king_attacks),Magics::KING,0);
        king_attacks = Magics::PopLSB(king_attacks);
    }
}