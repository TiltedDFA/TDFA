#include <iostream>

#include "Core/MagicConstants.hpp"
#include "MoveGen/MoveList.hpp"
#include "MoveGen/MoveGen.hpp"
#include "Core/Testing.hpp"
#include "Core/Debug.hpp"

constexpr unsigned long long b = 0xFF;
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define TEST_FEN_LONG "rnbq1rk1/pp2ppbp/6p1/2pp4/2PPnB2/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8"
#define TEST_FEN_WHITE_SPACE "                            8/8/8/8/4Pp2/8/8/8 w - e3 0 1                              "

int main(void)
{
    BB::Position pos(START_FEN);
    MoveGen gen;
    MoveList list;
    gen.GenerateAllMoves(pos,list.Current());
    Debug::PrintBB(Debug::MoveToAtkBoard(list));
    PRINT(list.Size());
    return 0;
}
