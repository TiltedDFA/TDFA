#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "MoveGen.hpp"
#include <limits>



class Search
{
#if DEVELOPER_MODE == 1
public:
    static inline U64 nodes_{0};
#endif
public:
    Search()=delete;
    
    Search(BB::Position& pos);

    void SetPos(BB::Position& pos);

    Score GoSearch(U16 depth, Score a, Score b);
private:
    BB::Position& pos_;
    MoveGen generator_;
};



#endif // #ifndef SEARCH_HPP