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
    constexpr move_info():encoded_move(), count(0){}
    constexpr void add_move(Move m){encoded_move.at(count) = m;++count;}
    std::array<Move, 7> encoded_move;
    uint16_t count;
};
namespace loc
{
    constexpr uint8_t WHITE = 0;
    constexpr uint8_t BLACK = 1;
    constexpr uint8_t KING  = 0;
    constexpr uint8_t QUEEN = 1;
    constexpr uint8_t BISHOP= 2;
    constexpr uint8_t KNIGHT= 3;
    constexpr uint8_t ROOK  = 4;
    constexpr uint8_t PAWN  = 5; 
}
// This will be specfic class used to decided which direction to test the moves [sq][D::val][index]
enum class D : uint8_t
{
    FILE,
    RANK,
    DIAG,
    ADIAG
};

#endif //#ifndef TYPES_HPP