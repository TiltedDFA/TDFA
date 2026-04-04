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
        std::from_chars(fen_sections.at(4).data(), fen_sections.at(4).data() + fen_sections.at(4).size(), info_.half_moves_);
    }
    if(fen_sections.at(5).empty())
    {
        full_moves_ = 0;
    }
    else
    {
        std::from_chars(fen_sections.at(5).data(), fen_sections.at(5).data() + fen_sections.at(5).size(), full_moves_);
    }
}
void Position::MakeMove(const Move m)
{
    assert(IsOk());
    state_stack_[state_sp_++] = info_;

    Sq from, to;
    MoveType mt;
    Moves::DecodeMove(m, &from, &to, &mt);

    // Clear EP square
    if(info_.en_passant_sq_ != Magics::EP_NULL)
    {
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        info_.en_passant_sq_ = Magics::EP_NULL;
    }

    ++info_.half_moves_;
    info_.captured_type_ = p_None;

    const PieceType p_type = Magics::TypeOf(PieceOn(from));

    switch(mt)
    {
    case mt_Quiet:
    {
        MovePieceFast(from, to, p_type, turn_);
        info_.zobrist_key_ ^= Zobrist::PIECES[turn_][p_type][from] ^ Zobrist::PIECES[turn_][p_type][to];

        // Pawn double push — set EP
        if(p_type == pt_Pawn)
        {
            if((from ^ to) == 16) // equivalent to abs(from-to)==16 for unsigned
            {
                info_.en_passant_sq_ = Sq(turn_ == White ? from + 8 : from - 8);
                info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
            }
            info_.half_moves_ = 0;
        }
        break;
    }
    case mt_Capture:
    {
        const Piece cap_p = PieceOn(to);
        const PieceType cap_pt = Magics::TypeOf(cap_p);
        RemovePieceFast(to, cap_pt, !turn_);
        info_.captured_type_ = cap_p;
        info_.zobrist_key_ ^= Zobrist::PIECES[!turn_][cap_pt][to];
        info_.half_moves_ = 0;

        MovePieceFast(from, to, p_type, turn_);
        info_.zobrist_key_ ^= Zobrist::PIECES[turn_][p_type][from] ^ Zobrist::PIECES[turn_][p_type][to];
        break;
    }
    case mt_EnPassant:
    {
        const Sq cap_sq = Sq(turn_ == White ? to - 8 : to + 8);
        RemovePieceFast(cap_sq, pt_Pawn, !turn_);
        info_.captured_type_ = MakePiece(!turn_, pt_Pawn);
        info_.zobrist_key_ ^= Zobrist::PIECES[!turn_][pt_Pawn][cap_sq];
        info_.half_moves_ = 0;

        MovePieceFast(from, to, pt_Pawn, turn_);
        info_.zobrist_key_ ^= Zobrist::PIECES[turn_][pt_Pawn][from] ^ Zobrist::PIECES[turn_][pt_Pawn][to];
        break;
    }
    case mt_Castling:
    {
        U8 castle_idx = 0;
        switch (Magics::EncodeKing(from, to))
        {
        case Magics::EncodeKing(4, 2):   castle_idx = 1; break;
        case Magics::EncodeKing(4, 6):   castle_idx = 2; break;
        case Magics::EncodeKing(60, 58): castle_idx = 3; break;
        case Magics::EncodeKing(60, 62): castle_idx = 4; break;
        }
        MovePieceFast(from, to, pt_King, turn_);
        MovePieceFast(Magics::ROOK_TO_FROM_ARR[castle_idx][0], Magics::ROOK_TO_FROM_ARR[castle_idx][1], pt_Rook, turn_);
        info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[castle_idx];
        break;
    }
    default: // promotions
    {
        const PieceType prom_to = Moves::PTypeOfProm(m);

        // Handle capture-promotion
        const Piece cap_p = PieceOn(to);
        if(cap_p != p_None)
        {
            const PieceType cap_pt = Magics::TypeOf(cap_p);
            RemovePieceFast(to, cap_pt, !turn_);
            info_.captured_type_ = cap_p;
            info_.zobrist_key_ ^= Zobrist::PIECES[!turn_][cap_pt][to];
        }

        // Move pawn then replace with promoted piece
        MovePieceFast(from, to, pt_Pawn, turn_);
        RemovePieceFast(to, pt_Pawn, turn_);
        AddPieceFast(prom_to, turn_, to);
        info_.zobrist_key_ ^= Zobrist::PIECES[turn_][pt_Pawn][from] ^ Zobrist::PIECES[turn_][prom_to][to];
        info_.half_moves_ = 0;
        break;
    }
    }

    // Castling rights update — single table lookup replaces 3 switch blocks
    const U8 old_cr = info_.castling_rights_;
    info_.castling_rights_ &= Magics::CASTLING_MASK[from] & Magics::CASTLING_MASK[to];
    if(old_cr != info_.castling_rights_)
        info_.zobrist_key_ ^= Zobrist::CASTLING[old_cr] ^ Zobrist::CASTLING[info_.castling_rights_];

    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;
    turn_ = !turn_;
    if(turn_ == White)
        ++full_moves_;
    assert(IsOk());
}
void Position::UnmakeMove(const Move m)
{
    assert(IsOk());
    turn_ = !turn_;

    Sq from, to;
    MoveType mt;
    Moves::DecodeMove(m, &from, &to, &mt);

    switch(mt)
    {
    case mt_Quiet:
    {
        const PieceType p_type = Magics::TypeOf(PieceOn(to));
        MovePieceFast(to, from, p_type, turn_);
        break;
    }
    case mt_Capture:
    {
        const PieceType p_type = Magics::TypeOf(PieceOn(to));
        MovePieceFast(to, from, p_type, turn_);
        const PieceType cap_pt = Magics::TypeOf(info_.captured_type_);
        const Colour cap_c = Magics::ColourOf(info_.captured_type_);
        AddPieceFast(cap_pt, cap_c, to);
        break;
    }
    case mt_EnPassant:
    {
        MovePieceFast(to, from, pt_Pawn, turn_);
        const Sq cap_sq = Sq(turn_ == White ? to - 8 : to + 8);
        AddPieceFast(pt_Pawn, !turn_, cap_sq);
        break;
    }
    case mt_Castling:
    {
        U8 castle_idx = 0;
        switch (Magics::EncodeKing(from, to))
        {
        case Magics::EncodeKing(4, 2):   castle_idx = 1; break;
        case Magics::EncodeKing(4, 6):   castle_idx = 2; break;
        case Magics::EncodeKing(60, 58): castle_idx = 3; break;
        case Magics::EncodeKing(60, 62): castle_idx = 4; break;
        }
        MovePieceFast(to, from, pt_King, turn_);
        MovePieceFast(Magics::ROOK_TO_FROM_ARR[castle_idx][1], Magics::ROOK_TO_FROM_ARR[castle_idx][0], pt_Rook, turn_);
        break;
    }
    default: // promotions
    {
        const PieceType prom_pt = Magics::TypeOf(PieceOn(to));
        RemovePieceFast(to, prom_pt, turn_);
        AddPieceFast(pt_Pawn, turn_, from);

        // Restore captured piece if any
        if(info_.captured_type_ != p_None)
        {
            const PieceType cap_pt = Magics::TypeOf(info_.captured_type_);
            const Colour cap_c = Magics::ColourOf(info_.captured_type_);
            AddPieceFast(cap_pt, cap_c, to);
        }
        break;
    }
    }
    // Restore previous state
    info_ = state_stack_[--state_sp_];
    assert(IsOk());
}
ZobristKey Position::HashCurrentPostion()
{
    info_.zobrist_key_ = 0;
    if(turn_ == Black)
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
