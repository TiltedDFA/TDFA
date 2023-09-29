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
    // BB::Position pos(START_FEN);
    // MoveGen gen;
    // MoveList list;
    // gen.GenerateAllMoves(pos,list.Current());
    // Debug::PrintBB(Debug::MoveToAtkBoard(list));
    // PRINT(list.Size());

    move_info info;
    RunTitBoardTest<D::DIAG>(12,"8/8/8/8/8/8/4B3/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::ADIAG>(12,"8/8/8/8/8/8/4B3/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;
    
    RunTitBoardTest<D::DIAG>(36,"8/6n1/8/4B3/8/8/8/R7 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::ADIAG>(36,"8/6n1/8/4B3/8/8/8/R7 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(6,"8/8/8/8/8/8/8/6R1 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::RANK>(4,"8/8/8/8/8/8/8/P3R1p1 w -- 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::RANK>(2,"8/8/8/8/8/8/8/P1R2P1n w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(29,"8/8/8/8/5R2/8/8/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(29,"8/8/8/5p2/5R2/8/5P2/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::DIAG>(24, "8/8/8/8/B7/8/8/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;
    
    BB::Position pos("8/8/8/5p2/5R2/8/5P2/8 w - - 0 1");
    Debug::PrintBB(pos.GetPieces<true>() | pos.GetPieces<false>());
    Debug::PrintBB(Magics::CollapsedRanksIndex(pos.GetPieces<true>() | pos.GetPieces<false>()));
    Debug::PrintBB(4647998506626711584);
    Debug::PrintBB(18156244167032864);
    return 0;
}
