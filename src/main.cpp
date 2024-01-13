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

    // UCI::loop();
    BB::Position pos(STARTPOS);
    MoveGen generator(pos);
    MoveList ml;
    Search finder(pos);

    // std::cout << finder.GoSearch(5, Eval::NEG_INF, Eval::POS_INF) << '\n';
    // std::cout << finder.GoSearch(5, Eval::POS_INF, Eval::NEG_INF) << '\n';
    // std::string tmp;
    // std::cin >> tmp;
    generator.GenerateLegalMoves<true>(pos, ml);

    uint64_t all_time{0};
    const int search_depth{8};
    {
        Timer<std::chrono::milliseconds> all_timer(&all_time);
        for(size_t i = 0; i < ml.len(); ++i)
        {
            pos.MakeMove(ml[i]);

            Score eval{0};
            uint64_t time;

            finder.SetPos(pos);
            {
                Timer<std::chrono::microseconds> timer(&time);
                eval = finder.GoSearch(search_depth);
            }
            PRINTNL(std::format("Depth: {}, Move: {}, Score: {},\t Time: {:.3f}s", search_depth, UTIL::MoveToStr(ml[i]), eval, static_cast<double>(time)/ 1'000'000.0));

            pos.UnmakeMove();
        }
    }
    PRINTNL(all_time);
    return 0;
}
