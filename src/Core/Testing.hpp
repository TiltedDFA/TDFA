#ifndef TESTING_HPP
#define TESTING_HPP

#include <vector>

#include "BitBoard.hpp"
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


bool RunWhitePawnGenTests()
{
    SET_UP_TESTS;
    pos.ImportFen(TESTFEN1);
    move_generator.WhitePawnMoves(move_list.Current(),pos.pieces_[BB::loc::WHITE][BB::loc::PAWN],Magics::IndexToBB(pos.en_passant_target_sq_));
    return true;
}


#endif // #ifndef TESTING_HPP