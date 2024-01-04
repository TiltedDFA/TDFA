#include "Uci.hpp"


std::string UCI::square(Sq sq)
{
    return std::string{char('a' + Magics::FileOf(sq)), char('1' + Magics::RankOf(sq))};
}
std::string PromotionChar(PieceType p)
{
    switch (p)
    {
    case Moves::PROM_BISHOP:
        return "b";
    case Moves::PROM_KNIGHT:
        return "n";
    case Moves::PROM_QUEEN:
        return "q";
    case Moves::PROM_ROOK:
        return "r";
    default:
        return "z";
    }
}
std::string UCI::move(Move m)
{
    return (Moves::IsPromotionMove(m) ? (square(Moves::GetStartIndex(m)) + square(Moves::GetTargetIndex(m)) + PromotionChar(Moves::GetPieceType(m))) :
                                        square(Moves::GetStartIndex(m)) + square(Moves::GetTargetIndex(m)));
}
std::string GetFirstWord(const std::string& str)
{
    int i = -1;
    while(str[++i] != ' ');
    return str.substr(0,i);
}
void UCI::HandleUci()
{
    std::cout << (std::string("id name") + ENGINE_NAME + '\n');
    std::cout << (std::string("id author") + ENGINE_AUTHOR + '\n');
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