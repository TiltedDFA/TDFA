#include "MoveGen.hpp"
#include "Debug.hpp"

using Magics::Shift;

std::array<std::array<std::array<move_info, 2187>, 4>, 64> PrecomputeTitboards()
{
    std::array<std::array<std::array<move_info, 2187>, 4>, 64> result{};
    for(U8 sq = 0; sq < 64; ++sq)
    {
        for(U16 us = 0; us < 256; ++us)
        {
            for(U16 them = 0; them < 256; ++them)
            {
                //skipping useless blocker configurations
                if(us & them || (((~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) & ((~us) & Magics::BBRankOf(sq) || them & Magics::BBRankOf(sq)))) continue;

                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                const U8 rank_combined = (us | them) & ~Magics::BBFileOf(sq);
                U8 other_combined = (us | them) & ~Magics::BBRankOf(sq);

                const U8 rankofsq = Magics::RankOf(sq);
                const U8 fileofsq = Magics::FileOf(sq);

                if(us & Magics::BBFileOf(sq))
                {
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((rank_combined >> current_file) & 1)) //empty
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), mt_Quiet));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq + (current_file - fileofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), mt_Capture));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq + (current_file - fileofsq));
                            break;
                        }
                    }
                    for(int8_t current_file = fileofsq - 1; current_file >= 0 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((rank_combined >> current_file) & 1))
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), mt_Quiet));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq - (fileofsq - current_file));
                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), mt_Capture));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq - (fileofsq - current_file));
                            break;
                        }
                    }
                    const U16 p1 = Magics::base_2_to_3_us[fileofsq][us & ~Magics::BBFileOf(sq)];
                    const U16 p2 = 2 * Magics::base_2_to_3_us[fileofsq][them];
                    assert((p1 + p2 ) <= 2187);
                    result.at(sq).at(Rank).at(p1 + p2) = rank_attack_moves;
                }

                //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                if(us & Magics::BBRankOf(sq))
                {
                    for(int8_t current_file = rankofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((other_combined >> current_file) & 1)) //empty
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), mt_Quiet));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq + 8 * (current_file - rankofsq));

                            const Sq a_diag_attk = sq + 9 * (current_file - rankofsq);
                            if(Magics::ValidSq(a_diag_attk) && (Magics::SqToBB(a_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][Diagonal]))
                            {
                                diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_diag_attk);
                                diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_diag_attk, mt_Quiet));
                            }

                            const Sq a_anti_diag_attk = sq + 7 * (current_file - rankofsq);
                            if(Magics::ValidSq(a_anti_diag_attk) && (Magics::SqToBB(a_anti_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][AntiDiagonal]))
                            {
                                anti_diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_anti_diag_attk);
                                anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_anti_diag_attk, mt_Quiet));
                            }
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), mt_Capture));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq + 8 * (current_file - rankofsq));

                            const Sq a_diag_attk = sq + 9 * (current_file - rankofsq);
                            if(Magics::ValidSq(a_diag_attk) && (Magics::SqToBB(a_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][Diagonal]))
                            {
                                diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_diag_attk);
                                diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_diag_attk, mt_Capture));
                            }

                            const Sq a_anti_diag_attk = sq + 7 * (current_file - rankofsq);
                            if(Magics::ValidSq(a_anti_diag_attk) && (Magics::SqToBB(a_anti_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][AntiDiagonal]))
                            {
                                anti_diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_anti_diag_attk);
                                anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_anti_diag_attk, mt_Capture));
                            }
                            break;
                        }

                    }
                    for(int8_t current_file = rankofsq - 1; current_file >= 0 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((other_combined >> current_file) & 1))
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), mt_Quiet));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq - 8 * (rankofsq - current_file));

                            const Sq a_diag_attk = sq - 9 * (rankofsq - current_file);
                            if(Magics::ValidSq(a_diag_attk) && (Magics::SqToBB(a_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][Diagonal]))
                            {
                                diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_diag_attk);
                                diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_diag_attk, mt_Quiet));
                            }

                            const Sq a_anti_diag_attk = sq - 7 * (rankofsq - current_file);
                            if(Magics::ValidSq(a_anti_diag_attk) && (Magics::SqToBB(a_anti_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][AntiDiagonal]))
                            {
                                anti_diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_anti_diag_attk);
                                anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_anti_diag_attk, mt_Quiet));
                            }

                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), mt_Capture));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq - 8 * (rankofsq - current_file));

                            const Sq a_diag_attk = sq - 9 * (rankofsq - current_file);
                            if(Magics::ValidSq(a_diag_attk) && (Magics::SqToBB(a_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][Diagonal]))
                            {
                                diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_diag_attk);
                                diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_diag_attk, mt_Capture));
                            }

                            const Sq a_anti_diag_attk = sq - 7 * (rankofsq - current_file);
                            if(Magics::ValidSq(a_anti_diag_attk) && (Magics::SqToBB(a_anti_diag_attk) & Magics::SLIDING_ATTACKS_MASK[sq][AntiDiagonal]))
                            {
                                anti_diagonal_attack_moves.attacks_ |= Magics::SqToBB(a_anti_diag_attk);
                                anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, a_anti_diag_attk, mt_Capture));
                            }
                            break;
                        }
                    }

//                    diag_attacks        &= Magics::SLIDING_ATTACKS_MASK[sq][(int)Diagonal];
//                    anti_diag_attacks   &= Magics::SLIDING_ATTACKS_MASK[sq][(int)AntiDiagonal];
//
//                    diagonal_attack_moves.attacks_      = diag_attacks;
//                    anti_diagonal_attack_moves.attacks_ = anti_diag_attacks;
//
//                    while(diag_attacks)
//                    {
//                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, Magics::FindLS1B(diag_attacks),Bishop));
//                        diag_attacks = Magics::PopLS1B(diag_attacks);
//                    }
//                    while(anti_diag_attacks)
//                    {
//                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, Magics::FindLS1B(anti_diag_attacks),Bishop));
//                        anti_diag_attacks = Magics::PopLS1B(anti_diag_attacks);
//                    }
                    const U16 p1 = Magics::base_2_to_3_us[rankofsq][us & ~Magics::BBRankOf(sq)];
                    const U16 p2 = 2 * Magics::base_2_to_3_us[rankofsq][them];
                    assert((p1 + p2 ) <= 2187);
                    result.at(sq).at((U8)File           ).at(p1 + p2) = file_attack_moves;
                    result.at(sq).at((U8)Diagonal       ).at(p1 + p2) = diagonal_attack_moves;
                    result.at(sq).at((U8)AntiDiagonal   ).at(p1 + p2) = anti_diagonal_attack_moves;
                }
            }
        }
    }
    return result;
}

std::array<std::array<std::array<move_info, 2187>, 4>, 64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();

void MoveGen::WhitePawnMoves(Position const* pos, MoveList* ml) 
{
    const BitBoard pawns = pos->Pieces(White, pt_Pawn);
    if(!pawns) return;
    BitBoard pawn_move;
    const BitBoard capturable_squares = pos->Pieces(Black);

    pawn_move = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & ~Magics::RANK_8BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index - 8, index, mt_Quiet));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & Magics::RANK_8BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);

        ml->add(Moves::EncodeMove(index - 8, index, mt_QueenPromotion));
        ml->add(Moves::EncodeMove(index - 8, index, mt_RookPromotion));
        ml->add(Moves::EncodeMove(index - 8, index, mt_BishopPromotion));
        ml->add(Moves::EncodeMove(index - 8, index, mt_KnightPromotion));

        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTHNORTH>(pawns) & pos->EmptySqs() & Shift<MD::NORTH>(pos->EmptySqs()) & Magics::RANK_4BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index - 16, index, mt_Quiet));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index > 55)
        {
            ml->add(Moves::EncodeMove(index - 9, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index - 9, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index - 9, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index - 9, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 9, index, mt_Capture));
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    if((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx - 9, idx, mt_EnPassant));
    }
    pawn_move = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index > 55)
        {
            ml->add(Moves::EncodeMove(index - 7, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index - 7, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index - 7, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index - 7, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 7, index, mt_Quiet));
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }
    if((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_WEST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx - 7, idx, mt_EnPassant));
    }
}

void MoveGen::BlackPawnMoves(Position const* pos, MoveList* ml) 
{
    const BitBoard pawns = pos->Pieces(Black, pt_Pawn);
    if(!pawns) return;
    BitBoard pawn_move;
    const BitBoard capturable_squares = pos->Pieces(White);

    pawn_move = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & ~Magics::RANK_1BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 8, index, mt_Quiet));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & Magics::RANK_1BB;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 8, index, mt_QueenPromotion));
        ml->add(Moves::EncodeMove(index + 8, index, mt_RookPromotion));
        ml->add(Moves::EncodeMove(index + 8, index, mt_BishopPromotion));
        ml->add(Moves::EncodeMove(index + 8, index, mt_KnightPromotion));
        pawn_move = Magics::PopLS1B(pawn_move);
    }
    pawn_move = Shift<MD::SOUTHSOUTH>(pawns) & pos->EmptySqs() & Shift<MD::SOUTH>(pos->EmptySqs()) & Magics::RANK_5BB;
    while (pawn_move)
    {
        const int index = Magics::FindLS1B(pawn_move);
        ml->add(Moves::EncodeMove(index + 16, index, mt_Quiet));
        pawn_move = Magics::PopLS1B(pawn_move);
    }

    pawn_move = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index < 8)
        {
            ml->add(Moves::EncodeMove(index + 7, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index + 7, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index + 7, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index + 7, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 7, index, mt_Capture));
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }
    if((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_EAST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::SOUTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx + 7, idx, mt_EnPassant));
    }
    pawn_move = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    while (pawn_move)
    {
        const Sq index = Magics::FindLS1B(pawn_move);
        if(index < 8)
        {
            ml->add(Moves::EncodeMove(index + 9, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index + 9, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index + 9, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index + 9, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 9, index, mt_Capture));
        }
        pawn_move = Magics::PopLS1B(pawn_move);
    }
    if((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_WEST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::SOUTH_WEST>(pawns));
        ml->add(Moves::EncodeMove(idx + 9, idx, mt_EnPassant));
    }
}