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

    // Position pos(STARTPOS);
    // Debug::PrintBoardGraphically(pos.GetArray());


    Uci uci;
    uci.Loop();
    return 0;
}
