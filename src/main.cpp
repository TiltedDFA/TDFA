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
void test_movegen()
{
    std::fstream perft_file("perftsuite.epd", std::ios::in);

    if(!perft_file.is_open()){std::cout << "file not found \n"; return;}

    std::string line;

    while(std::getline(perft_file, line))
    {
        if(line[0] == '#') continue;
        
        std::vector<std::string> chunks = Split(line, ";");
        const std::string& fen = chunks[0];

        Position pos(fen);
        const BitBoard tit_attacks_w = MoveGen::TitQueenAttacks<true>(&pos);
        const BitBoard tit_attacks_b = MoveGen::TitQueenAttacks<false>(&pos);
        const BitBoard pext_attacks_w = MoveGen::QueenAttacks<true>(&pos);
        const BitBoard pext_attacks_b = MoveGen::QueenAttacks<false>(&pos);
        if(tit_attacks_b != pext_attacks_b)
        {
            std::cout << "Fen: " << fen << "\n";
            std::cout << "Mismatch\nExpected: \n";
            Debug::PrintBB(tit_attacks_b);
            std::cout << "Was: \n";
            Debug::PrintBB(pext_attacks_b);
            return;
        }
         if(tit_attacks_w != pext_attacks_w)
        {
            std::cout << "Fen: " << fen << "\n";
            std::cout << "Mismatch\n Expected: \n";
            Debug::PrintBB(tit_attacks_w);
            std::cout << "Was: \n";
            Debug::PrintBB(pext_attacks_w);
            return;
        }
    }

    perft_file.close();
}
int main(void)
{
    #if USE_TITBOARDS != 1
    Pext::PextInit();
    #endif 
    test_movegen();
    // RunBenchmark<false>();
    return 0;
}
