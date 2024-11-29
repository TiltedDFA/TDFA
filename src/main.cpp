#include <iostream>

#include "MagicConstants.hpp"
#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Testing.hpp"
#include "Debug.hpp"
#include "Search.hpp"
#include "Timer.hpp"
#include "Uci.hpp"
#include "TranspositionTable.hpp"
#include "ZobristConstants.hpp"

int main(void)
{
    Position pos;
    assert((pos.Pieces(White) | pos.Pieces(Black)) == pos.Pieces(White, Black));
    RunBenchmark<true>();
    // RunBulkBenchmark<false>();
    // RunPerftSuite<false>();
    // BitBoard board = 1ull << 54;
    // std::cout << board << std::endl;
    // Debug::PrintBB(board);


    // BitBoard board = 8796227241984;
    // Debug::PrintBB(board);
    // U8 fileof = Magics::FileOf(Magics::FindLS1B(board));
    // std::cout << (int)fileof << std::endl;
    // Debug::PrintBB(board >> fileof);
//    RunBenchmark<false>();
//    PRINTNL(int(Moves::EncodeMove(55,48,Rook)));
    // std::ios::sync_with_stdio(false);
    // Uci uci;
    // uci.Loop();
    return 0;
}
