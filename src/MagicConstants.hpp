#ifndef MAGICCONSTANTS_HPP
#define MAGICCONSTANTS_HPP

#include "Types.hpp"
#include "ZobristConstants.hpp"
#include <array>
#include <bitset>
#include <cmath>
#include <cassert>

namespace Magics
{
    //forced compile time eval version of the other SqToBB
    template<Sq N>
    consteval BitBoard SqToBB()  {return 1ull << N;}
    
    //returns the file of an index/square
    constexpr Sq FileOf(U8 index)  {return index & 7;}

    constexpr U16 EncodeKing(Sq start, Sq target) {return start | (target << 6);}

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

    constexpr U8 EP_NULL = 255;

    constexpr BitBoard CROSS_DIAG = 0x8040201008040201;         // A1 - H8
    constexpr BitBoard ANTI_CROSS_DIAG = 0x0102040810204080;    // A8 - H1

    constexpr BitBoard ROOK_START_SQS = (SqToBB<0>()  | SqToBB<7>() |
                                         SqToBB<56>() | SqToBB<63>());
    
    constexpr U8 CASTLE_K_W = 0x08;
    constexpr U8 CASTLE_Q_W = 0x04;
    constexpr U8 CASTLE_K_B = 0x02;
    constexpr U8 CASTLE_Q_B = 0x01;

    constexpr U8 CASTLE_NOT_K_W = ~CASTLE_K_W & 0x0F;
    constexpr U8 CASTLE_NOT_Q_W = ~CASTLE_Q_W & 0x0F;
    constexpr U8 CASTLE_NOT_K_B = ~CASTLE_K_B & 0x0F;
    constexpr U8 CASTLE_NOT_Q_B = ~CASTLE_Q_B & 0x0F;

    constexpr U8 CASTLE_ALL = 0x0F;
    //preserve black's right to castle
    constexpr U8 NO_CASTLE_W = (CASTLE_K_B | CASTLE_Q_B);
    constexpr U8 NO_CASTLE_B = (CASTLE_K_W | CASTLE_Q_W);

    constexpr BitBoard ROOK_TO_FROM_W_Q = SqToBB<0>() | SqToBB<3>();
    constexpr BitBoard ROOK_TO_FROM_W_K = SqToBB<5>() | SqToBB<7>();
    constexpr BitBoard ROOK_TO_FROM_B_Q = SqToBB<56>()| SqToBB<59>();
    constexpr BitBoard ROOK_TO_FROM_B_K = SqToBB<61>()| SqToBB<63>();

    constexpr BitBoard ROOK_TO_FROM_ARR_BB[5] =
        {0, ROOK_TO_FROM_W_Q, ROOK_TO_FROM_W_K, ROOK_TO_FROM_B_Q, ROOK_TO_FROM_B_K};
    enum
    {
        CASTLING_WHITE_QUEENSIDE = 1,
        CASTLING_WHITE_KINGSIDE = 2,
        CASTLING_BLACK_QUEENSIDE = 3,
        CASTLING_BLACK_KINGSIDE = 4
    };
    constexpr ZobristKey CASTLING_ZOB_KEYS[5] =
    {
        0,
        (Zobrist::PIECES[true][pt_Rook][0]   ^ Zobrist::PIECES[true][pt_Rook][3]   ^ Zobrist::PIECES[true][pt_King][4]   ^ Zobrist::PIECES[true][pt_King][2]  ),
        (Zobrist::PIECES[true][pt_Rook][5]   ^ Zobrist::PIECES[true][pt_Rook][7]   ^ Zobrist::PIECES[true][pt_King][4]   ^ Zobrist::PIECES[true][pt_King][6]  ),
        (Zobrist::PIECES[false][pt_Rook][56] ^ Zobrist::PIECES[false][pt_Rook][59] ^ Zobrist::PIECES[false][pt_King][60] ^ Zobrist::PIECES[false][pt_King][58]),
        (Zobrist::PIECES[false][pt_Rook][61] ^ Zobrist::PIECES[false][pt_Rook][63] ^ Zobrist::PIECES[false][pt_King][60] ^ Zobrist::PIECES[false][pt_King][62])
    };
    constexpr BitBoard GetLS1B(BitBoard bb)  {return bb & -bb;}

#ifdef __GNUG__
    //finds least significant 1 bit and returns the position of it
    constexpr Sq FindLS1B(BitBoard bb)  {return (bb ? __builtin_ctzll(bb) : 0);}
    //returns the number of 1 bits in the board
    constexpr U8 PopCnt(BitBoard bb)  {return __builtin_popcountll(bb);}
#else
    //finds least significant 1 bit and returns the position of it
    constexpr Sq FindLS1B(BitBoard bb)   {return std::countr_zero(bb);}
    //returns the number of 1 bits in the board
    constexpr uint8_t PopCnt(BitBoard bb)  {return std::popcount(bb);}
#endif
    //returns (x^y). compile time friendly.
    constexpr double pow(double x, unsigned int y)  {return (y >= sizeof(unsigned)*8) ? 0 : y == 0 ? 1 : x * pow(x,y-1);}
    //returns an 8 bit number. the 1 bits in the number show that the corresponding file has at least one occupying piece.
    //Returns the index of the most significant 1 bit.
    constexpr Sq FindMS1B(BitBoard board)  {return FindLS1B(board) ^ 0x3F;}

    //Returns the number without the least significant 1 bit.
    //Not protected against 0 inputs
    constexpr BitBoard PopLS1B(BitBoard board)  {return (board & (board - 1));}

    constexpr Sq PopNRetLS1B(BitBoard& board) 
    {
        const Sq lsb = FindLS1B(board);
        board &= board -1;
        return lsb;
    }

    //Returns whether the index provided is in bounds of the board
    constexpr bool ValidSq(int index)  {return index >= 0 && index < 64;}
    
    //Returns the a bitboard with a 1 bit in the location of the index provided
    constexpr BitBoard SqToBB(Sq index) {assert(ValidSq(index));return 1ull << index;}

    [[nodiscard, gnu::always_inline]]
    constexpr PieceType TypeOf(Piece p)
    {
        return PieceType(p & 0b0111);
    }
    [[nodiscard, gnu::always_inline]]
    constexpr Colour ColourOf(Piece p)
    {
        assert(p != p_None);
        return Colour(p & 0b1000);
    }
    //returns the rank of an index/square
    constexpr Sq RankOf(Sq index) {assert(ValidSq(index));return index >> 3;}

    //finds the file of the square/index and returns a bitboard containing a 1 bit
    // in the square specified
    constexpr U8 BBFileOf(Sq square)  {return 1 << FileOf(square);}

    constexpr U8 BBRankOf(Sq square)  {return 1 << RankOf(square);}
    
    constexpr U8 CollapsedFilesIndex(BitBoard b) 
    {
        return (b * FILE_ABB) >> 56;
    }
    //returns an 8 bit number. the 1 bits in the number show that the corresponding rank has at least one occupying piece.
    constexpr U8 CollapsedRanksIndex(BitBoard b) 
    {
        //collapses the bb into a file
        //then performs collapsed files
        b |= b >> 4;
        b |= b >> 2;
        b |= b >> 1;
        return ((b & FILE_ABB) * ANTI_CROSS_DIAG) >> 56;
    }
    constexpr U8 CollapsedRanksIndex(BitBoard b, U8 file) 
    {
        assert(file < 8);
        // return (((b >> file) & FILE_ABB) * ANTI_CROSS_DIAG) >> 56;
        return ((b >> file) * ANTI_CROSS_DIAG) >> 56;
    }
    constexpr BitBoard PopMS1B(const BitBoard board) 
    {
        BitBoard b = board;
        b |= b >> 1;
        b |= b >> 2;
        b |= b >> 4;
        b |= b >> 8;
        b |= b >> 16;
        b |= b >> 32;
        return board & (b >> 1);
    }
    //Returns a bitboard which has been moved by the shift specified
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
        constexpr BitBoard knight_attack_template = SqToBB<1>() | SqToBB<3>() | SqToBB<8>() | SqToBB<12>() |
                                                    SqToBB<24>()| SqToBB<28>()| SqToBB<33>()| SqToBB<35>() ;
        for(U8 i{0}; i < 64;++i)
        {
            if(i % 8 < 2)     temp_array[i] = ((i < 18) ? (knight_attack_template  >> (18 - i)) : (knight_attack_template  << (i - 18)))
                                            & (~Magics::FILE_GBB & ~FILE_HBB);
            else if(i % 8 > 5)temp_array[i] = ((i < 18) ? (knight_attack_template  >> (18 - i)) : (knight_attack_template  << (i - 18)))
                                            & (~Magics::FILE_ABB & ~Magics::FILE_BBB);
            else temp_array[i] = ((i < 18) ? (knight_attack_template  >> (18 - i)) : (knight_attack_template  << (i - 18)));
        }
        return temp_array;
    }
    //Not a true conversion. Just returns the value of the binary number if it was base 3.
    //This omits the file of piece that you're trying to calculate the moves for
    template<bool gen_us>
    consteval std::array<std::array<U16, 256>, 8> compute_base_2_to_3() 
    {
        std::array<std::array<U16, 256>, 8> result{};
        for(U8 missed_file = 0; missed_file < 8; ++missed_file)
        {
            for(U16 i = 0; i < 256; ++i)
            {
                for(int j = 0; j < 8; ++j)
                {
                    if(j == missed_file) continue;
                    
                    if constexpr(gen_us)
                    {
                        result.at(missed_file).at(i) += ((i >> j) & 1) * ((j < missed_file) ? pow(3, j) : pow(3, j - 1));
                    }
                    else
                    {
                        result.at(missed_file).at(i) += ((i >> j) & 1) * ((j < missed_file) ? (2 * pow(3, j)) : (2 * pow(3, j - 1)));
                    }
                }
            }
        }
        return result;
    }
    //finds the attacking masks for sliding pieces. This omits the square of the attacking piece.
    consteval std::array<std::array<BitBoard, 4> ,64> PrecomputeMask() 
    {
        std::array<std::array<BitBoard,4>,64> r_val{};
        for(U8 i = 0; i < 64; ++i)
        {
            U8 rank = Magics::RankOf(i);
            U8 file = Magics::FileOf(i);
            BitBoard cross_attacks{0ull};
            BitBoard anti_cross_attacks{0ull};

            for(int8_t r = rank + 1, f = file + 1; r < 8 && f < 8; ++r, ++f)    cross_attacks       |= Magics::SqToBB(U8(r * 8 + f));
            for(int8_t r = rank - 1, f = file - 1; r >= 0 && f >= 0; --r, --f)  cross_attacks       |= Magics::SqToBB(U8(r * 8 + f));
            for(int8_t r = rank + 1, f = file - 1; r < 8 && f >= 0; ++r, --f)   anti_cross_attacks  |= Magics::SqToBB(U8(r * 8 +f));
            for(int8_t r = rank - 1, f = file + 1; r >= 0 && f < 8; --r, ++f)   anti_cross_attacks  |= Magics::SqToBB(U8(r * 8 +f));
            
            r_val[i][File] = (Magics::FILE_ABB << file) & ~Magics::SqToBB(i); //Rook file attacks
            r_val[i][Rank] = (Magics::RANK_1BB << (8 * rank)) & ~Magics::SqToBB(i); // Rook Rank attacks
            r_val[i][Diagonal] = cross_attacks & ~Magics::SqToBB(i); //Bishop cross attacks
            r_val[i][AntiDiagonal] = anti_cross_attacks & ~Magics::SqToBB(i); //Bishop anti cross attacks
        }
        return r_val;
    }
    
    consteval std::array<BitBoard, 64> KingAttackingMask() 
    {
        std::array<BitBoard, 64> temp_array{};
        constexpr BitBoard king_attack_template =   SqToBB<0>() | SqToBB<1>() | SqToBB<2>() |
                                                    SqToBB<8>() | SqToBB<10>()|
                                                    SqToBB<16>()| SqToBB<17>()| SqToBB<18>();
        for(U8 i{0}; i < 64; ++i)
        {
            if(i % 8 < 1)     temp_array[i] = ((i < 9) ? (king_attack_template  >> (9 - i)) : (king_attack_template  << (i - 9)))
                                            & ~FILE_HBB;
            else if(i % 8 > 6) temp_array[i] = ((i < 9) ? (king_attack_template  >> (9 - i)) : (king_attack_template  << (i - 9)))
                                            & ~FILE_ABB;
            else temp_array[i] = ((i < 9) ? (king_attack_template  >> (9 - i)) : (king_attack_template  << (i - 9)));
        }
        return temp_array;
    }
    //Not a true conversion. Just returns the value of the binary number if it was base 3.
    //This omits the file of piece that you're trying to calculate the moves for
    inline constexpr std::array<std::array<U16, 256>, 8> base_2_to_3_us = compute_base_2_to_3<true>();

    inline constexpr std::array<std::array<U16, 256>, 8> base_2_to_3_them = compute_base_2_to_3<false>();

    inline constexpr U16 GetBaseThreeUsThem(U8 us, U8 them, Sq piece_square) 
    {
        return base_2_to_3_us[piece_square][us] + base_2_to_3_them[piece_square][them];
    }

    //finds the attacking masks for sliding pieces. This omits the square of the attacking piece.
    inline constexpr std::array<std::array<BitBoard, 4>, 64> SLIDING_ATTACKS_MASK = PrecomputeMask();
    
    inline constexpr std::array<BitBoard, 64> KNIGHT_ATTACK_MASKS = KnightAttackingMask();

    inline constexpr std::array<BitBoard, 64> KING_ATTACK_MASKS = KingAttackingMask();
}
#endif //#ifndef MAGICCONSTANTS_HPP
