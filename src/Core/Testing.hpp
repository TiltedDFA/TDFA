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
#include "../MoveGen/MoveGen.hpp"
#include "../MoveGen/MoveList.hpp"
#include "Uci.hpp"

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

#define PERFTPOS2 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - "

template<D direction>
constexpr void RunTitBoardTest(uint8_t sq, std::string_view fen, move_info& info)
{
    BB::Position pos(fen);
    
    uint16_t p1{0};
    uint16_t p2{0};
    
    const bool us_is_white = Magics::IndexToBB(sq) & pos.GetPieces<true>();
    if(us_is_white)
    {
        p1 = Magics::base_2_to_3    [(direction == D::RANK) ? Magics::FileOf(sq)
                                                            : Magics::RankOf(sq)]
                                    [(direction == D::RANK) ? Magics::CollapsedFilesIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])
                                                            : Magics::CollapsedRanksIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])];

        p2 = 2 * Magics::base_2_to_3    [(direction == D::RANK) ? Magics::FileOf(sq)
                                                            : Magics::RankOf(sq)]
                                        [(direction == D::RANK) ? Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])
                                                            : Magics::CollapsedRanksIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])];
    }
    else
    {
        p1 = Magics::base_2_to_3    [(direction == D::RANK) ? Magics::FileOf(sq)
                                                            : Magics::RankOf(sq)]
                                    [(direction == D::RANK) ? Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])
                                                            : Magics::CollapsedRanksIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])];

        p2 = 2 * Magics::base_2_to_3    [(direction == D::RANK) ? Magics::FileOf(sq)
                                                            : Magics::RankOf(sq)]
                                        [(direction == D::RANK) ? Magics::CollapsedFilesIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])
                                                            : Magics::CollapsedRanksIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])];
    }
    std::cout << "sq="<< static_cast<int>(sq) << ", index=" << p1 + p2 << ", atk_dir=" << ((direction == D::FILE) ? "File" : (direction == D::RANK) ? "Rank" : (direction == D::DIAG) ? "Diagonal" : "Anti-Diagonal") <<  ", fen='" << fen << "'" << std::endl;

    std::cout << "Current Position:\n";
    (us_is_white) ? Debug::PrintUsThem(pos.GetPieces<true>(),pos.GetPieces<false>()) : Debug::PrintUsThem(pos.GetPieces<false>(),pos.GetPieces<true>());

    info = MoveGen::SLIDING_ATTACK_CONFIG.at(sq).at(static_cast<uint8_t>(direction)).at(p1 + p2);
}
class PerftHandler
{
public:
    PerftHandler():perft_data_({}),total_nodes_(0){}

    void ResetData(){perft_data_ = std::vector<std::string>{}; total_nodes_ = 0;}

    void RunPerft(int depth, BB::Position& pos){ResetData(); Perft<true>(depth, pos);}

    uint64_t GetNodes() const {return total_nodes_;}

    void PrintData()
    {
        std::cout << std::format("\nTotal: {}\n", total_nodes_);
        std::sort(perft_data_.begin(), perft_data_.end());
        for(const auto& i : perft_data_)
            std::cout << i;
    }
private:
    template<bool is_root>
    uint64_t Perft(int depth, BB::Position& pos)
    {
        if(!depth) return 1ull;

        MoveList ml{};
        uint64_t nodes{0};
        
        MoveGen gen{};

        if(pos.whites_turn_)
            gen.GenerateLegalMoves<true>(pos,ml);
        else
            gen.GenerateLegalMoves<false>(pos,ml);
        
        for(size_t i = 0; i < ml.len(); ++i)
        {
            if constexpr(is_root)
            {
                pos.MakeMove(ml[i]);
                const auto cnt = Perft<false>(depth - 1, pos);
                nodes += cnt;
                total_nodes_ += cnt;
                // PRINTNL("ROOT\n");
                pos.UnmakeMove();
                perft_data_.push_back(std::format("{} : {}\n", UCI::move(ml[i]), cnt));
            } 
            else
            {
                pos.MakeMove(ml[i]);
                nodes += Perft<false>(depth - 1, pos);
                pos.UnmakeMove();
            }
        }
        // ml.print();
        return nodes;
    }

private:
    std::vector<std::string> perft_data_;
    uint64_t total_nodes_;
};

#endif // #ifndef TESTING_HPP