#ifndef TYPES_HPP
#define TYPES_HPP


#define USE_TITBOARDS 1
#define USE_TRANSPOSITION_TABLE 1
#define DEBUG_TRANPOSITION_TABLE 0
#define DEVELOPER_MODE 1

#if DEVELOPER_MODE != 1
#define NDEBUG
#endif


#ifdef __GNUG__
#define INLINE __attribute__((always_inline))
#else
#define INLINE
#endif

#include <array>
#include <cstdint>

using U8  = unsigned char;
using U16 = unsigned short;
using U32 = unsigned int;
using U64 = unsigned long long;
using I16 = short;

using Move      = U16;
using BitBoard  = U64;
using PieceType = U8;
using Sq        = U8;
using Castling  = U8;
using Score     = I16;

constexpr std::size_t MAX_MOVES = 218;

enum MD : U8
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
    constexpr move_info(): encoded_move_(), count_(0), attacks_(0ull){}
    constexpr void add_move(Move m) noexcept {encoded_move_.at(count_++) = m;}

    std::array<Move, 7> encoded_move_;
    U8 count_;
    BitBoard attacks_;
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
enum AttackDirection : U8
{
    File,
    Rank,
    Diagonal,
    AntiDiagonal
};
enum class BoundType : U8
{
    EXACT_VAL,  //Score is X
    ALPHA,      //Score is at max X
    BETA        //Score is at least X
};
template<typename T>
float FloatDiv(T dividend, T divisor)
{
    return static_cast<double>(dividend) / static_cast<double>(divisor);
}
#endif //#ifndef TYPES_HPP