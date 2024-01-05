#include "Uci.hpp"
ArgList SplitArgs(std::string_view inp)
{
    ArgList ret{};

    if(inp.size() == 0 ) return ret;

    std::size_t start{0}, end{0};

    while(end < inp.size())
    {
        if(inp[end++] == ' ')
        {
            ret.push_back(inp.substr(start, end - start));
            start = end;
        }
    }
    if(inp[inp.size() - 1] != ' ')
        ret.push_back(inp.substr(start, end - start));
    return ret;
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
void UCI::HandlePosition(const ArgList& args)
{
    if(args[1] == "fen")
    {
        std::string constructed_fen{""};
        for(std::size_t i{2};i < 6; ++i)
            constructed_fen += std::string(args[i]) + ' ';
        constructed_fen += std::string(args[7]);
        pos.ImportFen(constructed_fen);
    }
}
void UCI::HandleStop()
{

}
void UCI::HandleNewGame()
{

}
void UCI::HandleSetOption(const ArgList& args)
{

}
void UCI::loop()
{
    std::string input{""};
    while(input != "quit")
    {
        std::getline(std::cin, input);

        ArgList args = SplitArgs(input);

        const auto it = INIT_VALUES.find(args[0]);
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