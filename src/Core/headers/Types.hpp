#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

typedef uint64_t BitBoard;

enum class MoveDirections : uint8_t
{
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST
};


#endif //#ifndef TYPES_HPP