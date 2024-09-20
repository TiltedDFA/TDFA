#include "Debug.hpp"
#include <string>
namespace Debug
{
    void PrintBB(const BitBoard board, const int board_center, const bool mirrored)
    {
        std::string output{}, current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8;++col)
            {
                if((col + row * 8) == board_center)
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
    void PrintBB(const BitBoard board, const bool mirrored)
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
    void PrintBoardState(const Position& pos)
    {
        // pos.WhiteToMove() ?  Debug::PrintUsThemBlank(pos.PiecesByColour<true>(), pos.PiecesByColour<false>(), true) :
        //                     Debug::PrintUsThemBlank(pos.PiecesByColour<false>(), pos.PiecesByColour<true>(), true);
        {
            std::string prnt{"Castling rights: "};
            if(pos.CastlingRights() & 0x08) prnt += "Wk";
            if(pos.CastlingRights() & 0x04) prnt += "Wq";
            if(pos.CastlingRights() & 0x02) prnt += "Bk";
            if(pos.CastlingRights() & 0x01) prnt += "Bq";
            std::cout << (prnt);
        }

        std::cout << ("Half moves: " + std::to_string(pos.HalfMoves())) << '\n'; 
        std::cout << ("Passant trgt sq: " + std::to_string(pos.EnPasSq())) << '\n'; 

        pos.WhiteToMove() ? std::cout << ("w to move") : std::cout <<("b to move");
        std::cout << '\n';
        std::cout <<("Full moves: " + std::to_string(pos.FullMoves())) << '\n'; 
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
        case pt_King:
            return "King";
        case pt_Queen:
            return "Queen";
        case pt_Bishop:
            return "Bishop";
        case pt_Knight:
            return "Knight";
        case pt_Rook:
            return "Rook";
        case pt_Pawn:
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
        move_str += "\n";
        PRINTNL(move_str);
    }
    void PrintEncodedMoveBin(Move move)
    {
        PRINTNL(std::bitset<16>(move));
    }
    void Fun(BitBoard b)
    {
        for(int i = 0; i < 64; ++i)
        {
            if(b & (1ull << i))
            {
                // do stuff
            }
        }
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
        if(move.count_ == 0) std::cout << "No moves found\n";

        BitBoard combined_board{0ull};

        for(U8 i = 0; i < move.count_; ++i)
            combined_board |= 1ull << Moves::TargetSq(move.encoded_move_[i]);
        
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
    void PrintBoardGraphically(Position* pos)
    {
        constinit const static char PIECE_TYPES_MAPPING[12] = {'q', 'b', 'n', 'r', 'p', 'k', 'Q', 'B', 'N', 'R', 'P', 'K'};
        char squares[64];
        std::memset(squares, '-', sizeof(squares));
        for(int i = 0; i < 12; ++i)
        {
            BitBoard current_pieces = pos->Pieces(Colour(i > 5 ? White : Black),  PieceType(i % 6));
            const char current_type = PIECE_TYPES_MAPPING[i];
            while(current_pieces)
            {
                squares[Magics::PopNRetLS1B(current_pieces)] = current_type;
            }
        }
        std::string output;
        for(int i = 0; i < 8;++i)
        {
            output += std::string(squares + sizeof(char)*(8*(7-i)), 8) + std::format("|{}\n",char('H'-i));
        }
        output += "--------\n12345678";
        std::cout << output << std::endl;
    }
}

