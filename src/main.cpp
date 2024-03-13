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
    // RunBenchmark<false>();
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
    Uci uci;
    uci.Loop();
    return 0;
}
