#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <array>

#define CONSTEVAL_TIT 0
#define DEVELOPER_MODE 1

using Move      = uint16_t;
using BitBoard  = uint64_t;
using PieceType = uint8_t;
using Sq        = uint8_t;
using Castling  = uint8_t;

constexpr std::size_t MAX_MOVES = 218;

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
    constexpr void add_move(Move m){encoded_move.at(count) = m; ++count;}
    std::array<Move, 7> encoded_move;
    uint16_t count;
};
namespace loc
{
    constexpr uint8_t BLACK = 0;
    constexpr uint8_t WHITE = 1;
    constexpr uint8_t KING  = 0;
    constexpr uint8_t QUEEN = 1;
    constexpr uint8_t BISHOP= 2;
    constexpr uint8_t KNIGHT= 3;
    constexpr uint8_t ROOK  = 4;
    constexpr uint8_t PAWN  = 5; 
}
enum class PromType : uint8_t
{
    NOPROMO,
    QUEEN,
    BISHOP,
    KNIGHT,
    ROOK
};
// This will be specfic class used to decided which direction to test the moves [sq][D::val][index]
enum class D : uint8_t
{
    FILE,
    RANK,
    DIAG,
    ADIAG
};
template<typename T>
float F_Div(T dividend, T divisor)
{
    return static_cast<double>(dividend) / static_cast<double>(divisor);
}
#endif //#ifndef TYPES_HPP