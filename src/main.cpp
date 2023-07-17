#include <iostream>

#include "Core/MagicConstants.hpp"
#include "MoveGen/MoveList.hpp"
#include "MoveGen/MoveGen.hpp"
#include "Core/Testing.hpp"


constexpr unsigned long long b = 0xFF;
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define TEST_FEN_LONG "rnbq1rk1/pp2ppbp/6p1/2pp4/2PPnB2/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8"
#define TEST_FEN_WHITE_SPACE "                            8/8/8/8/4Pp2/8/8/8 w - e3 0 1                              "
void PrintBB(BitBoard board, bool mirrored=true)
{
    std::string output{},current_line{};
    for(int row{0}; row < 8; ++row)
    {
        for(int col{0}; col < 8;++col)
        {
            if(((board >> (col + row*8)))&1ull)
            {
                current_line = mirrored ?   current_line + "1 " : "1 " + current_line;
            }
            else
            {
                current_line = mirrored ?   current_line + "0 " : "0 " + current_line;
            }
        }
        current_line += "|" + std::to_string(row + 1) + " \n";
        output = current_line + output;
        current_line = "";
    }                    
    output += "----------------\n";
    output += mirrored ? "A B C D E F G H" : "H G F E D C B A";
    std::cout << output << std::endl;
}
int main(void)
{
    BB::Position pos{};
    pos.ImportFen(START_FEN);
    MoveList instance;
    std::cout << instance.First() << "\n";
    std::cout << instance.Last() << "\n";
    for(uint16_t i = 0; i < MAX_MOVES;++i)
    {
        if(i == 198) *(*instance.Current())++ = std::numeric_limits<Move>::max()-1; 
        else *(*instance.Current())++ = i;
    }
    std::cout << instance.Last() << "\n";
    std::cout << instance.Contains(Move(std::numeric_limits<Move>::max())) << '\t' << std::numeric_limits<Move>::max();
    CmpMoveLists(instance,std::vector<Move>());
    return 0;
}