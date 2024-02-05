#include <iostream>

#include "MagicConstants.hpp"
#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Testing.hpp"
#include "Debug.hpp"
#include "Timer.hpp"
#include "ZobristConstants.hpp"


int main(void)
{
    Pext::PextInit();
    MagicBitBoards::init();

    std::cout << "Titboard perft group:\n";
    RunBenchmark<Titboards      ,true>();
    std::cout << "\nPext perft group:\n";
    RunBenchmark<PextBoards     ,true>();
    std::cout << "\nMagic Bitboard perft group:\n";
    RunBenchmark<MagicBitboards ,true>();
    return 0;
}
