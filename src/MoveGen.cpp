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

BitBoard MoveGen::GenerateAllWhiteAttacks(Position const* pos)
{
    BitBoard attacks{0ull};
    
    const BitBoard us = pos->PiecesByColour<true>();
    const BitBoard them = pos->PiecesByColour<false>();

    //King
    attacks |=  Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos->Pieces<true, loc::KING>())] 
                & (~pos->PiecesByColour<true>());
    //Sliding
    BitBoard bishop_queen = pos->Pieces<true, loc::QUEEN>() | pos->Pieces<true, loc::BISHOP>();
    while (bishop_queen)
    {
        const Sq piece_index = Magics::FindLS1B(bishop_queen);

        move_info const* move = GetMovesForSliding<Diagonal>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        move = GetMovesForSliding<AntiDiagonal>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        bishop_queen = Magics::PopLS1B(bishop_queen);
    }
    BitBoard rook_queen = pos->Pieces<true, loc::QUEEN>() | pos->Pieces<true, loc::ROOK>();
    while(rook_queen)
    {
        const Sq piece_index = Magics::FindLS1B(rook_queen);

        move_info const* move = GetMovesForSliding<File>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        move = GetMovesForSliding<Rank>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        rook_queen = Magics::PopLS1B(rook_queen);
    }
    //knights
    BitBoard knights = pos->Pieces<true, loc::KNIGHT>();
    while(knights)
    {
        attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~us;
        knights = Magics::PopLS1B(knights);
    }
    //pawns
    const BitBoard pawns = pos->Pieces<true, loc::PAWN>();
    attacks |= Magics::Shift<MD::NORTH_EAST>(pawns) & ~us;
    attacks |= Magics::Shift<MD::NORTH_WEST>(pawns) & ~us;
    return attacks;
}  

BitBoard MoveGen::GenerateAllBlackAttacks(Position const* pos)
{
    BitBoard attacks{0ull};
    
    const BitBoard us = pos->PiecesByColour<false>();
    const BitBoard them = pos->PiecesByColour<true>();
    
    //King
    attacks |=  Magics::KING_ATTACK_MASKS[Magics::FindLS1B(pos->Pieces<false, loc::KING>())] 
                & (~pos->PiecesByColour<false>());
    //Sliding
    BitBoard bishop_queen = pos->Pieces<false, loc::QUEEN>() | pos->Pieces<false, loc::BISHOP>();
    while (bishop_queen)
    {
        const Sq piece_index = Magics::FindLS1B(bishop_queen);

        move_info const* move = GetMovesForSliding<Diagonal>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        move = GetMovesForSliding<AntiDiagonal>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        bishop_queen = Magics::PopLS1B(bishop_queen);
    }
    BitBoard rook_queen = pos->Pieces<false, loc::QUEEN>() | pos->Pieces<false, loc::ROOK>();
    while(rook_queen)
    {
        const Sq piece_index = Magics::FindLS1B(rook_queen);

        move_info const* move = GetMovesForSliding<File>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        move = GetMovesForSliding<Rank>(piece_index, us, them);
        for(U8 i{0}; i < move->count; ++i)
                attacks |= Magics::SqToBB(Moves::TargetSq(move->encoded_move[i]));

        rook_queen = Magics::PopLS1B(rook_queen);
    }
    //knights
    BitBoard knights = pos->Pieces<false, loc::KNIGHT>();
    while(knights)
    {
        attacks |= Magics::KNIGHT_ATTACK_MASKS[Magics::FindLS1B(knights)] & ~us;
        knights = Magics::PopLS1B(knights);
    }
    //pawns
    const BitBoard pawns = pos->Pieces<false, loc::PAWN>();
    attacks |= Magics::Shift<MD::SOUTH_EAST>(pawns) & ~us;
    attacks |= Magics::Shift<MD::SOUTH_WEST>(pawns) & ~us;
    return attacks;
}