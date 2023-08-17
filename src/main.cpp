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

/*
    BB::Position pos{};
    pos.ImportFen(START_FEN);
    MoveList instance;
    MoveGen generator;
    std::cout << instance.First() << "\n";
    std::cout << instance.Last() << "\n";
    for(uint16_t i = 0; i < MAX_MOVES;++i)
    {
        if(i == 198) *(*instance.Current())++ = std::numeric_limits<Move>::max()-1; 
        else *(*instance.Current())++ = i;
    }
    std::cout << instance.Last() << "\n";
    std::cout << instance.Contains(Move(std::numeric_limits<Move>::max())) << '\t' << std::numeric_limits<Move>::max();
    CmpMoveLists(instance,std::vector<Move>());
    generator.UpdateVariables(pos.GetPieces<true>(),pos.GetPieces<false>());
    Debug::PrintEntireBoard(pos.pieces_);
    for(uint8_t i = 0; i < 64;++i)
    {
        std::cout << "Index: " << (int)i << " Rank: " << static_cast<int>(Magics::rank_of(i)) << " File: " << static_cast<int>(Magics::file_of(i)) << std::endl;
    }
    Debug::PrintBB(Magics::CROSS_DIAG);
    Debug::PrintBB(Magics::ANTI_CROSS_DIAG);
    const std::array<std::array<BitBoard, 4>, 64> data = PrecomputeMask();
    for(int i = 0; i < 64;++i)
    {
        for(int j = 0; j < 4;++j)
        {
            std::cout << "SQ: " << i << " DIR: " << j << std::endl;
            Debug::PrintBB(data[i][j]);
        }
    }
    */
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
    // Debug::PrintBB(0x02020202020202);
    // Debug::PrintBB(Magics::CollapsedRanksIndex(0x02020202020202));
    move_info info;
    RunTitBoardTest<D::RANK>(2,"8/8/8/8/8/8/8/p1R2p1N w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(6,"8/8/8/8/8/8/8/6R1 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::RANK>(2,"8/8/8/8/8/8/8/P1R2P1n w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(29,"8/8/8/8/5R2/8/8/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;

    RunTitBoardTest<D::FILE>(29,"8/8/8/5p2/5R2/8/5P2/8 w - - 0 1",info);
    PRINT_TIT_TEST_RESULTS;
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
