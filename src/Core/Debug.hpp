#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <bitset>
#include <iostream>
#include <vector>

#include "Move.hpp"
#include "Types.hpp"
#include "BitBoard.hpp"
#include "MagicConstants.hpp"

#define PRINT(x) std::cout << (x) << std::endl

namespace Debug
{
    void PrintBB(BitBoard board, int board_center,bool mirrored=false)
    {
        std::string output{},current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8;++col)
            {
                if((col + row*8) == board_center)
                {
                    current_line = mirrored ?   current_line + "X " : "X " + current_line;
                }
                else if(((board >> (col + row*8)))&1ull)
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
        std::cout << output << std::endl;
    }
    void PrintBB(BitBoard board, bool mirrored=false)
    {
        std::string output{},current_line{};
        for(int row{0}; row < 8; ++row)
        {
            for(int col{0}; col < 8;++col)
            {
                if(((board >> (col + row*8)))&1ull)
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
        std::cout << output << std::endl;
    }
    void PrintEntireBoard(const BitBoard (&board)[2][6])
    {
        std::cout << "wk" << std::endl;
        PrintBB(board[BB::loc::WHITE][BB::loc::KING],true);
        std::cout << "wq" << std::endl;
        PrintBB(board[BB::loc::WHITE][BB::loc::QUEEN],true);
        std::cout << "wb" << std::endl;
        PrintBB(board[BB::loc::WHITE][BB::loc::BISHOP],true);
        std::cout << "wn" << std::endl;
        PrintBB(board[BB::loc::WHITE][BB::loc::KNIGHT],true);
        std::cout << "wr" << std::endl;
        PrintBB(board[BB::loc::WHITE][BB::loc::ROOK],true);
        std::cout << "wp" << std::endl;
        PrintBB(board[BB::loc::WHITE][BB::loc::PAWN],true);

        std::cout << "bk" << std::endl;
        PrintBB(board[BB::loc::BLACK][BB::loc::KING],true);
        std::cout << "bq" << std::endl;
        PrintBB(board[BB::loc::BLACK][BB::loc::QUEEN],true);
        std::cout << "bb" << std::endl;
        PrintBB(board[BB::loc::BLACK][BB::loc::BISHOP],true);
        std::cout << "bn" << std::endl;
        PrintBB(board[BB::loc::BLACK][BB::loc::KNIGHT],true);
        std::cout << "br" << std::endl;
        PrintBB(board[BB::loc::BLACK][BB::loc::ROOK],true);
        std::cout << "bp" << std::endl;
        PrintBB(board[BB::loc::BLACK][BB::loc::PAWN],true);
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
        move_str += "The start index is: " + std::to_string(move & Moves::START_SQ_MASK) + "\n";
        move_str += "The end index is: " + std::to_string((move & Moves::END_SQ_MASK) >> Moves::END_SQ_SHIFT) + "\n";
        move_str += "THe piece moved was: " + PieceTypeToStr((move & Moves::PIECE_TYPE_MASK) >> Moves::PIECE_TYPE_SHIFT) + "\n";
        move_str += (((move & Moves::COLOUR_MASK) >> Moves::COLOUR_SHIFT) == 1) ? "The piece was white" : "The piece was black";
        move_str += "\n";
        std::cout << move_str;
    }
    void ShortPrintEncodedMoveStr(Move move)
    {
        std::string move_str{""};
        move_str += "S: " + std::to_string(move & Moves::START_SQ_MASK) + ", ";
        move_str += "E: " + std::to_string((move & Moves::END_SQ_MASK) >> Moves::END_SQ_SHIFT) + ", ";
        move_str += "T: " + PieceTypeToStr((move & Moves::PIECE_TYPE_MASK) >> Moves::PIECE_TYPE_SHIFT) + ", ";
        move_str += "C: " + (((move & Moves::COLOUR_MASK) >> Moves::COLOUR_SHIFT) == 1) ? "W" : "B";
        move_str += "\n";
        std::cout << move_str;
    }
    void PrintUsThem(BitBoard us, BitBoard them, bool mirrored=false)
    {
        std::string output{},current_line{};
            for(int row{0}; row < 8; ++row)
            {
                for(int col{0}; col < 8;++col)
                {
                    const int current_index = (col + row*8);
                    if(((us >> current_index))&1ull)
                    {
                        current_line = mirrored ?   current_line + "UU " : "UU " + current_line;
                    }
                    else if (((them >> current_index))&1ull)
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
            std::cout << output << std::endl;
    }
    void PrintEncodedMoveBin(Move move)
    {
        std::cout << std::bitset<16>(move) << std::endl;
    }
    void PrintEncodedMovesVector(const std::vector<Move>& moves,bool mirrored=false)
    {
        BitBoard combined_board{0ull};
        for(const Move i : moves)
        {
            combined_board |= 1ull << Moves::GetTargetIndex(i);
        }
        PrintBB(combined_board,mirrored);
    }
    void PrintEncodedMovesMoveInfo(const move_info& move,bool mirrored=false)
    {
        if(move.count == 0) std::cout << "No moves found\n";
        BitBoard combined_board{0ull};
        for(uint8_t i = 0; i < move.count;++i)
        {
           // combined_board |= 1ull << Moves::GetStartIndex(move.encoded_move[i]);
            combined_board |= 1ull << Moves::GetTargetIndex(move.encoded_move[i]);
        }
        PrintBB(combined_board,mirrored);
    }
    void PrintU8BB(uint8_t bb, uint8_t board_center, bool mirrored=false)
    {
        std::string output{},current_line{};

        for(int col{0}; col < 8;++col)
        {
            if(col == board_center)
                current_line = mirrored ?   current_line + "X " : "X " + current_line;
            else if((bb >> col)&1ull)
                current_line = mirrored ?   current_line + "1 " : "1 " + current_line ;
            else
                current_line = mirrored ?   current_line + "0 " : "0 " + current_line;

        }
        output = current_line + output;
        current_line = "";
                            
        output += "----------------\n";
        output += mirrored ? "A B C D E F G H" : "H G F E D C B A";
        std::cout << output << std::endl;
    }
    constexpr BitBoard MoveToAtkBoard(MoveList& m)
    {
        BitBoard ret{0};
        for(Move* it = m.First(); it != *m.Current();++it)
            ret |= Magics::IndexToBB(Moves::GetTargetIndex(*it));
        return ret;
    }
}

#endif // #ifndef DEBUG_HPP