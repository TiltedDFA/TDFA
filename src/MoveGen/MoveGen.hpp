#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <array>
#include <cassert>

#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"
#include <iostream>
using Magics::FileOf;

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
                if(us & them || (((~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) & ((~us) & Magics::BBRankOf(sq) || them & Magics::BBRankOf(sq)))) continue;

                move_info file_attack_moves{};
                move_info rank_attack_moves{};
                move_info diagonal_attack_moves{};
                move_info anti_diagonal_attack_moves{};

                const uint8_t rank_combined = (us | them) & ~Magics::BBFileOf(sq);
                uint8_t other_combined = (us | them) & ~Magics::BBRankOf(sq); 

                const uint8_t rankofsq = Magics::RankOf(sq);
                const uint8_t fileofsq = FileOf(sq);

                uint64_t diag_attacks = 0ull;
                uint64_t anti_diag_attacks = 0ull;
                if(us & Magics::BBFileOf(sq))
                {
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((rank_combined >> current_file) & 1)) //empty
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Moves::ROOK, 1));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq + (current_file - fileofsq), Moves::ROOK, 1));
                            break;
                        }
                    }
                    for(int8_t current_file = fileofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((rank_combined >> current_file) & 1)) 
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Moves::ROOK, 1));
                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            rank_attack_moves.add_move(Moves::EncodeMove(sq, sq - (fileofsq - current_file), Moves::ROOK, 1));
                            break;
                        }
                    }
                    uint16_t p1 = Magics::base_2_to_3[fileofsq][us & ~Magics::BBFileOf(sq)];
                    uint16_t p2 = 2 * Magics::base_2_to_3[fileofsq][them];
                    result.at(sq).at((uint8_t)D::RANK).at(p1 + p2) = rank_attack_moves;      
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
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Moves::ROOK, 1));
                            
                            if(Magics::IndexInBounds(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - rankofsq));
                            
                            if(Magics::IndexInBounds(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - rankofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq + 8 * (current_file - rankofsq), Moves::ROOK, 1));
                            
                            if(Magics::IndexInBounds(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::IndexToBB(sq + 9 * (current_file - rankofsq));
                                
                            if(Magics::IndexInBounds(sq +  7 * (current_file - rankofsq)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq +  7 * (current_file - rankofsq));
                            break;
                        }
                        
                    }
                    for(int8_t current_file = rankofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((other_combined >> current_file) & 1)) 
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Moves::ROOK, 1));
                            
                            if(Magics::IndexInBounds(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::IndexToBB(sq - 9 * (rankofsq - current_file));
                            
                            if(Magics::IndexInBounds(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (rankofsq - current_file));

                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            file_attack_moves.add_move(Moves::EncodeMove(sq, sq - 8 * (rankofsq - current_file), Moves::ROOK, 1));
                            if(Magics::IndexInBounds(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::IndexToBB(sq - 9 * (rankofsq - current_file));

                            if(Magics::IndexInBounds(sq -  7 * (rankofsq - current_file)))
                                anti_diag_attacks |=  Magics::IndexToBB(sq -  7 * (rankofsq - current_file));
                            break;
                        }
                    }

                    diag_attacks &= Magics::SLIDING_ATTACKS_MASK[sq][(int)D::DIAG];
                    anti_diag_attacks &= Magics::SLIDING_ATTACKS_MASK[sq][(int)D::ADIAG];
                    while(diag_attacks)
                    {
                        diagonal_attack_moves.add_move(Moves::EncodeMove(sq,Magics::FindLS1B(diag_attacks),Moves::BISHOP,1));
                        diag_attacks = Magics::PopLS1B(diag_attacks);
                    }
                    while(anti_diag_attacks)
                    {
                        anti_diagonal_attack_moves.add_move(Moves::EncodeMove(sq,Magics::FindLS1B(anti_diag_attacks),Moves::BISHOP,1));
                        anti_diag_attacks = Magics::PopLS1B(anti_diag_attacks);
                    }
                    uint16_t p1 = Magics::base_2_to_3[rankofsq][us & ~Magics::BBRankOf(sq)];
                    uint16_t p2 = 2 * Magics::base_2_to_3[rankofsq][them];

                    result.at(sq).at((uint8_t)D::FILE).at(p1 + p2) = file_attack_moves;
                    result.at(sq).at((uint8_t)D::DIAG).at(p1 + p2) = diagonal_attack_moves;
                    result.at(sq).at((uint8_t)D::ADIAG).at(p1 + p2) = anti_diagonal_attack_moves;
                }
            }
        }
    }
    return result;
}

class MoveGen
{
public:
    MoveGen();
    
    void GenerateAllMoves(const BB::Position&, MoveList& ml);
    
    BitBoard GenerateAllWhiteMoves(const BB::Position&, MoveList& ml);

    BitBoard GenerateAllBlackMoves(const BB::Position&, MoveList& ml);

    template<bool is_white>
    void GenerateLegalMoves(const BB::Position& pos, MoveList& ml)
    {
        /*
        
        -Gen pseudo legal
            -ez
            
        -Gen next bitboard after each move, regen attacks bitboard, see if legal
            -possibly store attacks bitboard? for less overhead?

        */
        pos_ = pos;
        MoveList pseudo_legal_ml;
        //Pseudo legal:
        KingMoves<is_white>(pseudo_legal_ml);
        QueenMoves<is_white>(pseudo_legal_ml);
        BishopMoves<is_white>(pseudo_legal_ml);
        KnightMoves<is_white>(pseudo_legal_ml);
        RookMoves<is_white>(pseudo_legal_ml);
        (is_white ? WhitePawnMoves(pseudo_legal_ml) : BlackPawnMoves(pseudo_legal_ml));
        Castling<is_white>(pseudo_legal_ml);

        //regen attacks   
        BitBoard king = pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::KING>();
        uint8_t king_square = Magics::FindLS1B(king);
        bool bishopFlag = false;
        bool rook_flag = false;
        bool queen_flag = false;

        BitBoard rookFindingRayCast = Magics::SLIDING_ATTACKS_MASK[king_square][0] | Magics::SLIDING_ATTACKS_MASK[king_square][1];
        BitBoard bishopFindingRayCast = Magics::SLIDING_ATTACKS_MASK[king_square][2] | Magics::SLIDING_ATTACKS_MASK[king_square][3];
        if(rookFindingRayCast & pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::ROOK>()){rook_flag = true;}
        if(bishopFindingRayCast & pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::BISHOP>()){bishopFlag = true;}
        if((rookFindingRayCast | bishopFindingRayCast) & pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::QUEEN>()){queen_flag = true;}
        
        for(uint8_t i = 0; i < pseudo_legal_ml.len(); ++i)
        {
            pos_.MakeMove(pseudo_legal_ml[i], PromType::NOPROMO);
            BitBoard attackBB = 0; 

            if(bishopFlag)
            {
                attackBB |= BishopAttacks<is_white ? false : true>();
            }
            if(rook_flag)
            {
                attackBB |= RookAttacks<is_white ? false : true>();
            }
            if(queen_flag)
            {
                attackBB |= QueenAttacks<is_white ? false : true>();
            }

            if(!(pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::KING>() & attackBB))
            { 
                ml.add(pseudo_legal_ml[i]);
            }
            pos_.UnmakeMove(pseudo_legal_ml[i], PromType::NOPROMO);
        }

    }

    template<bool is_white> 
    constexpr bool InCheck()
    {
        return pos_.GetSpecificPieces<is_white ? loc::WHITE : loc::BLACK, loc::KING>() & (is_white ? pos_.b_atks_ : pos_.w_atks_);
    }
    
  
    
    template<D direction>
    static constexpr move_info& GetMovesForSliding(uint8_t piece_sq, BitBoard us, BitBoard them) noexcept
    {
        return const_cast<move_info&>(SLIDING_ATTACK_CONFIG[piece_sq][static_cast<int>(direction)]
            [
                //us
                (Magics::base_2_to_3
                [(direction == D::RANK) ? Magics::FileOf(piece_sq) 
                : Magics::RankOf(piece_sq)]
                [(direction == D::RANK) ? Magics::CollapsedFilesIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])
                : Magics::CollapsedRanksIndex(us & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])])
                +
                //them
                (2 * Magics::base_2_to_3
                [(direction == D::RANK) ? Magics::FileOf(piece_sq) 
                : Magics::RankOf(piece_sq)]
                [(direction == D::RANK) ? Magics::CollapsedFilesIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])
                : Magics::CollapsedRanksIndex(them & Magics::SLIDING_ATTACKS_MASK[piece_sq][static_cast<int>(direction)])])
            ]);
    }
private:

    void WhitePawnMoves(MoveList& ml) noexcept;

    void BlackPawnMoves(MoveList& ml) noexcept;

    template<bool is_white>
    constexpr void BishopMoves(MoveList& ml)
    {
        BitBoard bishops = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::BISHOP>();
        if(!bishops) return;

        while(bishops)
        {
            const uint8_t bishop_index = Magics::FindLS1B(bishops);

            const move_info& move = GetMovesForSliding<D::DIAG>(bishop_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count; ++i)
            {
                ml.add(Moves::SetPieceTypeAndColour<is_white, Moves::BISHOP>(move.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            }
            
            const move_info& move1 = GetMovesForSliding<D::ADIAG>(bishop_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count; ++i)
            {
                ml.add(Moves::SetPieceTypeAndColour<is_white, Moves::BISHOP>(move1.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            }
            
            bishops = Magics::PopLS1B(bishops);
        }
    }

    template<bool is_white>
    constexpr BitBoard BishopAttacks()
    {
        BitBoard bishops = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::BISHOP>();
        BitBoard attacks{0};
        if(!bishops) return attacks;

        while(bishops)
        {
            const uint8_t bishop_index = Magics::FindLS1B(bishops);
            const move_info& move = GetMovesForSliding<D::DIAG>(bishop_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            
            const move_info& move1 = GetMovesForSliding<D::ADIAG>(bishop_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            
            bishops = Magics::PopLS1B(bishops);
        }
        return attacks;
    }
    template<bool is_white>
    constexpr BitBoard RookAttacks()
    {
        BitBoard rooks = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::ROOK>();
        BitBoard attacks{0};
        if(!rooks) return attacks;

        while(rooks)
        {
            const uint8_t rook_index = Magics::FindLS1B(rooks);

            const move_info& move = GetMovesForSliding<D::FILE>(rook_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            
            const move_info& move1 = GetMovesForSliding<D::RANK>(rook_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            
            rooks = Magics::PopLS1B(rooks);
        }
        return attacks;
    }
    template<bool is_white>
    constexpr BitBoard QueenAttacks()
    {
        BitBoard queens = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::QUEEN>();
        BitBoard attacks{0};
        if(!queens) return attacks;

        while(queens)
        {
            const uint8_t queen_index = Magics::FindLS1B(queens);
            
            const move_info& move = GetMovesForSliding<D::FILE>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));

            const move_info& move1 = GetMovesForSliding<D::RANK>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            

            const move_info& move2 = GetMovesForSliding<D::DIAG>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move2.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move2.encoded_move[i]));
            

            const move_info& move3 = GetMovesForSliding<D::ADIAG>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move3.count; ++i)
                attacks |= Magics::IndexToBB(Moves::GetTargetIndex(move3.encoded_move[i]));
            
            queens = Magics::PopLS1B(queens);
        }
        return attacks;
    }
    template<bool is_white>
    constexpr void RookMoves(MoveList& ml)
    {
        BitBoard rooks = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::ROOK>();
        if(!rooks) return;

        while(rooks)
        {
            const uint8_t rook_index = Magics::FindLS1B(rooks);

            const move_info& move = GetMovesForSliding<D::FILE>(rook_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count; ++i)
            {
                ml.add(Moves::SetColour<is_white>(move.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            }

            const move_info& move1 = GetMovesForSliding<D::RANK>(rook_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count; ++i)
            {
                ml.add(Moves::SetColour<is_white>(move1.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            }
            rooks = Magics::PopLS1B(rooks);
        }
    }

    template<bool is_white>
    constexpr void KnightMoves(MoveList& ml) noexcept
    {
        BitBoard knights = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::KNIGHT>();
        if(!knights) return;
        while(knights)
        {
            const uint8_t knight_index = Magics::FindLS1B(knights);
            BitBoard possible_move = Magics::KNIGHT_ATTACK_MASKS[knight_index] & (pos_.GetEmptySquares() | (is_white ? pos_.GetPieces<false>() : pos_.GetPieces<true>()));
            while(possible_move)
            {
                const uint8_t attack_index = Magics::FindLS1B(possible_move);
                ml.add((is_white ?   Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,1)
                                    :Moves::EncodeMove(knight_index,attack_index,Moves::KNIGHT,0)));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(attack_index);
                possible_move = Magics::PopLS1B(possible_move);
            }
            knights = Magics::PopLS1B(knights);
        }
    }

    template<bool is_white>
    constexpr void QueenMoves(MoveList& ml)
    {
        BitBoard queens = pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::QUEEN>();
        if(!queens) return;

        while(queens)
        {
            const uint8_t queen_index = Magics::FindLS1B(queens);
            
            const move_info& move = GetMovesForSliding<D::FILE>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move.count; ++i)
            {
                ml.add(Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move.encoded_move[i]));
            }

            const move_info& move1 = GetMovesForSliding<D::RANK>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move1.count; ++i)
            {
                ml.add(Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move1.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move1.encoded_move[i]));
            }

            const move_info& move2 = GetMovesForSliding<D::DIAG>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move2.count; ++i)
            {
                ml.add(Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move2.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move2.encoded_move[i]));
            }

            const move_info& move3 = GetMovesForSliding<D::ADIAG>(queen_index, (is_white) ? pos_.GetPieces<true>() : pos_.GetPieces<false>(), (is_white) ? pos_.GetPieces<false>() : pos_.GetPieces<true>());
            for(uint8_t i{0}; i < move3.count; ++i)
            {
                ml.add(Moves::SetPieceTypeAndColour<is_white, Moves::QUEEN>(move3.encoded_move[i]));
                (is_white ? pos_.w_atks_ : pos_.b_atks_) |= Magics::IndexToBB(Moves::GetTargetIndex(move3.encoded_move[i]));
            }
            queens = Magics::PopLS1B(queens);
        }
    }

    template<bool is_white>
    void KingMoves(MoveList& ml) noexcept
    {
        const uint8_t king_index = Magics::FindLS1B(pos_.GetSpecificPieces<(is_white ? loc::WHITE : loc::BLACK), loc::KING>());
        BitBoard king_attacks = Magics::KING_ATTACK_MASKS[king_index] & (pos_.GetEmptySquares() | (is_white ? pos_.GetPieces<false>() : pos_.GetPieces<true>()));
        while(king_attacks)
        {
            ml.add(Moves::EncodeMove(king_index, Magics::FindLS1B(king_attacks), Moves::KING, (is_white ? 1 : 0)));
            (is_white ? pos_.w_atks_ : pos_.b_atks_)  |= Magics::IndexToBB(Magics::FindLS1B(king_attacks));
            king_attacks = Magics::PopLS1B(king_attacks);
        }
    }

    template<bool is_white>
    constexpr void Castling(MoveList& ml) noexcept
    {   
        if(!((is_white ? 0x0C : 0x03) & pos_.info_.castling_rights_)) {return;} //checks for castling rights
        if(InCheck<is_white>()) {return;}

        BitBoard king_attacks = 0;
        const BitBoard whole_board = pos_.GetPieces<true>() | pos_.GetPieces<false>();
        const uint8_t king_index = (is_white ? 4 : 60);
        const uint8_t rank_looked_at = (is_white ? (whole_board & 0xFF) : whole_board >> 56);

        if // kingside
        (
            (pos_.info_.castling_rights_ & (is_white ? 0x08 : 0x02)) // has rights
            &&
            !(rank_looked_at & 0x60) // not blocked
            &&
            !(0xFF & (is_white ? pos_.b_atks_ : pos_.w_atks_ >> 56) & 0x60) // not under attack by enemy
        )
        {
            king_attacks |= (is_white ? Magics::IndexToBB<6>() : Magics::IndexToBB<62>());
        }
        if //queenside
        (
            (pos_.info_.castling_rights_ & (is_white ? 0x04 : 0x01)) // has rights
            && 
            !(rank_looked_at & 0x0E) // not blocked
            && 
            !(0xFF & (is_white ? pos_.b_atks_ : pos_.w_atks_ >> 56) & 0x0C) // not under attack by enemy
        )
        {
            king_attacks |= (is_white ? Magics::IndexToBB<2>() : Magics::IndexToBB<58>());
        }

        while(king_attacks)
        {
            ml.add(Moves::EncodeMove(king_index, Magics::FindLS1B(king_attacks), Moves::KING, (is_white ? 1 : 0)));
            (is_white ? pos_.w_atks_ : pos_.b_atks_)  |= Magics::IndexToBB(Magics::FindLS1B(king_attacks));
            king_attacks = Magics::PopLS1B(king_attacks);
        }
    }
    
public:    
    constexpr static std::array<std::array<std::array<move_info,2187>,4>,64> SLIDING_ATTACK_CONFIG = PrecomputeTitboards();
private:
    BB::Position pos_;
};

#endif // #ifndef MOVEGEN_HPP