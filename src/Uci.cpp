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
    transpos_table.Resize(TT_SIZE);
    std::cout << "readyok\n";
    // std::cout.flush();
}
void UCI::HandleGo()
{
    U64 time_taken_ms{0};
    {
        Timer<std::chrono::milliseconds> timer(&time_taken_ms);

        std::cout << std::format("bestmove {}\n", UTIL::MoveToStr(Search::FindBestMove(pos, transpos_table)));
    }
    #if USE_TRANSPOSITION_TABLE == 1
        std::cout << std::format("Time: {}ms, Search depth: {}, TT size: {}mB, TT enabled: {}\n", time_taken_ms, SEARCH_DEPTH, TT_SIZE, true);
    #else
        std::cout << std::format("Time: {}ms, Search depth: {}, TT size: {}mB, TT enabled: {}\n", time_taken_ms, SEARCH_DEPTH, TT_SIZE, false);
    #endif
    // size_t i{0};
    // while(i < 1'000'000)
    // {
    //     MoveList ml;
    //     if (i&1)
    //     {
    //         MoveGen::GenerateLegalMoves<false>(pos, ml);
    //     }
    //     else
    //     {
    //         MoveGen::GenerateLegalMoves<true>(pos, ml);
    //     }
    //     pos.MakeMove(ml[0]);
    //     transpos_table.Store(pos.postion_key_, Eval::Evaluate(pos), ml[0], 7, BoundType::EXACT_VAL);
    //     ++i;
    // }
    // std::cout << "done\n"; 
}
void UCI::HandlePosition(const ArgList& args)
{
    if(args[1] == "fen")
    {
        std::string constructed_fen{""};

        for(std::size_t i{2}; i < 7; ++i)
            constructed_fen += std::string(args[i]) + ' ';

        constructed_fen += std::string(args[7]);

        pos.ImportFen(constructed_fen);
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
    transpos_table.Clear();
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