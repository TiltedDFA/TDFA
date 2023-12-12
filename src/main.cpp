#include <iostream>

#include "Core/MagicConstants.hpp"
#include "MoveGen/MoveList.hpp"
#include "MoveGen/MoveGen.hpp"
#include "Core/Testing.hpp"
#include "Core/Debug.hpp"
#include "Core/Search.hpp"
#include "Core/Timer.hpp"
#include "Core/ZobristConstants.hpp"

int main(void)
{
    // RunBenchmark<false>();

    Zobrist::Init();

    for(const ZobristKey i : Zobrist::EN_PASSANT_ARR) PRINTNL(i);
    PRINTNL("------------------");
    for(const ZobristKey i : Zobrist::CAST_ARR) PRINTNL(i);

    // BB::Position pos(STARTPOS);
    // MoveGen generator(pos);
    // MoveList ml;
    // Search finder(pos);

    // generator.GenerateLegalMoves<true>(pos, ml);

    // uint64_t all_time{0};
    // const int search_depth{8};
    // {
    //     Timer<std::chrono::milliseconds> all_timer(&all_time);
    //     for(size_t i = 0; i < ml.len(); ++i)
    //     {
    //         pos.MakeMove(ml[i]);

    //         Score eval{0};
    //         uint64_t time;

    //         finder.SetPos(pos);
    //         {
    //             Timer<std::chrono::microseconds> timer(&time);
    //             eval = finder.GoSearch(search_depth, Eval::NEG_INF, Eval::POS_INF);
    //             // eval = finder.GoSearch(search_depth);
    //         }
    //         PRINTNL(std::format("Depth: {}, Move: {}, Score: {},\t Time: {:.3f}s", search_depth, UCI::move(ml[i]), eval, static_cast<double>(time)/ 1'000'000.0));

    //         pos.UnmakeMove();
    //     }
    // }
    // PRINTNL(all_time);
    // PRINTNL(std::format("It considered: {} nodes, with {:.2f} nps", finder.nodes_, F_Div(finder.nodes_, all_time)*1000));
    return 0;
}
