#ifndef MOVELIST_HPP
#define MOVELIST_HPP

#include "../Core/Types.hpp"
#include "../Core/Move.hpp"
#include <algorithm>
#include <limits>

template<typename T>
constexpr void __FILL(T* begin, std::size_t size, T value)
{
    do { *begin++ = value; }
    while (--size);
}

class MoveList
{
public:
    explicit constexpr MoveList() :head(), tail(&head[0]) { __FILL<Move>(head, MAX_MOVES, std::numeric_limits<Move>::max()); }
    constexpr Move* First()     {return &head[0];}
    constexpr Move* Last()      {return tail;}
    constexpr Move** Current()  {return &tail;}
    constexpr std::size_t Size()const {return tail - head;}
    constexpr bool Contains(Move move){return std::find(First(),Last(),move) != Last();}
private:
    Move head[MAX_MOVES], *tail;
};



#endif // #ifndef MOVELIST_HPP