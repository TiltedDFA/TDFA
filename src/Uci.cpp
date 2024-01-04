#include "Uci.hpp"

std::string GetFirstWord(const std::string& str)
{
    int i = -1;
    while(str[++i] != ' ');
    return str.substr(0,i);
}
void UCI::HandleUci()
{
    std::cout << (std::string("id name ") + ENGINE_NAME + '\n');
    std::cout << (std::string("id author ") + ENGINE_AUTHOR + '\n');
    std::cout << "uciok\n";
}
void UCI::HandleIsReady()
{
    std::cout << "readyok\n";
}
void UCI::HandleGo()
{

}
void UCI::HandlePosition(const std::string& str)
{

}
void UCI::HandleStop()
{

}
void UCI::HandleNewGame()
{

}
void UCI::HandleSetOption(const std::string& str)
{

}
void UCI::loop()
{
    std::string input{""};
    while(input != "quit")
    {
        std::getline(std::cin, input);
        const auto it = INIT_VALUES.find(input);
        if(it == INIT_VALUES.end()) continue;
        switch (it->second)
        {
        case 1:
            HandleUci();
            break;
        case 2:
            HandleIsReady();
            break;
        case 3:
            HandleGo();
            break;
        case 4:
            HandlePosition(input);
            break;
        case 5:
            HandleStop();
            break;
        case 6:
            HandleNewGame();
            break;
        case 7:
            HandleSetOption(input);
            break;
        default:
            break;
        }
    }
}