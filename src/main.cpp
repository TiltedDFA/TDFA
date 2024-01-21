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
    // RunPerftSuite<false>();

    // PRINTNL("------------------");
    // for(const ZobristKey i : Zobrist::CASTLING) PRINT(std::format("{},\n",i));
    // PRINTNL(sizeof(HashEntry));
    // // PRINTNL(sizeof(MoveGen::SLIDING_ATTACK_CONFIG));

    // PRINTNL(alignof(HashEntry));
    // TransposTable tt;
    // tt.Resize(1024);
    // Position pos(STARTPOS);
    
    // // PRINTNL(Eval::Evaluate(pos));
    // // pos.ImportFen("rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR b KQkq - 0 1");    
    // // PRINTNL(Eval::Evaluate(pos));
    // for(int i = 0; i < 9; ++i)
    // {
    //     PRINT(std::format("{}, {}\n", i, Search::GoSearch(tt, pos, i)));
    // }
    // pos.ImportFen("5k2/8/4K3/5P2/8/8/8/8 b - - 0 1");
    // for(int i = 0; i < 9; ++i)
    // {
    //     PRINT(std::format("{}, {}\n", i, Search::GoSearch(tt, pos, i)));
    // }

    // PRINTNL(alignof(Position));
    // PRINTNL(sizeof(Position));
    // PRINTNL(sizeof(move_info));
    // PRINTNL(alignof(move_info));

    // PRINTNL(sizeof(SLIDING_ATTACK_CONFIG));
    // PRINTNL(alignof(SLIDING_ATTACK_CONFIG));
    UCI::loop();

    // MoveList ml;

    // Position pos("1r3k1b/5p1p/4pPpQ/p5P1/B1pP4/2p5/PP5R/R5K1 b - - 1 30");
    // // Position pos("1r3k1b/5p1p/4pPp1/p5P1/B1pP4/2p4Q/PP5R/R5K1 w - - 0 30");

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
