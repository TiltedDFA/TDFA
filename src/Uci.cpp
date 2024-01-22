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
    time_manager.SetOptions(60'000, 0);
    transpos_table.Resize(TT_SIZE);
    std::cout << "readyok\n";
    // std::cout.flush();
}
void UCI::HandleGo(const ArgList& args)
{
    //update the time settings
    {
        U64 wtime{60'000};
        U64 btime{60'000};
        U64 winc{0};
        U64 binc{0};
        for(size_t i{0}; i < args.size(); ++i)
        {
            if(args[i] == "wtime")
                std::from_chars(args[i + 1].data(), args[i + 1].data() + args[i + 1].size(), wtime);
            if(args[i] == "btime")
                std::from_chars(args[i + 1].data(), args[i + 1].data() + args[i + 1].size(), btime);
            if(args[i] == "winc")
                std::from_chars(args[i + 1].data(), args[i + 1].data() + args[i + 1].size(), binc);
            if(args[i] == "binc")
                std::from_chars(args[i + 1].data(), args[i + 1].data() + args[i + 1].size(), binc);
        }
        if(pos.WhiteToMove())
        {
            time_manager.SetOptions(wtime, winc);
        }
        else
        {
            time_manager.SetOptions(btime, binc);
        }
    }
    //start the timer for this round of calculation
    time_manager.StartTiming();
    std::cout << std::format("bestmove {}\n", UTIL::MoveToStr(Search::FindBestMove(&pos, &transpos_table, &time_manager)));
    std::cout.flush();
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

    // #if DEVELOPER_MODE == 1
    //     // if(Moves::PType(move) == Moves::BAD_MOVE)/*handle error*/;
    // #endif

        pos.MakeMove(move);
    }
}
void UCI::HandleStop()
{

}
void UCI::HandleNewGame()
{
    pos = Position(STARTPOS);
    transpos_table.Clear();
    time_manager.SetOptions(60'000,0);
}
// void UCI::HandleSetOption(const ArgList& args)
// {
//     //there are no options to set as of this version
// }
void UCI::HandleBench(const ArgList& args)
{
    if(args[1] != "perft") return;

    if(args.size() > 2 && args[2] == "suite")
    {
        std::cout << "running perft suite bench\n";
        if(RunPerftSuite<false>() == false) 
            std::cout << "Failed to open perftsuite.epd";
        else
            std::cout << "completed perft suite bench\n";
    }
    else
    {
        std::cout << "running perft bench\n";
        RunBenchmark<false>();
        std::cout << "completed perft bench\n";
    }
    
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
            HandleGo(args);
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
        // case 7:
        //     HandleSetOption(args);
        //     break;
        case 8:
            HandleBench(args);
            break;
        default:
            break;
        }
    }
}