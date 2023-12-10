#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "../MoveGen/MoveGen.hpp"
#include <limits>



class Search
{
public:
    Search()=delete;
    
    Search(BB::Position& pos);



    void SetPos(BB::Position& pos);

    Score GoSearch(uint16_t depth);
private:
    BB::Position& pos_;
    MoveGen generator_;
};



#endif // #ifndef SEARCH_HPP