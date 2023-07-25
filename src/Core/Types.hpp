#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <array>

using Move      = uint16_t;
using BitBoard  = uint64_t;
using PieceType = uint32_t;

constexpr std::size_t MAX_MOVES = 256;

enum class MD : uint8_t
{
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST,
    NORTHNORTH,
    SOUTHSOUTH
};

struct move_info
{
    std::array<Move, 7> encoded_move;
    uint16_t count;
};
#endif //#ifndef TYPES_HPP