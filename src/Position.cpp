#include "Position.hpp"
#include <array>
#include <charconv>
#include <cmath>

// Lookup table: ASCII char → Piece (p_None for invalid chars)
static constexpr auto CHAR_TO_PIECE = []() consteval {
    std::array<Piece, 128> t{};
    for(auto& p : t) p = p_None;
    t['P'] = p_WhitePawn;   t['N'] = p_WhiteKnight; t['B'] = p_WhiteBishop;
    t['R'] = p_WhiteRook;   t['Q'] = p_WhiteQueen;  t['K'] = p_WhiteKing;
    t['p'] = p_BlackPawn;   t['n'] = p_BlackKnight;  t['b'] = p_BlackBishop;
    t['r'] = p_BlackRook;   t['q'] = p_BlackQueen;   t['k'] = p_BlackKing;
    return t;
}();

void Position::ImportFen(std::string_view fen)
{
    Reset();
    fen = RemoveWhiteSpace(fen);
    std::array<std::string_view,6> fen_sections;
    SplitFen(fen, fen_sections);

    U8 current_row = 7;
    U8 current_col = 0;
    for(const char c : fen_sections[0])
    {
        if(IsDigit(c))
        {
            current_col += c - '0';
            continue;
        }
        if(c == '/')
        {
            current_col = 0;
            --current_row;
            continue;
        }
        AddPiece(CHAR_TO_PIECE[U8(c)], (current_row * 8) + current_col);
        ++current_col;
    }

    turn_ = (fen_sections[1][0] == 'w') ? White : Black;

    // Castling — branchless OR with lookup
    static constexpr auto CASTLING_CHAR = []() consteval {
        std::array<U8, 128> t{};
        t['K'] = Magics::CASTLE_K_W;
        t['Q'] = Magics::CASTLE_Q_W;
        t['k'] = Magics::CASTLE_K_B;
        t['q'] = Magics::CASTLE_Q_B;
        return t;
    }();
    for(const char c : fen_sections[2])
        info_.castling_rights_ |= CASTLING_CHAR[U8(c)];

    // En passant
    if(fen_sections[3][0] != '-')
    {
        info_.en_passant_sq_ = Sq((fen_sections[3][0] - 'a') + (fen_sections[3][1] - '1') * 8);
    }

    // Half moves
    if(!fen_sections[4].empty())
        std::from_chars(fen_sections[4].data(), fen_sections[4].data() + fen_sections[4].size(), info_.half_moves_);

    // Full moves
    if(!fen_sections[5].empty())
        std::from_chars(fen_sections[5].data(), fen_sections[5].data() + fen_sections[5].size(), full_moves_);
}
void Position::MakeMove(const Move m)
{
    assert(IsOk());
    state_stack_[state_sp_++] = info_;

    const Sq from = Sq(m & 0x3F);
    const Sq to = Sq((m >> 6) & 0x3F);
    const MoveType mt = MoveType(m >> 12);

    // Clear EP square
    if(info_.en_passant_sq_ != Magics::EP_NULL)
    {
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        info_.en_passant_sq_ = Magics::EP_NULL;
    }

    ++info_.half_moves_;
    info_.captured_type_ = p_None;

    const PieceType p_type = Magics::TypeOf(PieceOn(from));
    info_.moved_type_ = p_type;

    switch(mt)
    {
    case mt_Quiet: [[likely]]
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

    const Sq from = Sq(m & 0x3F);
    const Sq to = Sq((m >> 6) & 0x3F);
    const MoveType mt = MoveType(m >> 12);

    switch(mt)
    {
    case mt_Quiet:
    {
        MovePieceFast(to, from, info_.moved_type_, turn_);
        break;
    }
    case mt_Capture:
    {
        MovePieceFast(to, from, info_.moved_type_, turn_);
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
        const PieceType prom_pt = Moves::PTypeOfProm(m);
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
