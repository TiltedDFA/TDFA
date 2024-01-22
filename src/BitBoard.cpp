#include "BitBoard.hpp"
#include <array>
#include <charconv>
#include <cmath>

void Position::ImportFen(std::string_view fen)
{
    Reset();
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
            info_.castling_rights_ |= Magics::CASTLE_K_W;
            break;
        case 'Q':
            info_.castling_rights_ |= Magics::CASTLE_Q_W;
            break;
        case 'k':
            info_.castling_rights_ |= Magics::CASTLE_K_B;
            break;
        case 'q':
            info_.castling_rights_ |= Magics::CASTLE_Q_B;
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
    assert(IsOk());
    previous_state_info.push_back(info_);
 
    Sq start_sq;
    Sq target_sq;
    PieceType p_type;
    Moves::DecodeMove(m, &start_sq, &target_sq, &p_type);

    U8 is_castling_move = 0;
    const BitBoard start_bb    = Magics::SqToBB(start_sq);
    const BitBoard target_bb   = Magics::SqToBB(target_sq);
    const BitBoard them_pieces = (whites_turn_ ? PiecesByColour<false>() : PiecesByColour<true>()) | (p_type == Pawn ?  EnPasBB() : 0);

    ++info_.half_moves_;

    if(Moves::IsPromotionMove(m)) p_type = Pawn;

    if((target_bb & them_pieces))
    {
        Sq capture_sq = target_sq;
        if(target_sq == info_.en_passant_sq_) capture_sq -= (whites_turn_ ? 8 : -8);

        info_.captured_type_ = (whites_turn_ ? RemovePiece<false>(Magics::SqToBB(capture_sq)) : RemovePiece<true>(Magics::SqToBB(capture_sq)));
        info_.zobrist_key_ ^= Zobrist::PIECES[!whites_turn_][info_.captured_type_][capture_sq];
        info_.half_moves_ = 0;
    }
    else
        info_.captured_type_ = NullPiece;

    //handle weird castling rights update
    if(info_.captured_type_ == Rook && (target_bb & (Magics::ROOK_START_SQS)))
    {

        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        switch (target_sq)
        {
        case 0:  info_.castling_rights_ &= (~Magics::CASTLE_Q_W & 0xF);break;
        case 7:  info_.castling_rights_ &= (~Magics::CASTLE_K_W & 0xF);break;
        case 56: info_.castling_rights_ &= (~Magics::CASTLE_Q_B & 0xF);break;
        case 63: info_.castling_rights_ &= (~Magics::CASTLE_K_B & 0xF);break;
        }
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
    }
    if(p_type == Rook && (start_bb & Magics::ROOK_START_SQS))
    {
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        switch (start_sq)
        {
        case 0:  info_.castling_rights_ &= (~Magics::CASTLE_Q_W & 0xF);break;
        case 7:  info_.castling_rights_ &= (~Magics::CASTLE_K_W & 0xF);break;
        case 56: info_.castling_rights_ &= (~Magics::CASTLE_Q_B & 0xF);break;
        case 63: info_.castling_rights_ &= (~Magics::CASTLE_K_B & 0xF);break;
        }
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
    }
    
    if(p_type == King)
    {
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        info_.castling_rights_ &= whites_turn_ ? Magics::NO_CASTLE_W : Magics::NO_CASTLE_B;
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        
        switch (Magics::EncodeKing(start_sq, target_sq))
        {
        case Magics::EncodeKing(4, 2):   is_castling_move = 1; break;
        case Magics::EncodeKing(4, 6):   is_castling_move = 2; break;
        case Magics::EncodeKing(60, 58): is_castling_move = 3; break;
        case Magics::EncodeKing(60, 62): is_castling_move = 4; break;
        }
    }
    if(info_.en_passant_sq_ != 255)
    {
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        info_.en_passant_sq_ = 255;
    }

    if(is_castling_move)
    {
        //move relevant pieces
        pieces_[whites_turn_][loc::KING] = target_bb;
        pieces_[whites_turn_][loc::ROOK] ^= Magics::ROOK_TO_FROM_ARR_BB[is_castling_move];
        //update the key
        info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[is_castling_move];
    }
    else
    {
        //move piece
        assert((pieces_[whites_turn_][p_type] & start_bb));
        assert(!(pieces_[whites_turn_][p_type] & target_bb));

        pieces_[whites_turn_][p_type] ^= start_bb | target_bb;
        info_.zobrist_key_ ^= Zobrist::PIECES[whites_turn_][p_type][start_sq] ^ Zobrist::PIECES[whites_turn_][p_type][target_sq];
    }
    
    if(p_type == Pawn)
    {
        if(std::abs(start_sq - target_sq) == 16)
        {
            info_.en_passant_sq_ = target_sq - (whites_turn_ ? 8 : -8);
            info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        }
        else if(Moves::IsPromotionMove(m))
        {
            const PieceType prom_to = Moves::PTypeOfProm(m);

            pieces_[whites_turn_][loc::PAWN] &= ~target_bb;
            pieces_[whites_turn_][prom_to]   |= target_bb;

            info_.zobrist_key_ ^= Zobrist::PIECES[whites_turn_][prom_to][target_sq] ^ Zobrist::PIECES[whites_turn_][Pawn][target_sq];
        }
        info_.half_moves_ = 0;
    }
    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;
    whites_turn_ = !whites_turn_;
    assert(IsOk());
}
void Position::UnmakeMove(Move m)
{
    assert(IsOk());
    whites_turn_ = !whites_turn_;

    Sq start_sq;
    Sq target_sq;
    PieceType p_type;
    Moves::DecodeMove(m, &start_sq, &target_sq, &p_type);

    U8 is_castling_move = 0;

    const BitBoard start_bb  = Magics::SqToBB(start_sq);
    const BitBoard target_bb = Magics::SqToBB(target_sq);

    if(Moves::IsPromotionMove(m))
    {
        p_type = Pawn;
        whites_turn_ ? RemovePiece<true>(target_bb) : RemovePiece<false>(target_bb);
        pieces_[whites_turn_][loc::PAWN] |= target_bb;
    }
    //if castling move
    if(p_type == King)
    {
        switch (Magics::EncodeKing(start_sq, target_sq))
        {
        case Magics::EncodeKing(4, 2):   is_castling_move = 1; break;
        case Magics::EncodeKing(4, 6):   is_castling_move = 2; break;
        case Magics::EncodeKing(60, 58): is_castling_move = 3; break;
        case Magics::EncodeKing(60, 62): is_castling_move = 4; break;
        }
    }
    
    if(is_castling_move)
    {
        pieces_[whites_turn_][loc::KING] = start_bb;
        pieces_[whites_turn_][loc::ROOK] ^= Magics::ROOK_TO_FROM_ARR_BB[is_castling_move];
    }
    else
    {
        pieces_[whites_turn_][p_type] ^= start_bb | target_bb;

        if(info_.captured_type_ != NullPiece)
        {
            Sq captured_sq = target_sq;
            if(info_.captured_type_ == Pawn && target_sq == previous_state_info.back().en_passant_sq_)
            {
                captured_sq -= (whites_turn_ ? 8 : -8);
            }
            pieces_[!whites_turn_][info_.captured_type_] |= Magics::SqToBB(captured_sq);
        }
    }
    //restore previous state
    info_ = previous_state_info.back();
    previous_state_info.pop_back();
    assert(IsOk());
}
void Position::HashCurrentPostion()
{
    info_.zobrist_key_ = 0;
    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;

    for(U8 clr = 0; clr < 2; ++clr)
    {
        for(U8 type = 0; type < 6; ++type)
        {
            BitBoard piece_board = this->Pieces(clr, type);
            while(piece_board)
            {
                const Sq idx = Magics::FindLS1B(piece_board);
                info_.zobrist_key_ ^= Zobrist::PIECES[clr][type][idx];
                piece_board = Magics::PopLS1B(piece_board);
            }
        }
    }
    if(info_.en_passant_sq_ != 0) 
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
}
