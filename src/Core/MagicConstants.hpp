#ifndef MAGICCONSTANTS_HPP
#define MAGICCONSTANTS_HPP

#include "Types.hpp"
#include <array>
#include <bitset>
#include <cmath>
#include <cassert>

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

#ifdef __GNUG__
    constexpr int FindLS1B(BitBoard bb){return __builtin_ctzll(bb);}
#else
    constexpr int FindLS1B(BitBoard bb){return std::countr_zero(bb);}
#endif
    static constexpr double pow(double x, unsigned int y){return (y >= sizeof(unsigned)*8) ? 0 : y == 0 ? 1 : x * pow(x,y-1);}
    static constexpr BitBoard CollapsedFilesIndex(BitBoard b) 
    {
        b |= b >> 32;
        b |= b >> 16;
        b |= b >>  8;
        return b & 0xFF;
    }
    static constexpr BitBoard CollapsedRanksIndex(BitBoard b) 
    {
        b |= b >>  4;
        b |= b >>  2;
        b |= b >>  1;
        b &= FILE_ABB;
        b |= b >>  7;
        b |= b >> 14;
        b |= b >> 28;
        return b & 0xFF;
    }
    //Not a true conversion. Just returns the value of the binary number if it was base 3
    static consteval std::array<std::array<uint16_t,256>,8> compute_base_2_to_3()
    {
        std::array<std::array<uint16_t,256>,8> result{};
        for(uint8_t missed_file = 0; missed_file < 8;++missed_file)
        {
            for(uint16_t i = 0; i < 256;++i)
            {
                for(int j = 0; j < 8;++j)
                {
                    if(j == missed_file) continue;
                    result.at(missed_file).at(i) += ((i>>j)&1) * ((j < missed_file) ? pow(3,j) : pow(3,j-1));
                }
                    assert(result.at(missed_file).at(i) < 1094);
            }
        }
        return result;
    }

    static constexpr std::array<std::array<uint16_t,256>,8> base_2_to_3 = compute_base_2_to_3();

    constexpr int FindMS1B(BitBoard board){return FindLS1B(board) ^ 0x3F;}

    constexpr BitBoard PopLS1B(BitBoard board) {return (board& (board-1));}

    constexpr bool IndexInBounds(int index) {return index > 0 && index < 64;}
    
    constexpr BitBoard IndexToBB(uint8_t index){return 1ull << index;}

    template<uint8_t N>
    consteval BitBoard IndexToBB(){return 1ull << N;}
    // 0 0 0 1 0 0 0 0
    constexpr uint8_t file_of(uint8_t index){return index & 7;}

    constexpr uint8_t rank_of(uint8_t index){return index >> 3;}

    constexpr uint8_t BBFileOf(uint8_t square){return 1 << file_of(square);}

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
    
    static consteval std::array<BitBoard, 64> KnightAttackingMask()
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
    static consteval std::array<std::array<BitBoard,4>,64> PrecomputeMask()
    {
        std::array<std::array<BitBoard,4>,64> r_val{};
        for(uint8_t i = 0; i < 64;++i)
        {
            uint8_t rank = Magics::rank_of(i);
            uint8_t file = Magics::file_of(i);
            BitBoard cross_attacks{0ull};
            BitBoard anti_cross_attacks{0ull};

            for(int8_t r = rank + 1, f = file + 1; r < 8 && f < 8;++r,++f)     cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 + f));
            for(int8_t r = rank - 1, f = file - 1; r >= 0 && f >= 0;--r,--f)   cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 + f));
            for(int8_t r = rank + 1, f = file - 1; r < 8 && f >= 0;++r, --f)   anti_cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 +f));
            for(int8_t r = rank - 1, f = file + 1; r >= 0 && f < 8;--r, ++f)   anti_cross_attacks |= Magics::IndexToBB(static_cast<uint8_t>(r*8 +f));
            
            r_val[i][static_cast<int>(D::FILE)] = (Magics::FILE_ABB << file) & ~Magics::IndexToBB(i); //Rook file attacks
            r_val[i][static_cast<int>(D::RANK)] = (Magics::RANK_1BB << (8*rank)) & ~Magics::IndexToBB(i); // Rook Rank attacks
            r_val[i][static_cast<int>(D::DIAG)] = cross_attacks & ~Magics::IndexToBB(i); //Bishop cross attacks
            r_val[i][static_cast<int>(D::ADIAG)] = anti_cross_attacks & ~Magics::IndexToBB(i); //Bishop anti cross attacks
        }
        return r_val;
    }

    static constexpr std::array<std::array<BitBoard,4>,64> SLIDING_ATTACKS_MASK = PrecomputeMask();
    
    static constexpr std::array<BitBoard, 64> KNIGHT_ATTACK_MASKS = KnightAttackingMask();

    static consteval std::array<BitBoard, 64> KingAttackingMask()
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
    static constexpr std::array<BitBoard, 64> KING_ATTACK_MASKS = KingAttackingMask();
}
#endif //#ifndef MAGICCONSTANTS_HPP