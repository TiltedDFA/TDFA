#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <bitset>
#include <iostream>
#include <vector>
#include <format>

#include "Move.hpp"
#include "Types.hpp"
#include "BitBoard.hpp"
#include "MagicConstants.hpp"

#if DEVELOPER_MODE == 1
#define PRINT(x) std::cout << (x)
#define PRINTNL(x) std::cout << (x) << std::endl
#else
#define PRINT(x)
#define PRINTNL(x)
#endif

namespace Debug
{
    //Prints a bitboard
    void PrintBB(BitBoard board, bool mirrored = false);
    
    //Prints a bitboard with an 'x' on a specified sq
    void PrintBB(BitBoard board, int board_center, bool mirrored = false);

    //Prints all of the information about a Position
    void PrintBoardState(const Position& pos);
    
    //Prints all of the piece bitboards contained within the Position.pieces_ array
    void PrintInduvidualPieces(const BitBoard (&board)[2][6]);

    //Helper function, converts a piece type to it's string representation
    std::string PieceTypeToStr(PieceType piece);

    //Prints an encoded move's data out
    void PrintEncodedMoveStr(Move move);

    //Prints an encoded move's data out in it's encoded binary form
    void PrintEncodedMoveBin(Move move);

    //More detailed version of the print bitboard function, can be used to 
    //print 2 bitboards worth of pieces
    void PrintUsThem(BitBoard us, BitBoard them, bool mirrored = false);
    
    void PrintUsThemBlank(BitBoard us, BitBoard them, bool mirrored = false);
    //Prints out the attack squares of the moves stored in a move_info as a bitboard
    void PrintEncodedMovesMoveInfo(const move_info& move, bool mirrored = false);

    //Prints out a uint8_t's pieces
    void PrintU8BB(U8 bb, U8 board_center, bool mirrored = false);

    void PrintBoardGraphically(BitBoard* boards);
}

#endif // #ifndef DEBUG_HPP