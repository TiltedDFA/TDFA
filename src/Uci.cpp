#include "Uci.hpp"

#include <utility>

ArgList SplitArgs(std::string* inp)
{
    ArgList ret;

    if(inp->empty()) return {""};

//    std::ranges::transform(std::as_const(*inp), inp->begin(), [](unsigned char c){return std::tolower(c);});

    std::size_t start{0}, end{0};

    while(end < inp->size())
    {
        if(inp->at(end++) == ' ')
        {
            ret.emplace_back(inp->c_str() + start, inp->c_str() + (end - 1));
            start = end;
        }
    }

    if(inp->at(inp->size() - 1) != ' ')
        ret.emplace_back(inp->c_str() + start, inp->c_str() + end);

    return ret;
}
void Uci::HandleUci()
{
    std::cout << (std::string("id name "    ) + ENGINE_NAME + '\n');
    std::cout << (std::string("id author "  ) + ENGINE_AUTHOR + '\n');
    std::cout << std::format("option name Hash type spin default {} min 1 max 32767\n", tt_size_);
    std::cout << "uciok\n";
}
void Uci::HandleIsReady()
{
    time_manager_.SetOptions(60'000, 0);
    std::cout << "readyok\n";
}
void Uci::HandleGo(const ArgList& args)
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
                std::from_chars(args[i + 1].data(), args[i + 1].data() + args[i + 1].size(), winc);
            if(args[i] == "binc")
                std::from_chars(args[i + 1].data(), args[i + 1].data() + args[i + 1].size(), binc);
        }
        if(pos_.ColourToMove() == White)
        {
            time_manager_.SetOptions(wtime, winc);
        }
        else
        {
            time_manager_.SetOptions(btime, binc);
        }
    }
    //start the timer for this round of calculation
    time_manager_.StartTiming();
    std::cout << std::format("bestmove {}\n", UTIL::MoveToStr(search_.FindBestMove(&pos_, &tt_, &time_manager_)));
    std::cout.flush();
}
void Uci::HandlePosition(const ArgList& args)
{
    if(args[1] == "fen")
    {
        assert(args.size() >= 7);

        std::string constructed_fen;
        for(std::size_t i{2}; i < 7; ++i)
            constructed_fen += std::string(args[i]) + ' ';

        constructed_fen += std::string(args[7]);

//        PRINTNL("Here");

        pos_.ImportFen(constructed_fen);
    }
    else if (args[1] == "startpos")
    {
        pos_.ImportFen(STARTPOS);
    }

    ArgList::const_iterator it;
    if((it = std::ranges::find(args, "moves")) == args.cend()) return;
    
    for(++it ;it != args.end(); ++it)
    {
        pos_.MakeMove(UTIL::UciToMove(*it, pos_));
    }
}
void Uci::HandleStop()
{

}
void Uci::HandleNewGame()
{
    pos_ = Position(STARTPOS);
    tt_.Clear();
    time_manager_.SetOptions(60'000,0);
}
void Uci::HandleSetOption(const ArgList& args)
{
    if(args[2] == "hash")
    {
        std::from_chars(args[4].data(), args[4].data() + args[4].size(), tt_size_);
        tt_.Resize(tt_size_);
    }
}
void Uci::HandleBench(const ArgList& args)
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
void Uci::HandlePrint(const ArgList& args)
{
    if(args[1] == "state")
    {
        Debug::PrintBoardGraphically(&pos_);
        Debug::PrintBoardState(pos_);
    }
}
void Uci::Loop()
{
    std::string input;
    while(input != "quit")
    {
        std::getline(std::cin, input);

        ArgList args = SplitArgs(&input);

        const auto it = COMMAND_VALUES.find(args[0]);

        if(it == COMMAND_VALUES.end()) continue;
        
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
        case 7:
            HandleSetOption(args);
            break;
        case 8:
            HandleBench(args);
            break;
        case 9:
            HandlePrint(args);
            break;
        default:
            break;
        }
    }
}
