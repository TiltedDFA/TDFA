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
            AddPiece(p_WhiteKnight, square);
            break;
        default:
            break;
        }
        ++current_col;
    }

    colour_to_move = Colour(fen_sections.at(1).at(0) != 'w');

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
void Position::MakeMove(Move m)
{
    assert(IsOk());
    previous_state_info.push_back(info_);

    const Sq from = Moves::StartSq(m);
    const Sq to = Moves::TargetSq(m);
    const MoveType type = Moves::GetMoveType(m);
    const PieceType type_moved = Magics::TypeOf(PieceOn(from));

    assert(type_moved != pt_All);
    assert(type_moved != pt_None);

    ++info_.half_moves_;
    info_.captured_type_ = pt_None;
    if(type_moved == pt_Pawn)
    {
        info_.half_moves_ = 0;
    }
    if(info_.en_passant_sq_ != Magics::EP_NULL)
    {
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
        info_.en_passant_sq_ = Magics::EP_NULL;
    }
    switch (type)
    {
        case mt_Quiet:
            if(type_moved == pt_Pawn && std::abs(from - to) == 16)
            {
                if (colour_to_move == White)
                {
                    info_.en_passant_sq_ = to - 8;
                }
                else
                {
                    info_.en_passant_sq_ = to + 8;
                }
                info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
            }
            else if (type_moved == pt_Rook && (Magics::SqToBB(from) & Magics::ROOK_START_SQS))
            {
                info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                switch (from)
                {
                    case 0:  info_.castling_rights_ &= Magics::CASTLE_NOT_Q_W;break;
                    case 7:  info_.castling_rights_ &= Magics::CASTLE_NOT_K_W;break;
                    case 56: info_.castling_rights_ &= Magics::CASTLE_NOT_Q_B;break;
                    case 63: info_.castling_rights_ &= Magics::CASTLE_NOT_K_B;break;
                }
                info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
            }
            break;
        case mt_EnPassant:
            RemovePiece(to  - (colour_to_move == White ? 8 : -8));
            info_.zobrist_key_ ^= Zobrist::PIECES[colour_to_move ^ Black][pt_Pawn][to  - (colour_to_move == White ? 8 : -8)];
            info_.captured_type_ = pt_Pawn;
            break;
        case mt_Capture:
            info_.half_moves_ = 0;
            info_.captured_type_ = Magics::TypeOf(PieceOn(to));
            RemovePiece(to);
            break;
        case mt_Castling:
            if (colour_to_move == White)
            {
                if (to == 6)
                {
                    MovePiece(7, 5);//move rook
                    info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[Magics::CASTLING_WHITE_KINGSIDE];
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                    info_.castling_rights_ &= Magics::CASTLE_NOT_K_W;
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                }
                else
                {
                    assert(to == 2);
                    MovePiece(0, 3);//move rook
                    info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[Magics::CASTLING_WHITE_QUEENSIDE];
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                    info_.castling_rights_ &= Magics::CASTLE_NOT_Q_W;
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                }
            }
            else
            {
                if (to == 58)
                {
                    //you're working on getting new board and move system integrated
                    MovePiece(56, 59);//move rook
                    info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[Magics::CASTLING_BLACK_QUEENSIDE];
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                    info_.castling_rights_ &= Magics::CASTLE_NOT_Q_B;
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                }
                else
                {
                    assert(to == 62);
                    MovePiece(63, 61);//move rook
                    info_.zobrist_key_ ^= Magics::CASTLING_ZOB_KEYS[Magics::CASTLING_BLACK_KINGSIDE];
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                    info_.castling_rights_ &= Magics::CASTLE_NOT_K_B;
                    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
                }
            }
            goto CASTLING_JMP;
        case mt_QueenPromotion:
            AddPiece(MakePiece(colour_to_move, pt_Queen),to);
            RemovePiece(from);
            info_.zobrist_key_ ^= Zobrist::PIECES[colour_to_move][pt_Queen][to] ^ Zobrist::PIECES[colour_to_move][pt_Pawn][from];
            goto PROMOTION_JMP;
        case mt_BishopPromotion:
            AddPiece(MakePiece(colour_to_move, pt_Bishop),to);
            RemovePiece(from);
            info_.zobrist_key_ ^= Zobrist::PIECES[colour_to_move][pt_Bishop][to] ^ Zobrist::PIECES[colour_to_move][pt_Pawn][from];
            goto PROMOTION_JMP;
        case mt_KnightPromotion:
            AddPiece(MakePiece(colour_to_move, pt_Knight),to);
            RemovePiece(from);
            info_.zobrist_key_ ^= Zobrist::PIECES[colour_to_move][pt_Knight][to] ^ Zobrist::PIECES[colour_to_move][pt_Pawn][from];
            goto PROMOTION_JMP;
        case mt_RookPromotion:
            AddPiece(MakePiece(colour_to_move, pt_Rook),to);
            RemovePiece(from);
            info_.zobrist_key_ ^= Zobrist::PIECES[colour_to_move][pt_Rook][to] ^ Zobrist::PIECES[colour_to_move][pt_Pawn][from];
            goto PROMOTION_JMP;
    }
    info_.zobrist_key_ ^= Zobrist::GetToFromPiece(colour_to_move, type_moved, to, from);
CASTLING_JMP:
    MovePiece(from, to);
PROMOTION_JMP:
    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;
    colour_to_move = Colour(colour_to_move ^ Black);
    assert(IsOk());
}
void Position::UnmakeMove(const Move m)
{
    assert(IsOk());
    colour_to_move = Colour(colour_to_move ^ Black);

    const Sq from = Moves::StartSq(m);
    const Sq to = Moves::TargetSq(m);
    const MoveType type = Moves::GetMoveType(m);
    const PieceType type_moved = Magics::TypeOf(PieceOn(from));


    U8 is_castling_move = 0;


    if(Moves::IsPromotion(m))
    {
        RemovePiece(to);
        AddPiece(MakePiece(colour_to_move, pt_Pawn), from);
    }
    //if castling move
    else if(type == mt_Castling)
    {
        switch (to)
        {
        case 2:
            MovePiece(3, 0);
            MovePiece(to, from);
            break;
        case 6:
            MovePiece(7, 5);
            MovePiece(to, from);
            break;
        case 58:
            MovePiece(59, 56);
            MovePiece(to, from);
            break;
        case 62:
            MovePiece(63, 61);
            MovePiece(to, from);
            break;
        }
    }
    else
    {
        MovePiece(to, from);

        if (info_.captured_type_ != pt_None)
        {
            Sq captured_sq = to;
            if (type == mt_EnPassant)
            {
                captured_sq -= (colour_to_move ? 8 : -8);
            }
            AddPiece(MakePiece(Colour(colour_to_move ^ Black), info_.captured_type_), captured_sq);
        }
    }
    //restore previous state
    info_ = previous_state_info.back();
    previous_state_info.pop_back();
    assert(IsOk());
}
void Position::HashCurrentPostion()
{
    assert(info_.zobrist_key_ == 0);
    info_.zobrist_key_ = 0;
    info_.zobrist_key_ ^= Zobrist::SIDE_TO_MOVE;

    for(U8 clr = 0; clr < 2; ++clr)
    {
        for(int type = pt_Queen; type <= pt_King; ++type)
        {
            BitBoard piece_board = Pieces(Colour(clr), PieceType(type));
            while(piece_board)
            {
                const Sq idx = Magics::FindLS1B(piece_board);
                info_.zobrist_key_ ^= Zobrist::PIECES[clr][type][idx];
                piece_board = Magics::PopLS1B(piece_board);
            }
        }
    }
    if(info_.en_passant_sq_ != Magics::EP_NULL)
        info_.zobrist_key_ ^= Zobrist::EN_PASSANT[info_.en_passant_sq_];
    info_.zobrist_key_ ^= Zobrist::CASTLING[info_.castling_rights_];
}
