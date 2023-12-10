#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Types.hpp"
#include "Evaluate.hpp"
#include "../MoveGen/MoveGen.hpp"
#include <limits>



class Search
{
public:
    Search();

    Score GoSearch(uint16_t depth);
private:
    MoveGen generator_;
    BB::Position pos_;

};



#endif // #ifndef SEARCH_HPP