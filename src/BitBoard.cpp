#include "BitBoard.hpp"
#include <array>
#include <charconv>

std::stack<BB::Position> BB::Position::previous_pos_info{};

static constexpr std::string_view RemoveWhiteSpace(std::string_view str)
{
    int start_index{-1};
    int end_index{static_cast<int>(str.size())};
    while(str.at(++start_index) == ' '){}
    while(str.at(--end_index) == ' '){}
    return std::string_view(str.begin() + start_index, str.begin() + end_index + 1);
}
static inline void SplitFen(std::string_view fen, std::array<std::string_view,6>& fen_sections)
{
    int start = 0;
    int end = -1;
    U8 current_fen_section = 0;
    while(static_cast<std::size_t>(++end) < fen.size())
    {
        if(fen.at(end) == ' ')
        {
            ++end;
            fen_sections.at(current_fen_section) = std::string_view(fen.begin() + start, fen.begin() + (end - 1));
            ++current_fen_section;
            start = end;
            continue;
        }
    }
    fen_sections.at(current_fen_section) = std::string_view(fen.begin() + start, fen.begin() + (++end - 1));
    ++current_fen_section;
}
static constexpr bool IsDigit(const char i) {return i <= '9' && i >= '0';}
namespace BB
{
    bool Position::ImportFen(std::string_view fen)
    {
        ResetBoard();
        fen = RemoveWhiteSpace(fen);
        std::array<std::string_view,6> fen_sections;
        SplitFen(fen, fen_sections);

        U8 current_row = 7;
        U8 current_col = 0;
        for(const char i : fen_sections[0])
        {
            if(IsDigit(i))
            {
                current_col += i - '0';
                if(current_col > '7') return false; 
                continue;
            }
            if(i == '/')
            {
                current_col = 0;
                --current_row;
                continue;
            }
            switch (i)
            {
            case('p'):
                pieces_[loc::BLACK][loc::PAWN]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('n'):
                pieces_[loc::BLACK][loc::KNIGHT]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('b'):
                pieces_[loc::BLACK][loc::BISHOP]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('r'):
                pieces_[loc::BLACK][loc::ROOK]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('q'):
                pieces_[loc::BLACK][loc::QUEEN]   |= 1ull << ((current_row * 8) + current_col);
                break;
            case('k'):
                pieces_[loc::BLACK][loc::KING]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('P'):
                pieces_[loc::WHITE][loc::PAWN]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('N'):
                pieces_[loc::WHITE][loc::KNIGHT]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('B'):
                pieces_[loc::WHITE][loc::BISHOP]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('R'):
                pieces_[loc::WHITE][loc::ROOK]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('Q'):
                pieces_[loc::WHITE][loc::QUEEN]   |= 1ull << ((current_row * 8) + current_col);
                break;
            case('K'):
                pieces_[loc::WHITE][loc::KING]    |= 1ull << ((current_row * 8) + current_col);
                break;
            default:
                return false;
            }
            ++current_col;
        }

        if(fen_sections.at(1).at(0) != 'w' && fen_sections.at(1).at(0) != 'b') return false;
        whites_turn_ = fen_sections.at(1).at(0) == 'w';

        for(const char i : fen_sections.at(2))
        {
            switch (i)
            {
            case '-':                
                break;
            case 'K':
                info_.castling_rights_ |= 0x08;
                break;
            case 'Q':
                info_.castling_rights_ |= 0x04;
                break;
            case 'k':
                info_.castling_rights_ |= 0x02;
                break;
            case 'q':
                info_.castling_rights_ |= 0x01;
                break; 
            default:
                return false;
            }
        }

        if(fen_sections.at(3) != "-")
        {
            if(fen_sections.at(3).at(0) < 'a' || fen_sections.at(3).at(0) > 'h') return false;
            if(fen_sections.at(3).at(1) != '3' && fen_sections.at(3).at(1) != '6') return false;

            U8 en_passant_index = 0;
            en_passant_index += (fen_sections.at(3).at(0)) - 'a';
            en_passant_index += (fen_sections.at(3).at(1) - '1') * 8;
            info_.en_passant_target_sq_ = en_passant_index;
        }

        if(fen_sections.at(4) == "")
        {
            info_.half_moves_ = 0;
        }
        else
        {
            std::from_chars(fen_sections.at(4).data(), fen_sections.at(4).data() + fen_sections.size(), info_.half_moves_);
        }
        if(fen_sections.at(4) == "")
        {
            full_moves_ = 0;
        }
        else
        {
            std::from_chars(fen_sections.at(5).data(), fen_sections.at(5).data() + fen_sections.size(), full_moves_);
        }
        return true;
    }
    
    void Position::MakeMove(Move m)
    {
        previous_pos_info.push(*this);
        //switch turns
        whites_turn_ = !whites_turn_;

        Sq start;
        Sq target;
        PieceType p_type;
        const bool is_white{!whites_turn_};
        Moves::DecodeMove(m, start, target, p_type);
        
        const BitBoard target_bb{Magics::IndexToBB(target)};
        
        full_moves_ += !is_white;

        if(Moves::IsPromotionMove(m)) //if not NOPROMO
        {
            pieces_[is_white][loc::PAWN] &= ~Magics::IndexToBB(start);

            if((is_white ? GetPieces<false>() : GetPieces<true>()) & target_bb)
            {
                if(target_bb & (Magics::IndexToBB<0>()  | Magics::IndexToBB<7>() | 
                            Magics::IndexToBB<56>() | Magics::IndexToBB<63>()))
                {
                    switch(target)
                    {
                    case 0:
                        info_.castling_rights_ &= 0x0B;
                        break;
                    case 7: 
                        info_.castling_rights_ &= 0x07;
                        break;
                    case 63:
                        info_.castling_rights_ &= 0x0D;
                        break;
                    case 56:
                        info_.castling_rights_ &= 0x0E;
                        break;
                    default:
                        break;
                    }
                }
                is_white ? RemoveIntersectingPiece<false>(target_bb) 
                        : RemoveIntersectingPiece<true>(target_bb);
            }
            pieces_[is_white][Moves::GetTypePromotingTo(m)] |= target_bb;
            info_.half_moves_ = 0;
            return;
        }
        
        //removes the piece we take
        if((is_white ? GetPieces<false>() : GetPieces<true>()) & target_bb)
        {
            //if removed piece is a rook
            if(target_bb & (Magics::IndexToBB<0>()  | Magics::IndexToBB<7>() | 
                            Magics::IndexToBB<56>() | Magics::IndexToBB<63>()))
            {
                switch(target)
                {
                case 0:
                    info_.castling_rights_ &= 0x0B;
                    break;
                case 7: 
                    info_.castling_rights_ &= 0x07;
                    break;
                case 63:
                    info_.castling_rights_ &= 0x0D;
                    break;
                case 56:
                    info_.castling_rights_ &= 0x0E;
                    break;
                default:
                    break;
                }
            }
            info_.half_moves_ = 0;
            is_white ? RemoveIntersectingPiece<false>(target_bb) 
                        : RemoveIntersectingPiece<true>(target_bb);
        }
        else
            ++info_.half_moves_;

        //updates en passant state
        if(p_type == Moves::PAWN) 
        {
            //remove en pessant taken piece
            if(target == info_.en_passant_target_sq_)
            {
                info_.en_passant_target_sq_ = 0; 
                pieces_[is_white][p_type] ^= Magics::IndexToBB(start) | target_bb; // move our pawn
                if(is_white)
                {
                    RemoveIntersectingPiece<false>(Magics::IndexToBB(target-8));
                }
                else
                {
                    RemoveIntersectingPiece<true>(Magics::IndexToBB(target+8));
                }
                return;
            }
            else if (std::abs(target - start) == 16)
            {
                info_.en_passant_target_sq_ = (is_white ? target - 8 : target + 8);
            }       
            else
            {
                info_.en_passant_target_sq_ = 0;
            }      
            info_.half_moves_ = 0;
        }
        else
        {
            info_.en_passant_target_sq_ = 0;
        }

        if(p_type == Moves::KING)
        {
            info_.castling_rights_ &= is_white ? 0x03 : 0x0C;

            switch(start | (target << 6))
            {
                case 132: //queen
                    pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<2>();
                    pieces_[loc::WHITE][loc::ROOK] ^= Magics::IndexToBB<0>() | Magics::IndexToBB<3>();
                    return;
                case 388: //king
                    pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<6>();
                    pieces_[loc::WHITE][loc::ROOK] ^= Magics::IndexToBB<7>() | Magics::IndexToBB<5>();
                    return;
                case 3772: //queen
                    pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<58>();
                    pieces_[loc::BLACK][loc::ROOK] ^= Magics::IndexToBB<56>() | Magics::IndexToBB<59>();
                    return;
                case 4028: //king
                    pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<62>();
                    pieces_[loc::BLACK][loc::ROOK] &= ~Magics::IndexToBB<63>();
                    pieces_[loc::BLACK][loc::ROOK] |= Magics::IndexToBB<61>();
                    return;
                default:
                    break;
            }
        } 
        else if(p_type == Moves::ROOK)
        {
            switch(start)
            {
            case 0:
                info_.castling_rights_ &= 0x0B;
                break;
            case 7: 
                info_.castling_rights_ &= 0x07;
                break;
            case 63:
                info_.castling_rights_ &= 0x0D;
                break;
            case 56:
                info_.castling_rights_ &= 0x0E;
                break;
            default:
                break;
            }
        }
        pieces_[is_white][p_type] ^= Magics::IndexToBB(start) | target_bb;
    }

    void Position::HashCurrentPostion()
    {
        postion_key_ = 0;
        postion_key_ ^= Zobrist::WHITE_TO_MOVE * whites_turn_;

        for(U8 clr = 0; clr < 2; ++clr)
        {
            for(U8 type = 0; type < 6; ++type)
            {
                BitBoard piece_board = this->GetSpecificPieces(clr, type);
                while(piece_board)
                {
                    const Sq idx = Magics::FindLS1B(piece_board);
                    postion_key_ ^= Zobrist::PIECES_ARR[clr][type][idx];
                    piece_board = Magics::PopLS1B(piece_board);
                }
            }
        }
        postion_key_ ^= Zobrist::EN_PASSANT_ARR[this->GetEnPassantSq()];
        postion_key_ ^= Zobrist::CAST_ARR[this->GetRawCastling()];
    }
}