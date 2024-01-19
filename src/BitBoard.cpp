#include "BitBoard.hpp"
#include <array>
#include <charconv>

std::stack<Position> Position::previous_pos_info{};

void Position::ImportFen(std::string_view fen)
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
            break;
        }
        ++current_col;
    }

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
            break;
        }
    }

    if(fen_sections.at(3) != "-")
    {
        U8 en_passant_index = 0;
        en_passant_index += (fen_sections.at(3).at(0)) - 'a';
        en_passant_index += (fen_sections.at(3).at(1) - '1') * 8;
        info_.en_passant_sq_ = en_passant_index;
    }

    if(fen_sections.at(4) == "")
    {
        info_.half_moves_ = 0;
    }
    else
    {
        std::from_chars(fen_sections.at(4).data(), fen_sections.at(4).data() + fen_sections.size(), info_.half_moves_);
    }
    if(fen_sections.at(5) == "")
    {
        full_moves_ = 0;
    }
    else
    {
        std::from_chars(fen_sections.at(5).data(), fen_sections.at(5).data() + fen_sections.size(), full_moves_);
    }
}

void Position::MakeMove(Move m)
{
    previous_pos_info.push(*this);

    Sq start_sq;
    Sq target_sq;
    PieceType p_type;
    const bool is_white{whites_turn_};
    Moves::DecodeMove(m, &start_sq, &target_sq, &p_type);
    
    //switch turns
    whites_turn_ = !whites_turn_;
    postion_key_ ^= Zobrist::SIDE_TO_MOVE;

    const BitBoard target_bb{Magics::IndexToBB(target_sq)};
    
    full_moves_ += !is_white;

    if(Moves::IsPromotionMove(m)) //if not NOPROMO
    {
        //removing start_sq piece from boards and zobrist key
        pieces_[is_white][loc::PAWN] &= ~Magics::IndexToBB(start_sq);
        postion_key_ ^= Zobrist::PIECES[is_white][p_type][start_sq];

        if((is_white ? GetPieces<false>() : GetPieces<true>()) & target_bb)
        {
            if(target_bb & (Magics::IndexToBB<0>()  | Magics::IndexToBB<7>() | 
                            Magics::IndexToBB<56>() | Magics::IndexToBB<63>()))
            {
                //remove old caslting rights
                postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];

                switch(target_sq)
                {
                case 0:
                    info_.castling_rights_ &= 0x0B;
                    break;
                case 7: 
                    info_.castling_rights_ &= 0x07;
                    break;
                case 56:
                    info_.castling_rights_ &= 0x0E;
                    break;
                case 63:
                    info_.castling_rights_ &= 0x0D;
                    break;
                default:
                    break;
                }

                //add the new castling rights 
                postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            }
            if(is_white)
            {
                //removing attacked piece from boards and zobrist key
                const PieceType pt = RemoveIntersectingPiece<false>(target_bb);
                postion_key_ ^= Zobrist::PIECES[false][pt][target_sq];
            }
            else
            {
                //removing attacked piece from boards and zobrist key
                const PieceType pt = RemoveIntersectingPiece<true>(target_bb);
                postion_key_ ^= Zobrist::PIECES[true][pt][target_sq];
            }
        }
        //adding the new piece that was promoted to, to the boards and zobrist key
        const PieceType promoting_to = Moves::GetTypePromotingTo(m);
        pieces_[is_white][promoting_to] |= target_bb;
        postion_key_ ^= Zobrist::PIECES[is_white][promoting_to][target_sq];
        info_.half_moves_ = 0;
        return;
    }
    
    //removes the piece we take
    if((is_white ? GetPieces<false>() : GetPieces<true>()) & target_bb)
    {
        //if removed piece is a rook starting sq
        if(target_bb & (Magics::IndexToBB<0>()  | Magics::IndexToBB<7>() | 
                        Magics::IndexToBB<56>() | Magics::IndexToBB<63>()))
        {
            //remove old caslting rights
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            switch(target_sq)
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
            //add the new castling rights 
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        }
        //removing attacked piece from boards and zobrist key
        if(is_white)
        {
            const PieceType pt = RemoveIntersectingPiece<false>(target_bb);
            postion_key_ ^= Zobrist::PIECES[false][pt][target_sq];
        }
        else
        {
            const PieceType pt = RemoveIntersectingPiece<true>(target_bb);
            postion_key_ ^= Zobrist::PIECES[true][pt][target_sq];
        }

        info_.half_moves_ = 0;
    }
    else
    {
        ++info_.half_moves_;
    }

    //updates en passant state
    if(p_type == Moves::PAWN) 
    {
        //remove en pessant taken piece
        if(target_sq == info_.en_passant_sq_)
        {
            //removes old en passant target from zobrist key
            postion_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];

            //resets the en passant target
            info_.en_passant_sq_ = 0; 

            // move our pawn
            pieces_[is_white][p_type] ^= Magics::IndexToBB(start_sq) | target_bb; 
            postion_key_ ^= Zobrist::PIECES[is_white][loc::PAWN][start_sq];
            postion_key_ ^= Zobrist::PIECES[is_white][loc::PAWN][target_sq];

            // deletes enemy pawn
            if(is_white)
            {
                pieces_[false][loc::PAWN] ^= target_bb >> 8;
                // RemoveIntersectingPiece<false>(target_sq - 8);
                postion_key_ ^= Zobrist::PIECES[false][loc::PAWN][target_sq - 8];
            }
            else
            {
                pieces_[true][loc::PAWN] ^= target_bb << 8;
                // RemoveIntersectingPiece<true>(target_sq + 8);
                postion_key_ ^= Zobrist::PIECES[true][loc::PAWN][target_sq + 8];
            }
            return;
        }
        if (std::abs(target_sq - start_sq) == 16)
        {
            //removes old en passant target from zobrist key
            postion_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];

            info_.en_passant_sq_ = (is_white ? target_sq - 8 : target_sq + 8);

            //adds new en passant target sq to the zobrist key
            postion_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        }       
        else
        {
            //removes old en passant target from zobrist key
            postion_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
            info_.en_passant_sq_ = 0;
        }      
        info_.half_moves_ = 0;
    }
    else
    {
        //removes old en passant target from zobrist key
        postion_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        info_.en_passant_sq_ = 0;
    }

    if(p_type == Moves::KING)
    {
        //remove old castling rights from zobrist key
        postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];

        info_.castling_rights_ &= is_white ? 0x03 : 0x0C;

        //add new castling rights to zobrist key
        postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];

        switch(EncodeKing(start_sq, target_sq))
        {
            case EncodeKing(4, 2): //queen side
                pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<2>();
                pieces_[loc::WHITE][loc::ROOK] ^= Magics::IndexToBB<0>() | Magics::IndexToBB<3>();

                //remove king start pos from zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::KING][4];
                //add king end sq to zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::KING][2];
                //remove old rook from zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::ROOK][0];
                // add new rook to zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::ROOK][3];
                return;
            case EncodeKing(4, 6): //king side
                pieces_[loc::WHITE][loc::KING] = Magics::IndexToBB<6>();
                pieces_[loc::WHITE][loc::ROOK] ^= Magics::IndexToBB<7>() | Magics::IndexToBB<5>();

                //remove king start pos from zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::KING][4];
                //add king end sq to zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::KING][6];
                //remove old rook from zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::ROOK][5];
                // add new rook to zobrist key
                postion_key_ ^= Zobrist::PIECES[true][loc::ROOK][7];
                return;
            case EncodeKing(60, 58): //queen side
                pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<58>();
                pieces_[loc::BLACK][loc::ROOK] ^= Magics::IndexToBB<56>() | Magics::IndexToBB<59>();
                //remove king start pos from zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::KING][60];
                //add king end sq to zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::KING][58];
                //remove old rook from zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::ROOK][56];
                // add new rook to zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::ROOK][59];
                return;
            case EncodeKing(60, 62): //king side
                pieces_[loc::BLACK][loc::KING] = Magics::IndexToBB<62>();
                pieces_[loc::BLACK][loc::ROOK] &= ~Magics::IndexToBB<63>();
                pieces_[loc::BLACK][loc::ROOK] |= Magics::IndexToBB<61>();
                //remove king start pos from zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::KING][60];
                //add king end sq to zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::KING][58];
                //remove old rook from zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::ROOK][63];
                // add new rook to zobrist key
                postion_key_ ^= Zobrist::PIECES[false][loc::ROOK][61];
                return;
            default:
                break;
        }
    } 
    else if(p_type == Moves::ROOK)
    {
        switch(start_sq)
        {
        case 0:
            //remove old castling rights from zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];

            info_.castling_rights_ &= 0x0B;

            //add new castling rights to zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            break;
        case 7: 
            //remove old castling rights from zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];

            info_.castling_rights_ &= 0x07;

            //add new castling rights to zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            break;
        case 63:
            //remove old castling rights from zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            
            info_.castling_rights_ &= 0x0D;

            //add new castling rights to zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            break;
        case 56:
            //remove old castling rights from zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];

            info_.castling_rights_ &= 0x0E;

            //add new castling rights to zobrist key
            postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            break;
        default:
            break;
        }
    }
    pieces_[is_white][p_type] ^= Magics::IndexToBB(start_sq) | target_bb;
    postion_key_ ^= Zobrist::PIECES[is_white][p_type][start_sq];
    postion_key_ ^= Zobrist::PIECES[is_white][p_type][target_sq];
}

void Position::HashCurrentPostion()
{
    postion_key_ = 0;
    postion_key_ ^= Zobrist::SIDE_TO_MOVE;

    for(U8 clr = 0; clr < 2; ++clr)
    {
        for(U8 type = 0; type < 6; ++type)
        {
            BitBoard piece_board = this->GetSpecificPieces(clr, type);
            while(piece_board)
            {
                const Sq idx = Magics::FindLS1B(piece_board);
                postion_key_ ^= Zobrist::PIECES[clr][type][idx];
                piece_board = Magics::PopLS1B(piece_board);
            }
        }
    }
    if(info_.en_passant_sq_) 
        postion_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
    postion_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
}
