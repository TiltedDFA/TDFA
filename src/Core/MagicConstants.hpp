#ifndef MAGICCONSTANTS_HPP
#define MAGICCONSTANTS_HPP

#include "Types.hpp"
#include <array>
#include <bitset>
#include <cmath>
namespace Magics
{
    constexpr BitBoard FILE_ABB = 0x01'01'01'01'01'01'01'01;
    constexpr BitBoard FILE_BBB = FILE_ABB << 1;
    constexpr BitBoard FILE_CBB = FILE_ABB << 2;
    constexpr BitBoard FILE_DBB = FILE_ABB << 3;
    constexpr BitBoard FILE_EBB = FILE_ABB << 4;
    constexpr BitBoard FILE_FBB = FILE_ABB << 5;
    constexpr BitBoard FILE_GBB = FILE_ABB << 6;
    constexpr BitBoard FILE_HBB = FILE_ABB << 7;

    constexpr BitBoard RANK_1BB = 0x00'00'00'00'00'00'00'FF;
    constexpr BitBoard RANK_2BB = RANK_1BB << 8;
    constexpr BitBoard RANK_3BB = RANK_1BB << 16;
    constexpr BitBoard RANK_4BB = RANK_1BB << 24;
    constexpr BitBoard RANK_5BB = RANK_1BB << 32;
    constexpr BitBoard RANK_6BB = RANK_1BB << 40; 
    constexpr BitBoard RANK_7BB = RANK_1BB << 48;
    constexpr BitBoard RANK_8BB = RANK_1BB << 56;

    constexpr BitBoard CROSS_DIAG = 0x8040201008040201;         // A1 - H8
    constexpr BitBoard ANTI_CROSS_DIAG = 0x0102040810204080;    // A8 - H1

    constexpr BitBoard GetLS1B(BitBoard bb){return bb & -bb;}
//#ifdef __GNUG__
#if 0
    constexpr int FindLS1B(BitBoard bb){return __builtin_ctzll(bb);}
#else
    constexpr int FindLS1B(BitBoard bb)
    {
        return std::countr_zero(bb);
    }
#endif
    constexpr BitBoard CollapsedFilesIndex(BitBoard b) 
    {
        b |= b >> 32;
        b |= b >> 16;
        b |= b >>  8;
        return b & 0xFF;
    }
    //Not a true conversion. Just returns the value of the binary number if it was base 3
    static consteval std::array<uint16_t,256> compute_base_2_to_3()
    {
        std::array<uint16_t,256> result{};
        for(uint16_t i = 0; i < 256;++i)
            for(int j = 0; j < 8;++j)
                result.at(i) += ((i>>j)%2) * std::pow(3,j);
        return result;
    }

    static constexpr std::array<uint16_t,256> base_2_to_3 = compute_base_2_to_3();

    constexpr int FindMS1B(BitBoard board){return FindLS1B(board) ^ 0x3F;}

    constexpr BitBoard PopLSB(BitBoard board) {return (board& (board-1));}
    
    constexpr BitBoard IndexToBB(uint8_t index){return 1ull << index;}

    template<uint8_t N>
    consteval BitBoard IndexToBB(){return 1ull << N;}

    constexpr uint8_t file_of(uint8_t index){return index & 7;}

    constexpr uint8_t rank_of(uint8_t index){return index >> 3;}

    template<MD D>
    constexpr BitBoard Shift(BitBoard b)
    {
        return 
            D == MD::NORTH      ? b                 << 8 :
            D == MD::SOUTH      ? b                 >> 8 :
            D == MD::NORTH_EAST ? (b & ~FILE_HBB)   << 9 :
            D == MD::EAST       ? (b & ~FILE_HBB)   << 1 :
            D == MD::SOUTH_EAST ? (b & ~FILE_HBB)   >> 7 :
            D == MD::SOUTH_WEST ? (b & ~FILE_ABB)   >> 9 :
            D == MD::WEST       ? (b & ~FILE_ABB)   >> 1 :
            D == MD::NORTH_WEST ? (b & ~FILE_ABB)   << 7 :
            D == MD::NORTHNORTH ? b                 << 16:
            D == MD::SOUTHSOUTH ? b                 >> 16:
            0;
    }
    
    consteval std::array<BitBoard, 64> KnightAttackingMask()
    {
        std::array<BitBoard, 64> temp_array{};
        constexpr BitBoard knight_attack_template = IndexToBB<0>() | IndexToBB<3>() | IndexToBB<8>() | IndexToBB<12>() |
                                                    IndexToBB<24>()| IndexToBB<28>()| IndexToBB<33>()| IndexToBB<35>() ;
        for(uint8_t i{0}; i < 64;++i)
        {
            if(i%8 < 2)     temp_array[i] = ((i < 18) ? (knight_attack_template  >> (18-i)) : (knight_attack_template  << (i-18)))
                                            & (~Magics::FILE_GBB & ~FILE_HBB);
            else if(i%8 > 5)temp_array[i] = ((i < 18) ? (knight_attack_template  >> (18-i)) : (knight_attack_template  << (i-18)))
                                            & (~Magics::FILE_ABB & ~Magics::FILE_BBB);
            else temp_array[i] = ((i < 18) ? (knight_attack_template  >> (18-i)) : (knight_attack_template  << (i-18)));
        }
        return temp_array;
    }

    constexpr std::array<BitBoard, 64> KNIGHT_ATTACK_MASKS = KnightAttackingMask();

    consteval std::array<BitBoard, 64> KingAttackingMask()
    {
        std::array<BitBoard, 64> temp_array{};
        constexpr BitBoard king_attack_template =   IndexToBB<0>() | IndexToBB<1>() | IndexToBB<2>() |
                                                    IndexToBB<8>() | IndexToBB<10>()|
                                                    IndexToBB<16>()| IndexToBB<17>()| IndexToBB<18>();
        for(uint8_t i{0}; i < 64;++i)
        {
            if(i%8 < 1)     temp_array[i] = ((i < 9) ? (king_attack_template  >> (9-i)) : (king_attack_template  << (i-9)))
                                            & ~FILE_HBB;
            else if(i%8 > 6)temp_array[i] = ((i < 9) ? (king_attack_template  >> (9-i)) : (king_attack_template  << (i-9)))
                                            & ~FILE_ABB;
            else temp_array[i] = ((i < 9) ? (king_attack_template  >> (9-i)) : (king_attack_template  << (i-9)));
        }
        return temp_array;
    }
    constexpr std::array<BitBoard, 64> KING_ATTACK_MASKS = KingAttackingMask();
}
#endif //#ifndef MAGICCONSTANTS_HPP