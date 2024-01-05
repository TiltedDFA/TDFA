#include "Debug.hpp"
namespace Debug
{
    void PrintBB(BitBoard board, int board_center, bool mirrored)
    {
        std::string output{}, current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8;++col)
            {
                if((col + row*8) == board_center)
                {
                    current_line = mirrored ?   current_line + "X " : "X " + current_line;
                }
                else if(((board >> (col + row * 8))) & 1ull)
                {
                    current_line = mirrored ?   current_line + "1 " : "1 " + current_line ;
                }
                else
                {
                    current_line = mirrored ?   current_line + "0 " : "0 " + current_line;
                }
            }
            current_line += "|" + std::to_string(row + 1) + " \n";
            output = current_line + output;
            current_line = "";
        }                    
        output += "----------------\n";
        output += mirrored ? "A B C D E F G H" : "H G F E D C B A";
        PRINTNL(output);
    }
    void PrintBB(BitBoard board, bool mirrored)
    {
        std::string output{}, current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8; ++col)
            {
                if(((board >> (col + row * 8))) & 1ull)
                {
                    current_line = mirrored ?   current_line + "1 " : "1 " + current_line;
                }
                else
                {
                    current_line = mirrored ?   current_line + "0 " : "0 " + current_line;
                }
            }
            current_line += "|" + std::to_string(row + 1) + " \n";
            output = current_line + output;
            current_line = "";
        }                    
        output += "----------------\n";
        output += mirrored ? "A B C D E F G H" : "H G F E D C B A";
        PRINTNL(output);
    }
    void PrintUsThemBlank(BitBoard us, BitBoard them, bool mirrored)
    {
        std::string output{},current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8; ++col)
            {
                const int current_index = (col + row * 8);
                if(((us >> current_index)) & 1ull)
                {
                    current_line = mirrored ?   current_line + "UU " : "UU " + current_line;
                }
                else if (((them >> current_index)) & 1ull)
                {
                    current_line = mirrored ?   current_line + "TT " : "TT " + current_line;
                }
                else
                {
                    current_line = mirrored ?  (current_line + std::string("00") + std::string(" ")) : ("00" + std::string(" ")  + current_line);
                }
            }
            current_line += "|" + std::to_string(row + 1) + " \n";
            output = current_line + output;
            current_line = "";
        }                    
        output += "------------------------\n";
        output += mirrored ? "A  B  C  D  E  F  G  H" : "H  G  F  E  D  C  B  A";
        PRINTNL(output);
    }
    void PrintBoardState(const BB::Position& pos)
    {
        pos.whites_turn_ ?  Debug::PrintUsThemBlank(pos.GetPieces<true>(), pos.GetPieces<false>(), true) :
                            Debug::PrintUsThemBlank(pos.GetPieces<false>(), pos.GetPieces<true>(), true);
        {
            std::string prnt{"Castiling rights: "};
            if(pos.info_.castling_rights_ & 0x08) prnt += "Wk";
            if(pos.info_.castling_rights_ & 0x04) prnt += "Wq";
            if(pos.info_.castling_rights_ & 0x02) prnt += "Bk";
            if(pos.info_.castling_rights_ & 0x01) prnt += "Bq";
            PRINTNL(prnt);
        }

        PRINTNL("Half moves: " + std::to_string(pos.info_.half_moves_)); 
        PRINTNL("Passant trgt sq: " + std::to_string(pos.info_.en_passant_target_sq_)); 
#if DEVELOPER_MODE == 1
        pos.whites_turn_ ? PRINTNL("IsWhite'sTurn") : PRINTNL("IsBlack'sTurn");
#endif
        PRINTNL("Full moves: " + std::to_string(pos.full_moves_)); 
    }
    void PrintInduvidualPieces(const BitBoard (&board)[2][6])
    {
        PRINTNL("wk");
        PrintBB(board[loc::WHITE][loc::KING],true);
        PRINTNL("wq");
        PrintBB(board[loc::WHITE][loc::QUEEN],true);
        PRINTNL("wb");
        PrintBB(board[loc::WHITE][loc::BISHOP],true);
        PRINTNL("wn");
        PrintBB(board[loc::WHITE][loc::KNIGHT],true);
        PRINTNL("wr");
        PrintBB(board[loc::WHITE][loc::ROOK],true);
        PRINTNL("wp");
        PrintBB(board[loc::WHITE][loc::PAWN],true);

        PRINTNL("bk");
        PrintBB(board[loc::BLACK][loc::KING],true);
        PRINTNL("bq");
        PrintBB(board[loc::BLACK][loc::QUEEN],true);
        PRINTNL("bb");
        PrintBB(board[loc::BLACK][loc::BISHOP],true);
        PRINTNL("bn");
        PrintBB(board[loc::BLACK][loc::KNIGHT],true);
        PRINTNL("br");
        PrintBB(board[loc::BLACK][loc::ROOK],true);
        PRINTNL("bp");
        PrintBB(board[loc::BLACK][loc::PAWN],true);
    }
    std::string PieceTypeToStr(PieceType piece)
    {
        switch (piece)
        {
        case Moves::KING:
            return "King";
        case Moves::QUEEN:
            return "Queen";
        case Moves::BISHOP:
            return "Bishop";
        case Moves::KNIGHT:
            return "Knight";
        case Moves::ROOK:
            return "Rook";
        case Moves::PAWN:
            return "Pawn";
        default:
            return "Error with piece type to string";
        }
    }
    void PrintEncodedMoveStr(Move move)
    {
        std::string move_str{""};
        const std::string comma (", ");
        move_str += std::string("S: ") + std::to_string(move & Moves::START_SQ_MASK) + comma;
        move_str += std::string("E: ") + std::to_string((move & Moves::END_SQ_MASK) >> Moves::END_SQ_SHIFT) + comma;
        move_str += std::string("T: ") + PieceTypeToStr(Moves::GetPieceType(move)) + comma;
        move_str += "\n";
        PRINT(move_str);
    }
    void PrintEncodedMoveBin(Move move)
    {
        PRINTNL(std::bitset<16>(move));
    }
    void PrintUsThem(BitBoard us, BitBoard them, bool mirrored)
    {
        std::string output{}, current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8; ++col)
            {
                const int current_index = (col + row * 8);
                if(((us >> current_index)) & 1ull)
                {
                    current_line = mirrored ?   current_line + "UU " : "UU " + current_line;
                }
                else if (((them >> current_index)) & 1ull)
                {
                    current_line = mirrored ?   current_line + "TT " : "TT " + current_line;
                }
                else
                {
                    current_line = mirrored ?   current_line + ((current_index < 10) ? ("0" + std::to_string(current_index) + " ") : (std::to_string(current_index) + " ")) : ((current_index < 10) ? ("0" + std::to_string(current_index) + " ") : (std::to_string(current_index) + " ")) + current_line;
                }
            }
            current_line += "|" + std::to_string(row + 1) + " \n";
            output = current_line + output;
            current_line = "";
        }                    
        output += "------------------------\n";
        output += mirrored ? "A  B  C  D  E  F  G  H" : "H  G  F  E  D  C  B  A";
        PRINTNL(output);
    }
    void PrintEncodedMovesMoveInfo(const move_info& move, bool mirrored)
    {
        if(move.count == 0) std::cout << "No moves found\n";

        BitBoard combined_board{0ull};

        for(U8 i = 0; i < move.count; ++i)
            combined_board |= 1ull << Moves::GetTargetIndex(move.encoded_move[i]);
        
        PrintBB(combined_board, mirrored);
    }
    void PrintU8BB(U8 bb, U8 board_center, bool mirrored)
    {
        std::string output{},current_line{};

        for(int col{0}; col < 8; ++col)
        {
            if(col == board_center)
                current_line = mirrored ?   current_line + "X " : "X " + current_line;
            else if((bb >> col) & 1ull)
                current_line = mirrored ?   current_line + "1 " : "1 " + current_line ;
            else
                current_line = mirrored ?   current_line + "0 " : "0 " + current_line;
        }
        output = current_line + output;
        current_line = "";
        output += "----------------\n";
        output += mirrored ? "A B C D E F G H" : "H G F E D C B A";
        PRINTNL(output);
    }
}

