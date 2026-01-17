#include <iostream>

#include "MagicConstants.hpp"
#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Testing.hpp"
#include "Debug.hpp"
#include "Timer.hpp"
#include "ZobristConstants.hpp"
#include "Pext.hpp"
#include "MagicBitboards.hpp"

int main(void)
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    Pext::PextInit();
    MagicBitBoards::init();

    std::cout << "Titboard perft group:\n";
    RunBenchmark<Titboards, false>();
    std::cout << "\nTitboard (PEXT) perft group:\n";
    RunBenchmark<TitboardsPext, false>();
    std::cout << "\nMagic Bitboard perft group:\n";
    RunBenchmark<MagicBitboards, false>();
    std::cout << "\nPext Bitboard perft group:\n";
    RunBenchmark<PextBoards, false>();
    return 0;
}
