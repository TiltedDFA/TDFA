#include "Uci.hpp"

// Zero-alloc argument splitter — operates on string_views into the input buffer
static ArgList SplitArgs(std::string_view inp) noexcept
{
    ArgList ret;
    if(inp.empty()) return ret;

    size_t start = 0;
    for(size_t i = 0; i < inp.size(); ++i)
    {
        if(inp[i] == ' ')
        {
            if(i > start)
                ret.push(inp.substr(start, i - start));
            start = i + 1;
        }
    }
    if(start < inp.size())
        ret.push(inp.substr(start));

    return ret;
}

// Constexpr command lookup — no hash map, no heap
static constexpr U8 CommandId(std::string_view cmd) noexcept
{
    // Compare first char for fast dispatch, then verify full string
    if(cmd.empty()) return 0;
    switch(cmd[0])
    {
    case 'u':
        if(cmd == "uci") return 1;
        if(cmd == "ucinewgame") return 6;
        return 0;
    case 'i':
        if(cmd == "isready") return 2;
        return 0;
    case 'g':
        if(cmd == "go") return 3;
        return 0;
    case 'p':
        if(cmd == "position") return 4;
        if(cmd == "print") return 9;
        return 0;
    case 's':
        if(cmd == "stop") return 5;
        if(cmd == "setoption") return 7;
        return 0;
    case 'b':
        if(cmd == "bench") return 8;
        return 0;
    default:
        return 0;
    }
}

void Uci::HandleUci()
{
    // Single write call — avoid multiple cout << chains
    char buf[256];
    int n = std::snprintf(buf, sizeof(buf),
        "id name %s\nid author %s\noption name Hash type spin default %zu min 1 max 32767\nuciok\n",
        ENGINE_NAME, ENGINE_AUTHOR, tt_size_);
    std::cout.write(buf, n);
}

void Uci::HandleIsReady()
{
    time_manager_.SetOptions(60'000, 0);
    std::cout.write("readyok\n", 8);
}

void Uci::HandleGo(const ArgList& args)
{
    U64 wtime{60'000}, btime{60'000}, winc{0}, binc{0};
    for(U8 i = 1; i < args.size(); ++i)
    {
        const auto key = args[i];
        if(key[0] == 'w')
        {
            if(key == "wtime" && i + 1 < args.size())
                std::from_chars(args[i+1].data(), args[i+1].data() + args[i+1].size(), wtime);
            else if(key == "winc" && i + 1 < args.size())
                std::from_chars(args[i+1].data(), args[i+1].data() + args[i+1].size(), winc);
        }
        else if(key[0] == 'b')
        {
            if(key == "btime" && i + 1 < args.size())
                std::from_chars(args[i+1].data(), args[i+1].data() + args[i+1].size(), btime);
            else if(key == "binc" && i + 1 < args.size())
                std::from_chars(args[i+1].data(), args[i+1].data() + args[i+1].size(), binc);
        }
    }
    if(pos_.ColourToMove() == White)
        time_manager_.SetOptions(wtime, winc);
    else
        time_manager_.SetOptions(btime, binc);

    time_manager_.StartTiming();
    const Move best = search_.FindBestMove(&pos_, &tt_, &time_manager_);

    // Zero-alloc move-to-string directly into stack buffer
    const Sq from = Moves::StartSq(best);
    const Sq to = Moves::TargetSq(best);
    char mbuf[16] = "bestmove ";
    int pos = 9;
    mbuf[pos++] = char('a' + Magics::FileOf(from));
    mbuf[pos++] = char('1' + Magics::RankOf(from));
    mbuf[pos++] = char('a' + Magics::FileOf(to));
    mbuf[pos++] = char('1' + Magics::RankOf(to));
    if(Moves::IsPromotionMove(best))
    {
        switch(Moves::PTypeOfProm(best))
        {
        case pt_Queen:  mbuf[pos++] = 'q'; break;
        case pt_Rook:   mbuf[pos++] = 'r'; break;
        case pt_Bishop: mbuf[pos++] = 'b'; break;
        case pt_Knight: mbuf[pos++] = 'n'; break;
        default: break;
        }
    }
    mbuf[pos++] = '\n';
    std::cout.write(mbuf, pos);
    std::cout.flush();
}

void Uci::HandlePosition(const ArgList& args)
{
    if(args[1] == "fen")
    {
        assert(args.size() >= 8);
        // Build FEN on stack — no heap allocation
        char fen_buf[256];
        int len = 0;
        for(U8 i = 2; i < 7; ++i)
        {
            std::memcpy(fen_buf + len, args[i].data(), args[i].size());
            len += args[i].size();
            fen_buf[len++] = ' ';
        }
        std::memcpy(fen_buf + len, args[7].data(), args[7].size());
        len += args[7].size();
        fen_buf[len] = '\0';

        pos_.ImportFen(std::string_view(fen_buf, len));
        pos_.HashCurrentPostion();
    }
    else if(args[1] == "startpos")
    {
        pos_.ImportFen(STARTPOS);
        pos_.HashCurrentPostion();
    }

    // Find "moves" token and apply
    for(U8 i = 0; i < args.size(); ++i)
    {
        if(args[i] == "moves")
        {
            for(U8 j = i + 1; j < args.size(); ++j)
                pos_.MakeMove(UTIL::UciToMove(args[j], pos_));
            return;
        }
    }
}

void Uci::HandleStop()
{
}

void Uci::HandleNewGame()
{
    pos_ = Position(STARTPOS);
    tt_.Clear();
    time_manager_.SetOptions(60'000, 0);
}

void Uci::HandleSetOption(const ArgList& args)
{
    if(args.size() >= 5 && args[2] == "hash")
    {
        std::from_chars(args[4].data(), args[4].data() + args[4].size(), tt_size_);
        tt_.Resize(tt_size_);
    }
}

void Uci::HandleBench(const ArgList& args)
{
    if(args.size() < 2 || args[1] != "perft") return;

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
    if(args.size() >= 2 && args[1] == "state")
    {
        Debug::PrintBoardGraphically(&pos_);
        Debug::PrintBoardState(pos_);
    }
}

void Uci::Loop()
{
    while(true)
    {
        std::getline(std::cin, input_buf_);
        if(input_buf_ == "quit") return;

        const ArgList args = SplitArgs(input_buf_);
        if(args.empty()) continue;

        switch(CommandId(args[0]))
        {
        case 1: HandleUci(); break;
        case 2: HandleIsReady(); break;
        case 3: HandleGo(args); break;
        case 4: HandlePosition(args); break;
        case 5: HandleStop(); break;
        case 6: HandleNewGame(); break;
        case 7: HandleSetOption(args); break;
        case 8: HandleBench(args); break;
        case 9: HandlePrint(args); break;
        default: break;
        }
    }
}
