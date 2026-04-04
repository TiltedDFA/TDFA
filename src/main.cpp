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

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);
    if(argc > 1 && std::string_view(argv[1]) == "bench")
    {
        RunBenchmark<false>();
    }
    else
    {
        Uci uci;
        uci.Loop();
    }
    return 0;
}
