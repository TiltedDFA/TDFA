#include <iostream>

#include "core/headers/MagicConstants.hpp"

constexpr unsigned long long b = 0xFF;

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
    PrintBB(0xFF);
    std::cout << Magics::LSBIndex<unsigned>(0xFF) << std::endl;
    std::cout << Magics::MSBIndex<unsigned>(0xFF) << std::endl;
    return 0;
}