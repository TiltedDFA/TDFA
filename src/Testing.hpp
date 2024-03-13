#ifndef TESTING_HPP
#define TESTING_HPP

#include <vector>

#include "BitBoard.hpp"
#include "Debug.hpp"
#include "MagicConstants.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <format>
#include <fstream>
#include "MoveGen.hpp"
#include "MoveList.hpp"
#include "Util.hpp"
#include "Timer.hpp"

//white pawn move gen tests
#define TESTFEN1 "8/8/8/4pP2/8/8/8/8 w - e6 0 1"     //checks left capturing en passant
#define TESTFEN2 "8/8/8/8/2p5/8/2P5/8 w - - 0 1"    //checks obsticle in way of double move - single move should still work
#define TESTFEN3 "8/8/8/5Pp1/8/8/8/8 w - g6 0 1"     //checks right capturing en passant 
#define TESTFEN4 "8/8/3qp3/4P3/8/8/8/8 w - - 0 1"   //checks left capture with no other possible moves
#define TESTFEN5 "8/8/8/8/8/8/rb6/Pr6 w - - 0 1"    //checks right capture with no other possible moves
#define TESTFEN6 "8/8/8/8/8/8/r7/Pr6 w - - 0 1"     //checks another position with no possible moves
#define TESTFEN7 "8/8/8/qq6/P1q1q3/4P1q1/2P3PP/8 w - - 0 1" //complicated position with lots of possible moves
#define TESTFEN8 "8/8/2p1kPp1/6P1/4K3/8/8/8 w - - 0 1" //pawn endgame with 1 possible pawn move for white 
#define TESTFEN9 "R7/P5k1/8/8/8/6P1/6K1/r7 w - - 0 1" //another pawn endgame with 1 possible pawn moves for white
#define TESTFEN10 "rnbq1rk1/pp2ppbp/6p1/2pp4/2PPnB2/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8" //very complicated position taken from queens gambit opening with many possible white pawn moves

#define STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define KIWIPETE "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"
#define PERFTPOS3 "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"
#define PERFTPOS4 "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
#define PERFTPOS5 "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
#define PERFTPOS6 "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
#define TRICKYENDGAMEPOS "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1"
#define PERPETUALCHECK "6k1/6p1/8/6KQ/1r6/q2b4/8/8 w - - 0 1"
class PerftHandler
{
public:
    PerftHandler():perft_data_({}),total_nodes_(0){}

    void ResetData(){perft_data_ = std::vector<std::string>{}; total_nodes_ = 0;}

    template<bool output_perft_paths>
    void RunPerft(int depth, Position* pos){ResetData(); Perft<true, output_perft_paths>(depth, pos);}
    template<bool output_perft_paths>
    void RunBulkPerft(int depth, Position* pos){ResetData(); BulkPerft<true, output_perft_paths>(depth, pos);}

    U64 GetNodes() const {return total_nodes_;}

    void PrintData()
    {
        std::cout << std::format("\nTotal: {}\n", total_nodes_);
        std::sort(perft_data_.begin(), perft_data_.end());
        for(const auto& i : perft_data_) std::cout << i;
    }
private:
    template<bool is_root, bool output_perft_paths>
    U64 Perft(int depth, Position* pos)
    {
        if(!depth) return 1ull;

        MoveList ml{};
        U64 nodes{0};

        if(pos->WhiteToMove())
            MoveGen::GeneratePseudoLegalMoves<true>(pos, &ml);
        else
            MoveGen::GeneratePseudoLegalMoves<false>(pos, &ml);
        
        for(size_t i = 0; i < ml.len(); ++i)
        {
            if constexpr(is_root)
            {
            #if TDFA_DEBUG == 1
                const Position copy = *pos;
                Sq start;
                Sq end;
                PieceType t;
                Moves::DecodeMove(ml[i], &start, &end, &t);
                if(start == 2 && end == 47)
                {
                    PRINT("\n");
                }
            #endif
                pos->MakeMove(ml[i]);
                if(!(pos->WhiteToMove() ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
                {
                    const auto cnt = Perft<false, output_perft_paths>(depth - 1, pos);
                    if constexpr(output_perft_paths) 
                        nodes += cnt;
                    total_nodes_ += cnt;
                    if constexpr(output_perft_paths) 
                        perft_data_.push_back(std::format("{} : {}\n", UTIL::MoveToStr(ml[i]), cnt));
                }
                pos->UnmakeMove(ml[i]);
            #if TDFA_DEBUG == 1
                assert(copy == *pos);
            #endif
            }
            else
            {
            #if TDFA_DEBUG == 1
                const Position copy = *pos;
                Sq start;
                Sq end;
                PieceType t;
                Moves::DecodeMove(ml[i], &start, &end, &t);
                //used for breakpointing at specific move in perft when debugging
                // if(start == 14 && end == 6 && t == PromQueen)
                // {
                //     PRINT("");
                // }
            #endif
                pos->MakeMove(ml[i]);
                if(!(pos->WhiteToMove() ? MoveGen::InCheck<false>(pos) : MoveGen::InCheck<true>(pos)))
                {
                    nodes += Perft<false, output_perft_paths>(depth - 1, pos);
                }
                pos->UnmakeMove(ml[i]);
            #if TDFA_DEBUG == 1
                assert(copy == *pos);
            #endif
            }
        }

        return nodes;
    }
    template<bool is_root, bool output_perft_paths>
    U64 BulkPerft(int depth, Position* pos)
    {
        assert(depth > 0);

        MoveList ml{};

        if(pos->WhiteToMove())
            MoveGen::GenerateLegalMoves<true>(pos, &ml);
        else
            MoveGen::GenerateLegalMoves<false>(pos, &ml);
        if(depth == 1) return ml.len();
        U64 nodes{0};
        
        for(size_t i = 0; i < ml.len(); ++i)
        {
            if constexpr(is_root)
            {
                pos->MakeMove(ml[i]);

                const U64 cnt = BulkPerft<false, output_perft_paths>(depth - 1, pos);
                if constexpr(output_perft_paths) nodes += cnt;
                total_nodes_ += cnt;
                if constexpr(output_perft_paths) perft_data_.push_back(std::format("{} : {}\n", UTIL::MoveToStr(ml[i]), cnt));

                pos->UnmakeMove(ml[i]);
            }
            else
            {
                pos->MakeMove(ml[i]);
                nodes += BulkPerft<false, output_perft_paths>(depth - 1, pos);
                pos->UnmakeMove(ml[i]);
            }
        }
        return nodes;
    }
private:
    std::vector<std::string> perft_data_;
    U64 total_nodes_;
};

//returns nps
template<bool output_perft_paths>
U64 TestPerft(unsigned depth, U64 expected_nodes, U16 test_number, const std::string& fen)
{
    PerftHandler perft;
    Position pos(fen);
    U64 time {1};
    
    {
        Timer<std::chrono::microseconds> t(&time);
        perft.RunPerft<output_perft_paths>(depth, &pos);
    }
    if constexpr(output_perft_paths)
    {
        perft.PrintData();
    }

    const U64 nps = static_cast<double>(perft.GetNodes() * 1'000'000) / static_cast<double>(time);

    if(expected_nodes == perft.GetNodes())
    {
        std::cout << std::format("Test {} passed at depth {} with {} nps.", test_number, depth, nps) << std::endl;
    }
    else
    {
        std::cout << std::format("Test {} *FAILED*. Exp {}, was {}.", test_number, expected_nodes, perft.GetNodes()) << std::endl;
    }
    return nps;
}
template<bool output_perft_paths>
U64 TestBulkPerft(unsigned depth, U64 expected_nodes, U16 test_number, const std::string& fen)
{
    PerftHandler perft;
    Position pos(fen);
    U64 time {1};
    
    {
        Timer<std::chrono::microseconds> t(&time);
        perft.RunBulkPerft<output_perft_paths>(depth, &pos);
    }
    if constexpr(output_perft_paths)
    {
        perft.PrintData();
    }

    const U64 nps = static_cast<double>(perft.GetNodes() * 1'000'000) / static_cast<double>(time);

    if(expected_nodes == perft.GetNodes())
    {
        std::cout << std::format("Test {} passed at depth {} with {} nps.", test_number, depth, nps) << std::endl;
    }
    else
    {
        std::cout << std::format("Test {} *FAILED*. Exp {}, was {}.", test_number, expected_nodes, perft.GetNodes()) << std::endl;
    }
    return nps;
}
template<bool output_perft_paths>
void RunBenchmark()
{
    U64 mean_nps{0};

    mean_nps += TestPerft<output_perft_paths>(6, 119060324, 1, STARTPOS);
    mean_nps += TestPerft<output_perft_paths>(5, 193690690, 2, KIWIPETE);
    mean_nps += TestPerft<output_perft_paths>(7, 178633661, 3, PERFTPOS3);
    mean_nps += TestPerft<output_perft_paths>(6, 706045033, 4, PERFTPOS4);
    mean_nps += TestPerft<output_perft_paths>(5, 89941194,  5, PERFTPOS5);
    mean_nps += TestPerft<output_perft_paths>(5, 164075551, 6, PERFTPOS6);

    std::cout << "All tests completed with means nps: " << (mean_nps / 6) << std::endl;
}
template<bool output_perft_paths>
void RunBulkBenchmark()
{
    U64 mean_nps{0};

    mean_nps += TestBulkPerft<output_perft_paths>(6, 119060324, 1, STARTPOS);
    mean_nps += TestBulkPerft<output_perft_paths>(5, 193690690, 2, KIWIPETE);
    mean_nps += TestBulkPerft<output_perft_paths>(7, 178633661, 3, PERFTPOS3);
    mean_nps += TestBulkPerft<output_perft_paths>(6, 706045033, 4, PERFTPOS4);
    mean_nps += TestBulkPerft<output_perft_paths>(5, 89941194,  5, PERFTPOS5);
    mean_nps += TestBulkPerft<output_perft_paths>(5, 164075551, 6, PERFTPOS6);

    std::cout << "All tests completed with means nps: " << (mean_nps / 6) << std::endl;
}
static std::vector<std::string> Split(const std::string& line, const std::string& delimiter)
{
    std::vector<std::string> tmp{};
    size_t pos = 0;
    std::string s{line};

    while ((pos = s.find(delimiter)) != std::string::npos) 
    {
        tmp.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    return tmp;
}
template<bool output_perft_paths>
bool RunPerftSuite()
{
    std::fstream perft_file("perftsuite.epd", std::ios::in);

    if(!perft_file.is_open()){return false;}

    std::string line;

    uint32_t current_perft{1};

    while(std::getline(perft_file, line))
    {
        if(line[0] == '#') continue;
        
        std::vector<std::string> chunks = Split(line, ";");
        const std::string& fen = chunks[0];
        unsigned depth{999999};
        U64 expected_nodes{0};

        for(int i = chunks.size() - 1; i > 0 ; --i)
        {
            if(std::stoull(chunks[i].substr(3)) > 100'000'000) continue;
            depth = (chunks[i])[1] - '0';
            expected_nodes = std::stoull(chunks[i].substr(3));
            break;
        }
        assert(depth < 7);
        TestPerft<output_perft_paths>(depth, expected_nodes, current_perft, fen);
        ++current_perft;
    }

    perft_file.close();
    return true;
}
#endif // #ifndef TESTING_HPP