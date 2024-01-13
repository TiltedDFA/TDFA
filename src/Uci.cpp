#include "Uci.hpp"
ArgList SplitArgs(std::string_view inp)
{
    ArgList ret;

    if(inp.size() == 0 ) return {""};

    std::size_t start{0}, end{0};

    while(end < inp.size())
    {
        if(inp[end++] == ' ')
        {
            ret.push_back(inp.substr(start, end - start - 1));
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
    MoveList ml;

    if(pos.whites_turn_)
    {
        generator.GeneratePseudoLegalMoves<true>(pos, ml);
    }
    else
    {
        generator.GeneratePseudoLegalMoves<false>(pos, ml);
    }
    
    Move best_move{0};
    Score best_eval = (pos.whites_turn_ ? Eval::NEG_INF : Eval::POS_INF);

    for(size_t i{0}; i < ml.len(); ++i)
    {
        pos.MakeMove(ml[i]);
        if(!(pos.whites_turn_ ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
        {
            Score eval = search.GoSearch(6);
            if(pos.whites_turn_)
            {
                if(eval < best_eval) 
                {
                    best_eval = eval;
                    best_move = ml[i];
                }
            }
            else
            {
                if(eval > best_eval) 
                {
                    best_eval = eval;
                    best_move = ml[i];
                }
            }
        }
        pos.UnmakeMove();
    }
    std::cout << std::format("bestmove {}\n", UTIL::MoveToStr(best_move));
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
        Debug::PrintBoardState(pos);
    }
    else if (args[1] == "startpos")
    {
        pos.ImportFen(STARTPOS);
    }

    ArgList::const_iterator it;
    if((it = std::find(args.cbegin(), args.cend(), "moves")) == args.cend()) return;
    
    for(++it ;it != args.end(); ++it)
    {
        const Move move = UTIL::UciToMove(*it, pos);
#if DEVELOPER_MODE == 1
        if(Moves::GetPieceType(move) == Moves::BAD_MOVE)/*handle error*/;
#endif
        pos.MakeMove(move);
        Debug::PrintBoardState(pos);

    }
}
void UCI::HandleStop()
{

}
void UCI::HandleNewGame()
{
    pos = BB::Position(STARTPOS);
}
void UCI::HandleSetOption(const ArgList& args)
{
    //there are no options to set as of this version
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
            HandlePosition(args);
            break;
        case 5:
            HandleStop();
            break;
        case 6:
            HandleNewGame();
            break;
        case 7:
            HandleSetOption(args);
            break;
        default:
            break;
        }
    }
}