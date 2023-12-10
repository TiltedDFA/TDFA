#include <iostream>

#include "Core/MagicConstants.hpp"
#include "MoveGen/MoveList.hpp"
#include "MoveGen/MoveGen.hpp"
#include "Core/Testing.hpp"
#include "Core/Debug.hpp"
#include "Core/Search.hpp"
#include "Core/Timer.hpp"

int main(void)
{
    // RunBenchmark<false>();
    BB::Position pos(STARTPOS);
    MoveGen generator(pos);
    MoveList ml;
    Search finder(pos);

    generator.GenerateLegalMoves<true>(pos, ml);

    for(int i = 0; i < ml.len(); ++i)
    {
        pos.MakeMove(ml[i]);

        finder.SetPos(pos);

        PRINTNL(std::format("After move {}, score is: {}", UCI::move(ml[i]), finder.GoSearch(4)));

        pos.UnmakeMove();
    }
    return 0;
}
