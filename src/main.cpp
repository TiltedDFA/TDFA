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
    std::ios::sync_with_stdio(false);
    // std::cin.tie(nullptr);
    // std::ios::sync_with_stdio(true);
    // Position pos;
    // assert((pos.Pieces(White) | pos.Pieces(Black)) == pos.Pieces(White, Black));
    // RunBenchmark<false>();
    //tf
    // TestPerft<true>(5,5363555,1,"rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq a3 0 1");
    // TestPerft<true>(4,204508,1,"rnbqkbnr/1ppppppp/8/p7/P7/8/1PPPPPPP/RNBQKBNR w KQkq a6 0 2");
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
    // RunBenchmark<false>();
    // TestSearch();
//    PRINTNL(int(Moves::EncodeMove(55,48,Rook)));
    // std::ios::sync_with_stdio(false);
    Uci uci;
    uci.Loop();
    return 0;
}
