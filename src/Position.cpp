#include "Position.hpp"
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
        const Sq square = ((current_row * 8) + current_col);
        switch (i)
        {
        case('p'):
            AddPiece(p_BlackPawn, square);
            break;
        case('n'):
            AddPiece(p_BlackKnight, square);
            break;
        case('b'):
            AddPiece(p_BlackBishop, square);
            break;
        case('r'):
            AddPiece(p_BlackRook, square);
            break;
        case('q'):
            AddPiece(p_BlackQueen, square);
            break;
        case('k'):
            AddPiece(p_BlackKing, square);
            break;
        case('P'):
            AddPiece(p_WhitePawn, square);
            break;
        case('N'):
            AddPiece(p_WhiteKnight, square);
            break;
        case('B'):
            AddPiece(p_WhiteBishop, square);
            break;
        case('R'):
            AddPiece(p_WhiteRook, square);
            break;
        case('Q'):
            AddPiece(p_WhiteQueen, square);
            break;
        case('K'):
            AddPiece(p_WhiteKing, square);
            break;
        default:
            break;
        }
        ++current_col;
    }

    turn_ = (fen_sections _AT(1) _AT(0) == 'w') ? White : Black;

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

    if(fen_sections.at(4).empty())
    {
        info_.half_moves_ = 0;
    }
    else
    {
        std::from_chars(fen_sections.at(4).data(), fen_sections.at(4).data() + fen_sections.size(), info_.half_moves_);
    }
    if(fen_sections.at(5).empty())
    {
        full_moves_ = 0;
    }
    else
    {
        std::from_chars(fen_sections.at(5).data(), fen_sections.at(5).data() + fen_sections.size(), full_moves_);
    }
}
void Position::MakeMove(const Move m)
{
    assert(IsOk());
    previous_state_info.push_back(info_);

    Sq start_sq;
    Sq target_sq;
    MoveType mt;
    Moves::DecodeMove(m, &start_sq, &target_sq, &mt);

    PieceType p_type = Magics::TypeOf(PieceOn(start_sq));

    U8 is_castling_move = 0;
    const BitBoard start_bb    = Magics::SqToBB(start_sq);
    const BitBoard target_bb   = Magics::SqToBB(target_sq);
    const BitBoard them_pieces = Pieces(!turn_) | (p_type == pt_Pawn ?  EnPasBB() : 0);

    ++info_.half_moves_;

    if(Moves::IsPromotionMove(m)) p_type = pt_Pawn;

    if((target_bb & them_pieces))
    {
        Sq capture_sq = target_sq;
        if(target_sq == info_.en_passant_sq_) capture_sq -= (turn_ == White ? 8 : -8);

        info_.captured_type_ = PopPiece(capture_sq);
        assert(info_.captured_type_ != p_None);
        info_.zobrist_key_ ^= Zobrist::PIECES[!turn_][info_.captured_type_][capture_sq];
        info_.half_moves_ = 0;
    }
    else
        info_.captured_type_ = p_None;

    //handle weird castling rights update
    if(Magics::TypeOf(info_.captured_type_) == pt_Rook && (target_bb & (Magics::ROOK_START_SQS)))
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
    if(p_type == pt_Rook && (start_bb & Magics::ROOK_START_SQS))
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
    

    if(p_type == pt_King)
    {
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        info_.castling_rights_ &= turn_ == White ? Magics::NO_CASTLE_W : Magics::NO_CASTLE_B;
        info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
        
        switch (Magics::EncodeKing(start_sq, target_sq))
        {
        case Magics::EncodeKing(4, 2):   is_castling_move = 1; break;
        case Magics::EncodeKing(4, 6):   is_castling_move = 2; break;
        case Magics::EncodeKing(60, 58): is_castling_move = 3; break;
        case Magics::EncodeKing(60, 62): is_castling_move = 4; break;
        default: ;
        }
    }
    if(info_.en_passant_sq_ != Magics::EP_NULL)
    {
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        info_.en_passant_sq_ = Magics::EP_NULL;
    }

    if(is_castling_move)
    {
        //move relevant pieces
        // pieces_[whites_turn_][loc::KING] = target_bb;
        MovePiece(start_sq, target_sq);
        MovePiece(Magics::ROOK_TO_FROM_ARR[is_castling_move][0], Magics::ROOK_TO_FROM_ARR[is_castling_move][1]);
        // assert("Double check this", 0);
        //update the key
        info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[is_castling_move];
    }
    else
    {
        // //move piece
        // assert((pieces_[whites_turn_][p_type] & start_bb));
        // assert(!(pieces_[whites_turn_][p_type] & target_bb));

        // pieces_[whites_turn_][p_type] ^= start_bb | target_bb;
        MovePiece(start_sq, target_sq);
        info_.zobrist_key_ ^= Zobrist::PIECES[turn_][p_type][start_sq] ^ Zobrist::PIECES[turn_][p_type][target_sq];
    }
    
    if(p_type == pt_Pawn)
    {
        if(std::abs(start_sq - target_sq) == 16)
        {
            info_.en_passant_sq_ = target_sq - (turn_ == White ? 8 : -8);
            info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        }
        else if(Moves::IsPromotionMove(m))
        {
            const PieceType prom_to = Moves::PTypeOfProm(m);
            // assert("Need to update promotion system of moves since changed piece type", 0);
            // pieces_[whites_turn_][loc::PAWN] &= ~target_bb;
            // pieces_[whites_turn_][prom_to]   |= target_bb;
            RemovePiece(target_sq);
            AddPiece(MakePiece(turn_, prom_to),target_sq);

            info_.zobrist_key_ ^= Zobrist::PIECES[turn_][prom_to][target_sq] ^ Zobrist::PIECES[turn_][pt_Pawn][target_sq];
        }
        info_.half_moves_ = 0;
    }
    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;
    turn_ = !turn_;
    assert(IsOk());
}
void Position::UnmakeMove(const Move m)
{
    assert(IsOk());
    turn_ = !turn_;

    Sq start_sq;
    Sq target_sq;
    MoveType mt;
    Moves::DecodeMove(m, &start_sq, &target_sq, &mt);

    PieceType p_type = Magics::TypeOf(PieceOn(start_sq));
    U8 is_castling_move = 0;

    const BitBoard start_bb  = Magics::SqToBB(start_sq);
    const BitBoard target_bb = Magics::SqToBB(target_sq);

    if(Moves::IsPromotionMove(m))
    {
        p_type = pt_Pawn;
        RemovePiece(target_sq);
        AddPiece(MakePiece(turn_, pt_Pawn),target_sq);
    }
    //if castling move
    if(p_type == pt_King)
    {
        switch (Magics::EncodeKing(start_sq, target_sq))
        {
        case Magics::EncodeKing(4, 2):   is_castling_move = 1; break;
        case Magics::EncodeKing(4, 6):   is_castling_move = 2; break;
        case Magics::EncodeKing(60, 58): is_castling_move = 3; break;
        case Magics::EncodeKing(60, 62): is_castling_move = 4; break;
        }
    }
    
    if(is_castling_move) [[unlikely]]
    {
        // pieces_[whites_turn_][loc::KING] = start_bb;
        // pieces_[whites_turn_][loc::ROOK] ^= Magics::ROOK_TO_FROM_ARR_BB[is_castling_move];
        MovePiece(target_sq, start_sq);
        MovePiece(Magics::ROOK_TO_FROM_ARR[is_castling_move][1], Magics::ROOK_TO_FROM_ARR[is_castling_move][0]);
        // assert("Double check this", 0);
    }
    else
    {
        MovePiece(target_sq, start_sq);

        if(info_.captured_type_ != p_None)
        {
            Sq captured_sq = target_sq;
            if(Magics::TypeOf(info_.captured_type_) == pt_Pawn && target_sq == previous_state_info.back().en_passant_sq_)
            {
                captured_sq -= (turn_ == White ? 8 : -8);
            }
            // pieces_[!whites_turn_][info_.captured_type_] |= Magics::SqToBB(captured_sq);
            AddPiece(info_.captured_type_, captured_sq);
        }
    }
    //restore previous state
    info_ = previous_state_info.back();
    previous_state_info.pop_back();
    assert(IsOk());
}
ZobristKey Position::HashCurrentPostion()
{
    // assert(info_.zobrist_key_ == 0);
    info_.zobrist_key_ = 0;
    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;

    for(Colour c = White; c <= Black; c = Colour(c + 1))
    {
        for(PieceType pt = pt_begin_it; pt <= pt_end_it; pt = PieceType(pt + 1))
        {
            BitBoard piece_board = this->Pieces(c, pt);
            while(piece_board)
            {
                const Sq idx = Magics::FindLS1B(piece_board);
                info_.zobrist_key_ ^= Zobrist::PIECES[c][pt][idx];
                piece_board = Magics::PopLS1B(piece_board);
            }
        }
    }
    if(info_.en_passant_sq_ != Magics::EP_NULL)
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
    return info_.zobrist_key_;
}
