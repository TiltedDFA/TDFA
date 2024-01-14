#include <iostream>

#include "MagicConstants.hpp"
#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Testing.hpp"
#include "Debug.hpp"
#include "Search.hpp"
#include "Timer.hpp"
#include "Uci.hpp"
#include "ZobristConstants.hpp"

int main(void)
{
    // RunBenchmark<false>();
    // RunPerftSuite<false>();
    // Zobrist::Init();

    // for(const ZobristKey i : Zobrist::EN_PASSANT_ARR) PRINTNL(i);
    // PRINTNL("------------------");
    // for(const ZobristKey i : Zobrist::CAST_ARR) PRINTNL(i);

    // PRINTNL(sizeof(MoveGen::SLIDING_ATTACK_CONFIG));

    UCI::loop();

    // MoveList ml;

    // BB::Position pos("1r3k1b/5p1p/4pPpQ/p5P1/B1pP4/2p5/PP5R/R5K1 b - - 1 30");
    // // BB::Position pos("1r3k1b/5p1p/4pPp1/p5P1/B1pP4/2p4Q/PP5R/R5K1 w - - 0 30");

    // MoveGen::GenerateLegalMoves<false>(pos, ml);

    // for(size_t i{0}; i < ml.len(); ++i)
    // {
    //     pos.MakeMove(ml[i]);

    //     Score eval = Search::GoSearch(pos, SEARCH_DEPTH);
        
    //     pos.UnmakeMove();
    //     PRINTNL(std::format("{}. Move: {}, Score: {}", i+1, UTIL::MoveToStr(ml[i]), eval));
    // }
    return 0;
}
