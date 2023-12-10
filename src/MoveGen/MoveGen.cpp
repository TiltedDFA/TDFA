#include "MoveGen.hpp"
#include <algorithm>
using Magics::Shift;


MoveGen::MoveGen(BB::Position& current_pos):pos_(current_pos){};

void MoveGen::WhitePawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<true, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<false>() | (pos_.GetEnPassantBB() & ~Magics::RANK_3BB);

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares() & ~Magics::RANK_8BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml.add(Moves::EncodeMove(index - 8, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH>(pawns) & pos_.GetEmptySquares() & Magics::RANK_8BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);

        ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_BISHOP));
        ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_QUEEN));
        ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_ROOK));
        ml.add(Moves::EncodeMove(index - 8, index, Moves::PROM_KNIGHT));

        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::NORTH>(pos_.GetEmptySquares()) & Magics::RANK_4BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml.add(Moves::EncodeMove(index - 16, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
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
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
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
        pawn_move = Magics::PopLS1B(pawn_move);
    }
}

void MoveGen::BlackPawnMoves(MoveList& ml) noexcept
{
    BitBoard pawns = pos_.GetSpecificPieces<false, loc::PAWN>();
    if(!pawns) return;
    BitBoard pawn_move{0};
    const BitBoard capturable_squares = pos_.GetPieces<true>() | (pos_.GetEnPassantBB() & ~Magics::RANK_6BB);

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares() & ~Magics::RANK_1BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml.add(Moves::EncodeMove(index + 8, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    } 

    pawn_move = Shift<MD::SOUTH>(pawns) & pos_.GetEmptySquares() & Magics::RANK_1BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_BISHOP));
        ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_QUEEN));
        ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_ROOK));
        ml.add(Moves::EncodeMove(index + 8, index, Moves::PROM_KNIGHT));
        pawn_move = Magics::PopLS1B(pawn_move);
    } 
    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos_.GetEmptySquares() & Shift<MD::SOUTH>(pos_.GetEmptySquares()) & Magics::RANK_5BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml.add(Moves::EncodeMove(index + 16, index, Moves::PAWN));
        pawn_move = Magics::PopLS1B(pawn_move);
    } 

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
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
        pawn_move = Magics::PopLS1B(pawn_move);
    } 

    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
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
        pawn_move = Magics::PopLS1B(pawn_move);
    } 
}

BitBoard MoveGen::GenerateAllWhiteAttacks(const BB::Position& pos)
{
    BitBoard attacks{0ull};
    
    const BitBoard us = pos.GetPieces<true>();
    const BitBoard them = pos.GetPieces<false>();

    //King
    attacks |=  Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos.GetSpecificPieces<true, loc::KING>())] 
                & (~pos.GetPieces<true>());
    //Sliding
    BitBoard bishop_queen = pos.GetSpecificPieces<true, loc::QUEEN>() | pos.GetSpecificPieces<true, loc::BISHOP>();
    while (bishop_queen)
    {
        const Sq piece_index = Magics::FindLS1B(bishop_queen);

        move_info* move = const_cast<move_info*>(GetMovesForSliding<D::DIAG>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        move = const_cast<move_info*>(GetMovesForSliding<D::ADIAG>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        bishop_queen = Magics::PopLS1B(bishop_queen);
    }
    BitBoard rook_queen = pos.GetSpecificPieces<true, loc::QUEEN>() | pos.GetSpecificPieces<true, loc::ROOK>();
    while(rook_queen)
    {
        const Sq piece_index = Magics::FindLS1B(rook_queen);

        move_info* move = const_cast<move_info*>(GetMovesForSliding<D::FILE>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        move = const_cast<move_info*>(GetMovesForSliding<D::RANK>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        rook_queen = Magics::PopLS1B(rook_queen);
    }
    //knights
    BitBoard knights = pos.GetSpecificPieces<true, loc::KNIGHT>();
    while(knights)
    {
        attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~us;
        knights = Magics::PopLS1B(knights);
    }
    //pawns
    const BitBoard pawns = pos.GetSpecificPieces<true, loc::PAWN>();
    attacks |= Magics::Shift<MD::NORTH_EAST>(pawns) & ~us;
    attacks |= Magics::Shift<MD::NORTH_WEST>(pawns) & ~us;
    return attacks;
}  
BitBoard MoveGen::GenerateAllBlackAttacks(const BB::Position& pos)
{
    BitBoard attacks{0ull};
    
    const BitBoard us = pos.GetPieces<false>();
    const BitBoard them = pos.GetPieces<true>();
    
    //King
    attacks |=  Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos.GetSpecificPieces<false, loc::KING>())] 
                & (~pos.GetPieces<false>());
    //Sliding
    BitBoard bishop_queen = pos.GetSpecificPieces<false, loc::QUEEN>() | pos.GetSpecificPieces<false, loc::BISHOP>();
    while (bishop_queen)
    {
        const Sq piece_index = Magics::FindLS1B(bishop_queen);

        move_info* move = const_cast<move_info*>(GetMovesForSliding<D::DIAG>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        move = const_cast<move_info*>(GetMovesForSliding<D::ADIAG>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        bishop_queen = Magics::PopLS1B(bishop_queen);
    }
    BitBoard rook_queen = pos.GetSpecificPieces<false, loc::QUEEN>() | pos.GetSpecificPieces<false, loc::ROOK>();
    while(rook_queen)
    {
        const Sq piece_index = Magics::FindLS1B(rook_queen);

        move_info* move = const_cast<move_info*>(GetMovesForSliding<D::FILE>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        move = const_cast<move_info*>(GetMovesForSliding<D::RANK>(piece_index, us, them));
        for(uint8_t i{0}; i < move->count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move->encoded_move[i]));

        rook_queen = Magics::PopLS1B(rook_queen);
    }
    //knights
    BitBoard knights = pos.GetSpecificPieces<false, loc::KNIGHT>();
    while(knights)
    {
        attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~us;
        knights = Magics::PopLS1B(knights);
    }
    //pawns
    const BitBoard pawns = pos.GetSpecificPieces<false, loc::PAWN>();
    attacks |= Magics::Shift<MD::SOUTH_EAST>(pawns) & ~us;
    attacks |= Magics::Shift<MD::SOUTH_WEST>(pawns) & ~us;
    return attacks;
}