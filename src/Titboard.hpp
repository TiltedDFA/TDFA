#ifndef TITBOARD_HPP
#define TITBOARD_HPP

#include "MagicConstants.hpp"
#include "Types.hpp"
#include <array>

inline std::array<std::array<std::array<move_info, 2187>, 4>, 64> PrecomputeTitboards()
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

                BitBoard diag_attacks = 0ull;
                BitBoard anti_diag_attacks = 0ull;

                if(us & Magics::BBFileOf(sq))
                {
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((rank_combined >> current_file) & 1)) //empty
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Rook));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq + (current_file - fileofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Rook));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq + (current_file - fileofsq));
                            break;
                        }
                    }
                    for(int8_t current_file = fileofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((rank_combined >> current_file) & 1)) 
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Rook));
                            rank_attack_moves.attacks_ |= Magics::SqToBB(sq - (fileofsq - current_file));
                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Rook));
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
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Rook));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq + 8 * (current_file - rankofsq));
                            
                            if(Magics::ValidSq(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::SqToBB(sq + 9 * (current_file - rankofsq));
                            
                            if(Magics::ValidSq(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::SqToBB(sq +  7 * (current_file - rankofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Rook));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq + 8 * (current_file - rankofsq));
                            
                            if(Magics::ValidSq(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::SqToBB(sq + 9 * (current_file - rankofsq));
                                
                            if(Magics::ValidSq(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::SqToBB(sq +  7 * (current_file - rankofsq));
                            break;
                        }
                        
                    }
                    for(int8_t current_file = rankofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((other_combined >> current_file) & 1)) 
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Rook));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq - 8 * (rankofsq - current_file));

                            if(Magics::ValidSq(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::SqToBB(sq - 9 * (rankofsq - current_file));
                            
                            if(Magics::ValidSq(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::SqToBB(sq -  7 * (rankofsq - current_file));

                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Rook));
                            file_attack_moves.attacks_ |= Magics::SqToBB(sq - 8 * (rankofsq - current_file));

                            if(Magics::ValidSq(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::SqToBB(sq - 9 * (rankofsq - current_file));

                            if(Magics::ValidSq(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::SqToBB(sq -  7 * (rankofsq - current_file));
                            break;
                        }
                    }

                    diag_attacks        &= Magics::SLIDING_ATTACKS_MASK[sq][(int)Diagonal];
                    anti_diag_attacks   &= Magics::SLIDING_ATTACKS_MASK[sq][(int)AntiDiagonal];

                    diagonal_attack_moves.attacks_      = diag_attacks;
                    anti_diagonal_attack_moves.attacks_ = anti_diag_attacks;

                    while(diag_attacks)
                    {
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq, Magics::FindLS1B(diag_attacks),Bishop));
                        diag_attacks = Magics::PopLS1B(diag_attacks);
                    }
                    while(anti_diag_attacks)
                    {
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq, Magics::FindLS1B(anti_diag_attacks),Bishop));
                        anti_diag_attacks = Magics::PopLS1B(anti_diag_attacks);
                    }
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
#endif // #ifndef TITBOARD_HPP