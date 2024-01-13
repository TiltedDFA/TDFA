#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "MoveGen.hpp"
#include <limits>



class Search
{
public:
    Search()=delete;
    
    Search(BB::Position& pos);

    void init(BB::Position& pos);

    void SetPos(BB::Position& pos);

    Score GoSearch(U16 depth, Score a = Eval::NEG_INF, Score b = Eval::POS_INF);


private:
    BB::Position& pos_;
    MoveGen generator_;
};



#endif // #ifndef SEARCH_HPP