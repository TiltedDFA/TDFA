#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <array>

#define CONSTEVAL_TIT 0
#define DEVELOPER_MODE 0

using U8  = unsigned char;
using U16 = unsigned short;
using U32 = unsigned int;
using U64 = unsigned long long;

using Move      = U16;
using BitBoard  = U64;
using PieceType = U8;
using Sq        = U8;
using Castling  = U8;

constexpr std::size_t MAX_MOVES = 218;

enum class MD : U8
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
    constexpr void add_move(Move m){encoded_move.at(count++) = m;}
    std::array<Move, 7> encoded_move;
    U8 count;
};
namespace loc
{
    constexpr U8 BLACK = 0;
    constexpr U8 WHITE = 1;
    constexpr U8 KING  = 0;
    constexpr U8 QUEEN = 1;
    constexpr U8 BISHOP= 2;
    constexpr U8 KNIGHT= 3;
    constexpr U8 ROOK  = 4;
    constexpr U8 PAWN  = 5; 
}
enum class PromType : U8
{
    NOPROMO,
    QUEEN,
    BISHOP,
    KNIGHT,
    ROOK
};
// This will be specfic class used to decided which direction to test the moves [sq][D::val][index]
enum class D : U8
{
    FILE,
    RANK,
    DIAG,
    ADIAG
};
template<typename T>
float FloatDiv(T dividend, T divisor)
{
    return static_cast<double>(dividend) / static_cast<double>(divisor);
}
#endif //#ifndef TYPES_HPP