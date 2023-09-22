#ifndef TESTING_HPP
#define TESTING_HPP

#include <vector>

#include "BitBoard.hpp"
#include "Debug.hpp"
#include "MagicConstants.hpp"
#include "../MoveGen/MoveGen.hpp"
#include "../MoveGen/MoveList.hpp"

constexpr bool CmpMoveLists(MoveList& l1,const std::vector<Move>& l2)
{
    if (l2.size() != l1.Size()) return false;
    for(Move move : l2)
        if(!l1.Contains(move)) return false;
    return true;
}
#define SET_UP_TESTS BB::Position pos{}; MoveGen move_generator{}; MoveList move_list{}
#define CMP_MOVE_LISTS(generated_move_list,expected_move_vector) if(CmpMoveLists((generated_move_list),(expected_move_vector))) std::cout << "Test passed"; \
                                                                else {}
#define SET_UP_TITBOARD_TESTS   BB::Position pos; \
                                MoveGen generator; \
                                move_info info{};
#define TEST_TITBOARD_GEN(piece_to_move_sq, fen, move_info_store,direction) {\
            uint8_t sq = (piece_to_move_sq);\
            pos.ImportFen((fen));\
            uint16_t p1 = Magics::base_2_to_3[Magics::FileOf(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];\
            uint16_t p2 = 2 * Magics::base_2_to_3[Magics::FileOf(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];\
            uint16_t index = p1+p2;\
            std::cout << "sq= "<< (piece_to_move_sq) << " index = " << index << std::endl;\
            info = generator.SLIDING_ATTACK_CONFIG.at(sq).at((direction)).at(p1+p2);}

#define PRINT_TIT_TEST_RESULTS if(info.count == 0) std::cout << "***NO MOVES FOUND***\n\n"; else {for(int i = 0; i < info.count;++i) Debug::ShortPrintEncodedMoveStr(info.encoded_move[i]);std::cout << "\n";}

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

template<D direction>
constexpr void RunTitBoardTest(uint8_t sq,std::string_view fen, move_info& info)
{
    BB::Position pos(fen);
    
    uint16_t p1{0};
    uint16_t p2{0};
    
    const bool us_is_white = Magics::IndexToBB(sq) & pos.GetPieces<true>();
    if(us_is_white)
    {
        p1 = Magics::base_2_to_3    [(direction == D::RANK) ? Magics::FileOf(sq)
                                                            : Magics::RankOf(sq)]
                                    [(direction == D::RANK) ? Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])
                                                            : Magics::CollapsedRanksIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])];

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
                                        [(direction == D::RANK) ? Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])
                                                            : Magics::CollapsedRanksIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][static_cast<uint8_t>(direction)])];
    }
    std::cout << "sq="<< static_cast<int>(sq) << ", index=" << p1+p2 << ", atk_dir=" << ((direction == D::FILE) ? "File" : (direction == D::RANK) ? "Rank" : (direction == D::DIAG) ? "Diagonal" : "Anti-Diagonal") <<  ", fen='" << fen << "'" << std::endl;
   
    // std::cout << "White BB:\n";
    // Debug::PrintBB(pos.GetPieces<true>());
    // std::cout << "Black BB:\n";
    // Debug::PrintBB(pos.GetPieces<false>());
    // std::cout << "Combined BB:\n";
    // Debug::PrintBB(pos.GetPieces<true>() | pos.GetPieces<false>());
    std::cout << "Current Position:\n";
    (us_is_white) ? Debug::PrintUsThem(pos.GetPieces<true>(),pos.GetPieces<false>()) : Debug::PrintUsThem(pos.GetPieces<false>(),pos.GetPieces<true>());

    info = MoveGen::SLIDING_ATTACK_CONFIG.at(sq).at(static_cast<uint8_t>(direction)).at(p1+p2);
}


#endif // #ifndef TESTING_HPP