#ifndef MAGICCONSTANTS_HPP
#define MAGICCONSTANTS_HPP

#include "Types.hpp"

namespace Magics
{
    constexpr BitBoard FILE_ABB = 0x0101010101010101;
    constexpr BitBoard FILE_BBB = FILE_ABB << 1;
    constexpr BitBoard FILE_CBB = FILE_ABB << 2;
    constexpr BitBoard FILE_DBB = FILE_ABB << 3;
    constexpr BitBoard FILE_EBB = FILE_ABB << 4;
    constexpr BitBoard FILE_FBB = FILE_ABB << 5;
    constexpr BitBoard FILE_GBB = FILE_ABB << 6;
    constexpr BitBoard FILE_HBB = FILE_ABB << 7;

    constexpr BitBoard RANK_1BB = 0x00000000000000FF;
    constexpr BitBoard RANK_2BB = RANK_1BB << 8;
    constexpr BitBoard RANK_3BB = RANK_1BB << 16;
    constexpr BitBoard RANK_4BB = RANK_1BB << 24;
    constexpr BitBoard RANK_5BB = RANK_1BB << 32;
    constexpr BitBoard RANK_6BB = RANK_1BB << 40; 
    constexpr BitBoard RANK_7BB = RANK_1BB << 48;
    constexpr BitBoard RANK_8BB = RANK_1BB << 56;

    template<typename T>
    constexpr T LSBIndex(BitBoard board){return static_cast<T>(__builtin_ctzll(board));}

    template<typename T>
    constexpr T MSBIndex(BitBoard board){return static_cast<T>(__builtin_clzll(board) ^ 0x3F);}

    constexpr BitBoard PopLSB(BitBoard board) {return (board& (board-1));}
    
    template<typename T>
    constexpr BitBoard IndexToBB(T index){return 1ull << index;}

    template<MoveDirections D>
    constexpr BitBoard Shift(BitBoard b)
    {
        return 
            D == MoveDirections::NORTH      ? b                 << 8 :
            D == MoveDirections::SOUTH      ? b                 >> 8 :
            D == MoveDirections::NORTH_EAST ? (b & ~FILE_HBB)   << 9 :
            D == MoveDirections::EAST       ? (b & ~FILE_HBB)   << 1 :
            D == MoveDirections::SOUTH_EAST ? (b & ~FILE_HBB)   >> 7 :
            D == MoveDirections::SOUTH_WEST ? (b & ~FILE_ABB)   >> 9 :
            D == MoveDirections::WEST       ? (b & ~FILE_ABB)   >> 1 :
            D == MoveDirections::NORTH_WEST ? (b & ~FILE_ABB)   << 9 :
            0;
    }
}
#endif //#ifndef MAGICCONSTANTS_HPP