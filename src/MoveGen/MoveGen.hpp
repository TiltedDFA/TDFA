#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"
using Magics::file_of;
/**
 * The reason that we'd need an array of [64][4][2187] despite the blocker configurations being the same in any direction
 * would be due to the target indexs recorded being different based on the diretions.
 * As of writing this (26/7/23 03:09) the only things that I think would differe between the directions in terms of how
 * we precompute the moves would be how the moves are recorded(target indexes vairing) 
*/
consteval std::array<std::array<std::array<move_info,2187>,4>,64> PrecomputeTitboards()
{
    std::array<std::array<std::array<move_info,2187>,4>,64> result{};
    for(uint8_t sq = 0; sq < 64; ++sq)
    {
        for(uint16_t us = 0; us < 256;++us)
        {
            for(uint16_t them = 0; them < 256;++them)
            {
                //skiping useless blocker configs
                if(us & them || (~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) continue;

                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                uint8_t combined = (us | them) & ~file_of(sq);
                
                uint64_t diag_attacks = 0ull;
                uint64_t anti_diag_attacks = 0ull;
                
                for(int8_t current_file = file_of(sq) + 1; current_file < 8; ++current_file)
                {
                    if(!((combined >> current_file) & 1)) //empty
                    {
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - file_of(sq)), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - file_of(sq)), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq + 9 * (current_file - file_of(sq))))
                            diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - file_of(sq)));
                        if(Magics::IndexInBounds(sq -  7 * (current_file - file_of(sq))))
                            anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (current_file - file_of(sq)));
                        continue;
                    }

                    if((us >> current_file) & 1) break; //our piece

                    if((them >> current_file) & 1) //their piece
                    {
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - file_of(sq)), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - file_of(sq)), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq + 9 * (current_file - file_of(sq))))
                            diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - file_of(sq)));
                        if(Magics::IndexInBounds(sq -  7 * (current_file - file_of(sq))))
                            anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (current_file - file_of(sq)));
                        break;
                    }

                }
                for(int8_t current_file = file_of(sq) - 1; current_file > - 1 ; --current_file)
                {
                    if(!((combined >> current_file) & 1)) 
                    {
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (file_of(sq) - current_file), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (file_of(sq) - current_file), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq - 9 * (current_file - file_of(sq))))
                            diag_attacks |= Magics::IndexToBB(sq - 9 * (current_file - file_of(sq)));
                        if(Magics::IndexInBounds(sq +  7 * (current_file - file_of(sq))))
                            anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - file_of(sq)));

                        continue;
                    }

                    if((us >> current_file) & 1) break;
                    if((them >> current_file) & 1)
                    {
                        file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (file_of(sq) - current_file), Moves::ROOK, 1));

                        rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (file_of(sq) - current_file), Moves::ROOK, 1));

                        if(Magics::IndexInBounds(sq - 9 * (current_file - file_of(sq))))
                            diag_attacks |= Magics::IndexToBB(sq - 9 * (current_file - file_of(sq)));
                        if(Magics::IndexInBounds(sq +  7 * (current_file - file_of(sq))))
                            anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - file_of(sq)));
                        break;
                    }
                }
                diag_attacks &= Magics::SLIDING_ATTACKS_MASK[file_of(sq)][(int)D::DIAG];
                anti_diag_attacks &= Magics::SLIDING_ATTACKS_MASK[file_of(sq)][(int)D::ADIAG];

                while(diag_attacks)
                {
                    diagonal_attack_moves.add_move(Moves::EncodeMove(sq,Magics::FindLS1B(diag_attacks),Moves::BISHOP,1));
                    diag_attacks = Magics::PopLS1B(diag_attacks);
                }
                while(anti_diag_attacks)
                {
                    diagonal_attack_moves.add_move(Moves::EncodeMove(sq,Magics::FindLS1B(anti_diag_attacks),Moves::BISHOP,1));
                    anti_diag_attacks = Magics::PopLS1B(anti_diag_attacks);
                }
                uint16_t p1 = Magics::base_2_to_3[file_of(sq)][us];
                uint16_t p2 = 2 * Magics::base_2_to_3[file_of(sq)][them];
                uint16_t index = p1 + p2;

                result.at(sq).at(0).at(index) = file_attack_moves;
                result.at(sq).at(1).at(index) = rank_attack_moves;
                
                //needing to do this since in some positons across diagonals, bishops have less than 8 moves meaning the full blocker config cannot be used to index
                uint16_t cpy_us = us;
                uint16_t cpy_them = them;
                while(!(combined & 1))
                {
                    cpy_us >>= 1;
                    cpy_them >>= 1;
                    combined >>= 1;
                }
                p1 = Magics::base_2_to_3[file_of(sq)][cpy_us];
                p2 = 2 * Magics::base_2_to_3[file_of(sq)][cpy_them];
                index = p1 + p2;

                result.at(sq).at(2).at(index) = diagonal_attack_moves;
                result.at(sq).at(3).at(index) = anti_diagonal_attack_moves;
            }
        }
    }
    return result;
}

class MoveGen
{
public:
    MoveGen();
    
    [[nodiscard]] MoveList GenerateAllMoves(const BB::Position& pos);
    
    template<D direction>
    static constexpr move_info GetMovesForSliding(uint8_t piece_sq, BitBoard us, BitBoard them) noexcept
    {
        return SLIDING_ATTACK_CONFIG[piece_sq][static_cast<int>(direction)]
                [
                    //us
                    (Magics::base_2_to_3
                    [(direction == D::RANK) ? Magics::file_of(piece_sq) 
                    : Magics::rank_of(piece_sq)]
                    [(direction == D::RANK) ? Magics::CollapsedRanksIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])
                    : Magics::CollapsedRanksIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])])
                    +
                    //them
                    (2 * Magics::base_2_to_3
                    [(direction == D::RANK) ? Magics::file_of(piece_sq) 
                    : Magics::rank_of(piece_sq)]
                    [(direction == D::RANK) ? Magics::CollapsedRanksIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])
                    : Magics::CollapsedRanksIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])])
                ];
    }
private:

    void UpdateVariables(BitBoard white_pieces, BitBoard black_pieces);

    void WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    void BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;

    template<bool is_white>
    void BishopMoves(Move** move_list,BitBoard bishops)
    {
        if(!bishops) return;

        while(bishops)
        {
            const uint8_t bishop_index = Magics::FindLS1B(bishops);

            move_info move = GetMovesForSliding<D::DIAG>(bishop_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::BISHOP>(move.encoded_move[i]);
            }
            
            move = GetMovesForSliding<D::ADIAG>(bishop_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::BISHOP>(move.encoded_move[i]);
            }
            
            bishops = Magics::PopLS1B(bishops);
        }
    }

    template<bool is_white>
    void RookMoves(Move** move_list, BitBoard rooks)
    {
        if(!rooks) return;

        while(rooks)
        {
            const uint8_t rook_index = Magics::FindLS1B(rooks);

            move_info move = GetMovesForSliding<D::FILE>(rook_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetColour<is_white>(move.encoded_move[i]);
            }

            move = GetMovesForSliding<D::RANK>(rook_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetColour<is_white>(move.encoded_move[i]);
            }
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<bool is_white>
    void KnightMoves(Move** move_list, BitBoard knights) noexcept
    {
        if(!knights) return;
        while(knights)
        {
            const uint8_t knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & (empty_squares_ 
                                        | (is_white ?   (black_pieces_ & ~white_pieces_): 
                                                        (~black_pieces_ & white_pieces_)));
            while(possible_move)
            {
                const uint8_t attack_index = Magics::FindLS1B(possible_move);
                *(*move_list)++ = (is_white ?   Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,1):
                                                Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,0));
                possible_move = Magics::PopLS1B(possible_move);
            }
            knights = Magics::PopLS1B(knights);
        }
    }

    template<bool is_white>
    void QueenMoves(Move** move_list,BitBoard queens)
    {
        if(!queens) return;

        while(queens)
        {
            const uint8_t queen_index = Magics::FindLS1B(queens);
            
            move_info move = GetMovesForSliding<D::FILE>(queen_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move.encoded_move[i]);
            }

            move = GetMovesForSliding<D::RANK>(queen_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);;
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move.encoded_move[i]);
            }


            move = GetMovesForSliding<D::DIAG>(queen_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move.encoded_move[i]);
            }

            move = GetMovesForSliding<D::ADIAG>(queen_index,(is_white) ? white_pieces_ : black_pieces_,(is_white) ? black_pieces_ : white_pieces_);
            for(uint8_t i{0}; i < move.count;++i)
            {
                *(*move_list)++ = Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move.encoded_move[i]);
            }
            queens = Magics::PopLS1B(queens);
        }
    }

    void WhiteKingMoves(Move** move_list, BitBoard king) noexcept;

    void BlackKingMoves(Move** move_list, BitBoard king) noexcept;
    
public:    
    constexpr static std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
    inline static BitBoard EnPassantTargetSquare;
private:
    BitBoard white_pieces_;
    BitBoard black_pieces_;
    BitBoard empty_squares_;
};

#endif // #ifndef MOVEGEN_HPP