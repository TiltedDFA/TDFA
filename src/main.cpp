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
    /*
    BB::Position pos{};
    pos.ImportFen(TESTFEN9);
    BitBoard pieces = 0x448800;//pos.GetPieces<true>();
    Debug::PrintBB(pieces);
    Debug::PrintBB(Magics::CollapsedFilesIndex(pieces));
    std::cout << Magics::CollapsedFilesIndex(pieces);
    */
    // Debug::PrintBB(0x02020202020202);
    // Debug::PrintBB(Magics::CollapsedRanksIndex(0x02020202020202));

    /////// This is for testing all generated moves
    // BB::Position pos;
    // pos.ImportFen(START_FEN);
    // MoveGen generator;
    // MoveGen::EnPassantTargetSquare = 0x00;
    // MoveList list = generator.GenerateAllMoves(pos);
    // move_info info{};
    // Move* currentMove = list.First();
    // for(size_t i = 0; i < list.Size();++i)
    // {
    //     Debug::ShortPrintEncodedMoveStr(*currentMove);
    //     ++currentMove;
    // }
    // for(int i = 0; i < 64; ++i)
    // {
    //     Debug::PrintBB(Magics::SLIDING_ATTACKS_MASK[i][(int)D::ADIAG],i,true);
    // }  


    //////// This is for testing some specfic test cases with the titboard move generator
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
    // RunTitBoardTest<D::RANK>(2,"8/8/8/8/8/8/8/P1R2P1n w - - 0 1",info);
    // PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(29,"8/8/8/8/5R2/8/8/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(29,"8/8/8/5p2/5R2/8/5P2/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::DIAG>(24, "8/8/8/8/B7/8/8/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;
    
    BB::Position pos("8/8/8/5p2/5R2/8/5P2/8 w - - 0 1");
    Debug::PrintBB(pos.GetPieces<true>() | pos.GetPieces<false>());
    Debug::PrintBB(Magics::CollapsedRanksIndex(pos.GetPieces<true>() | pos.GetPieces<false>()));
    // std::cout << sizeof(MoveGen::SLIDING_ATTACK_CONFIG);
    // for(int i = 0; i < 4;++i)
    // {
    //     for(int sq = 0; sq < 64;++sq)
    //     {
    //         std::cout << "At sq:" << sq << "in direction" << i << std::endl;
    //         Debug::PrintBB(Magics::SLIDING_ATTACKS_MASK[sq][i]);
    //     }
    // }
    return 0;
}
