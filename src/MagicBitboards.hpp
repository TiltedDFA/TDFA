#ifndef MAGICBITBOARDS_HPP
#define MAGICBITBOARDS_HPP
#include "Types.hpp"

namespace MagicBitBoards {

namespace internal {
    
    // private stuff
    enum class __File : U8
    {
        A = 0,
        B = 1,
        C = 2,
        D = 3,
        E = 4,
        F = 5,
        G = 6,
        H = 7
    };
    enum class __Rank : U8
    {
        RANK_1 = 0,
        RANK_2 = 1,
        RANK_3 = 2,
        RANK_4 = 3,
        RANK_5 = 4,
        RANK_6 = 5,
        RANK_7 = 6,
        RANK_8 = 7
    };
    enum class Color : int8_t
    {
        WHITE = 0,
        BLACK = 1,
        NONE = -1
    };

    inline U64 pdep(U64 val, U64 mask) 
    {
        U64 res = 0;
        for (U64 bb = 1; mask; bb += bb) 
        {
            if (val & bb)
                res |= mask & -mask;
            mask &= mask - 1;
        }
        return res;
    }
    inline U64 shiftRight(U64 bb) 
    {
        return (bb << 1ULL) & 0xfefefefefefefefeULL;
    }
    inline U64 shiftLeft(U64 bb) 
    {
        return (bb >> 1ULL) & 0x7f7f7f7f7f7f7f7fULL;
    }
    inline U64 shiftUp(U64 bb) { return bb << 8ULL; }

    inline U64 shiftDown(U64 bb) { return bb >> 8ULL; } 

    inline __Rank squareRank(Sq square) { return (__Rank)(square / 8); }

    inline __File squareFile(Sq square) { return (__File)(square % 8); }

    extern U64 pawnAttacks[2][64];
    extern U64 knightAttacks[64];
    extern U64 kingAttacks[64];

    inline U64 pawnAttacksSlow(Sq sq, Color color)
    {
        const int Sq_DIAGONAL_LEFT  = sq + (color == Color::WHITE ? 7 : -9),
                  Sq_DIAGONAL_RIGHT = sq + (color == Color::WHITE ? 9 : -7);

        __Rank rank = squareRank(sq);
        if ((color == Color::WHITE && rank == __Rank::RANK_8) || (color == Color::BLACK && rank == __Rank::RANK_1))
            return 0ULL;

        __File file = squareFile(sq);

        if (file == __File::A)
            return 1ULL << Sq_DIAGONAL_RIGHT;

        if (file == __File::H)
            return 1ULL << Sq_DIAGONAL_LEFT;

        return (1ULL << Sq_DIAGONAL_LEFT) | (1ULL << Sq_DIAGONAL_RIGHT);
    }

    inline U64 bishopAttacksSlow(Sq sq, U64 occupied, bool excludeLastSqEachDirection = false)
    {
        U64 attacks = 0ULL;
        int r, f;
        int br = sq / 8;
        int bf = sq % 8;

        for (r = br + 1, f = bf + 1; r >= 0 && r <= 7 && f >= 0 && f <= 7; r++, f++)
        {
            if (excludeLastSqEachDirection && (f == 7 || r == 7))
                break;
            Sq s = r * 8 + f;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        for (r = br - 1, f = bf + 1; r >= 0 && r <= 7 && f >= 0 && f <= 7; r--, f++)
        {
            if (excludeLastSqEachDirection && (r == 0 || f == 7))
                break;
            Sq s = r * 8 + f;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        for (r = br + 1, f = bf - 1; r >= 0 && r <= 7 && f >= 0 && f <= 7; r++, f--)
        {
            if (excludeLastSqEachDirection && (r == 7 || f == 0))
                break;
            Sq s = r * 8 + f;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        for (r = br - 1, f = bf - 1; r >= 0 && r <= 7 && f >= 0 && f <= 7; r--, f--)
        {
            if (excludeLastSqEachDirection && (r == 0 || f == 0))
                break;
            Sq s = r * 8 + f;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        return attacks;
    }

    inline U64 rookAttacksSlow(Sq sq, U64 occupied, bool excludeLastSqEachDirection = false)
    {
        U64 attacks = 0ULL;
        int r, f;
        int rr = sq / 8;
        int rf = sq % 8;

        for (r = rr + 1; r >= 0 && r <= 7; r++)
        {
            if (excludeLastSqEachDirection && r == 7)
                break;
            Sq s = r * 8 + rf;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        for (r = rr - 1; r >= 0 && r <= 7; r--)
        {
            if (excludeLastSqEachDirection && r == 0)
                break;
            Sq s = r * 8 + rf;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        for (f = rf + 1; f >= 0 && f <= 7; f++)
        {
            if (excludeLastSqEachDirection && f == 7)
                break;
            Sq s = rr * 8 + f;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        for (f = rf - 1; f >= 0 && f <= 7; f--)
        {
            if (excludeLastSqEachDirection && f == 0)
                break;
            Sq s = rr * 8 + f;
            attacks |= (1ULL << s);
            if (occupied & (1ULL << s))
                break;
        }

        return attacks;
    }

    const U64 BISHOP_SHIFTS[64] = {
        58, 59, 59, 59, 59, 59, 59, 58, 
        59, 59, 59, 59, 59, 59, 59, 59, 
        59, 59, 57, 57, 57, 57, 59, 59, 
        59, 59, 57, 55, 55, 57, 59, 59, 
        59, 59, 57, 55, 55, 57, 59, 59, 
        59, 59, 57, 57, 57, 57, 59, 59, 
        59, 59, 59, 59, 59, 59, 59, 59, 
        58, 59, 59, 59, 59, 59, 59, 58
    };

    const U64 ROOK_SHIFTS[64] = {
        52, 53, 53, 53, 53, 53, 53, 52, 
        53, 54, 54, 54, 54, 54, 54, 53, 
        53, 54, 54, 54, 54, 54, 54, 53, 
        53, 54, 54, 54, 54, 54, 54, 53, 
        53, 54, 54, 54, 54, 54, 54, 53, 
        53, 54, 54, 54, 54, 54, 54, 53, 
        53, 54, 54, 54, 54, 54, 54, 53, 
        52, 53, 53, 53, 53, 53, 53, 52
    };

    const U64 BISHOP_MAGICS[64] = {
        0x89a1121896040240ULL, 0x2004844802002010ULL, 0x2068080051921000ULL, 0x62880a0220200808ULL,
        0x4042004000000ULL, 0x100822020200011ULL, 0xc00444222012000aULL, 0x28808801216001ULL,
        0x400492088408100ULL, 0x201c401040c0084ULL, 0x840800910a0010ULL, 0x82080240060ULL,
        0x2000840504006000ULL, 0x30010c4108405004ULL, 0x1008005410080802ULL, 0x8144042209100900ULL,
        0x208081020014400ULL, 0x4800201208ca00ULL, 0xf18140408012008ULL, 0x1004002802102001ULL,
        0x841000820080811ULL, 0x40200200a42008ULL, 0x800054042000ULL, 0x88010400410c9000ULL,
        0x520040470104290ULL, 0x1004040051500081ULL, 0x2002081833080021ULL, 0x400c00c010142ULL,
        0x941408200c002000ULL, 0x658810000806011ULL, 0x188071040440a00ULL, 0x4800404002011c00ULL,
        0x104442040404200ULL, 0x511080202091021ULL, 0x4022401120400ULL, 0x80c0040400080120ULL,
        0x8040010040820802ULL, 0x480810700020090ULL, 0x102008e00040242ULL, 0x809005202050100ULL,
        0x8002024220104080ULL, 0x431008804142000ULL, 0x19001802081400ULL, 0x200014208040080ULL,
        0x3308082008200100ULL, 0x41010500040c020ULL, 0x4012020c04210308ULL, 0x208220a202004080ULL,
        0x111040120082000ULL, 0x6803040141280a00ULL, 0x2101004202410000ULL, 0x8200000041108022ULL,
        0x21082088000ULL, 0x2410204010040ULL, 0x40100400809000ULL, 0x822088220820214ULL,
        0x40808090012004ULL, 0x910224040218c9ULL, 0x402814422015008ULL, 0x90014004842410ULL,
        0x1000042304105ULL, 0x10008830412a00ULL, 0x2520081090008908ULL, 0x40102000a0a60140ULL,
    };

    const U64 ROOK_MAGICS[64] = {
        0xa8002c000108020ULL, 0x6c00049b0002001ULL, 0x100200010090040ULL, 0x2480041000800801ULL, 
        0x280028004000800ULL, 0x900410008040022ULL, 0x280020001001080ULL, 0x2880002041000080ULL,
        0xa000800080400034ULL, 0x4808020004000ULL, 0x2290802004801000ULL, 0x411000d00100020ULL,
        0x402800800040080ULL, 0xb000401004208ULL, 0x2409000100040200ULL, 0x1002100004082ULL,
        0x22878001e24000ULL, 0x1090810021004010ULL, 0x801030040200012ULL, 0x500808008001000ULL,
        0xa08018014000880ULL, 0x8000808004000200ULL, 0x201008080010200ULL, 0x801020000441091ULL,
        0x800080204005ULL, 0x1040200040100048ULL, 0x120200402082ULL, 0xd14880480100080ULL,
        0x12040280080080ULL, 0x100040080020080ULL, 0x9020010080800200ULL, 0x813241200148449ULL,
        0x491604001800080ULL, 0x100401000402001ULL, 0x4820010021001040ULL, 0x400402202000812ULL,
        0x209009005000802ULL, 0x810800601800400ULL, 0x4301083214000150ULL, 0x204026458e001401ULL,
        0x40204000808000ULL, 0x8001008040010020ULL, 0x8410820820420010ULL, 0x1003001000090020ULL,
        0x804040008008080ULL, 0x12000810020004ULL, 0x1000100200040208ULL, 0x430000a044020001ULL,
        0x280009023410300ULL, 0xe0100040002240ULL, 0x200100401700ULL, 0x2244100408008080ULL,
        0x8000400801980ULL, 0x2000810040200ULL, 0x8010100228810400ULL, 0x2000009044210200ULL,
        0x4080008040102101ULL, 0x40002080411d01ULL, 0x2005524060000901ULL, 0x502001008400422ULL,
        0x489a000810200402ULL, 0x1004400080a13ULL, 0x4000011008020084ULL, 0x26002114058042ULL,
    };

    extern U64 bishopAttacksEmptyBoard[64];
    extern U64 rookAttacksEmptyBoard[64];
    extern U64 bishopAttacksTable[64][1ULL << 9ULL]; 
    extern U64 rookAttacksTable[64][1ULL << 12ULL];

}

inline void init()
{
    using namespace internal;

    // Init pawn, king and knight attacks
    U64 king = 1;
    for (int sq = 0; sq < 64; sq++)
    {
        // Pawn
        pawnAttacks[0][sq] = pawnAttacksSlow(sq, Color::WHITE);
        pawnAttacks[1][sq] = pawnAttacksSlow(sq, Color::BLACK);

        // Knight
        U64 n = 1ULL << sq;
        U64 h1 = ((n >> 1ULL) & 0x7f7f7f7f7f7f7f7fULL) | ((n << 1ULL) & 0xfefefefefefefefeULL);
        U64 h2 = ((n >> 2ULL) & 0x3f3f3f3f3f3f3f3fULL) | ((n << 2ULL) & 0xfcfcfcfcfcfcfcfcULL);
        knightAttacks[sq] = (h1 << 16ULL) | (h1 >> 16ULL) | (h2 << 8ULL) | (h2 >> 8ULL);

        // King
        U64 attacks = shiftLeft(king) | shiftRight(king) | king;
        attacks = (attacks | shiftUp(attacks) | shiftDown(attacks)) ^ king;
        kingAttacks[sq] = attacks;
        king <<= 1ULL;
    }

    // Init slider attacks on an empty board
    for (Sq sq = 0; sq < 64; sq++)
    {
        // last arg true = exclude last sq of each direction
        bishopAttacksEmptyBoard[sq] = bishopAttacksSlow(sq, 0ULL, true);
        rookAttacksEmptyBoard[sq] = rookAttacksSlow(sq, 0ULL, true);
    }

    // Init slider tables
    for (Sq sq = 0; sq < 64; sq++)
    {
        // Bishop
        U64 numBlockersArrangements = 1ULL << std::popcount(bishopAttacksEmptyBoard[sq]);
        for (U64 n = 0; n < numBlockersArrangements; n++)
        {
            U64 blockersArrangement = pdep(n, bishopAttacksEmptyBoard[sq]);
            U64 key = (blockersArrangement * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq];
            bishopAttacksTable[sq][key] = bishopAttacksSlow(sq, blockersArrangement);
        }

        // Rook
        numBlockersArrangements = 1ULL << std::popcount(rookAttacksEmptyBoard[sq]);
        for (U64 n = 0; n < numBlockersArrangements; n++)
        {
            U64 blockersArrangement = pdep(n, rookAttacksEmptyBoard[sq]);
            U64 key = (blockersArrangement * ROOK_MAGICS[sq]) >> ROOK_SHIFTS[sq];
            rookAttacksTable[sq][key] = rookAttacksSlow(sq, blockersArrangement);
        }
    }

}

inline U64 bishopAttacks(Sq sq, U64 occupancy)
{
    using namespace internal;

    // Mask to only include bits on diagonals
    U64 blockers = occupancy & bishopAttacksEmptyBoard[sq];

    // Generate the key using a multiplication and right shift
    U64 key = (blockers * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq];

    // Return the preinitialized attack set bitboard from the table
    return bishopAttacksTable[sq][key];
}

inline U64 rookAttacks(Sq sq, U64 occupancy)
{
    using namespace internal;

    // Mask to only include bits on rank and file
    U64 blockers = occupancy & rookAttacksEmptyBoard[sq];

    // Generate the key using a multiplication and right shift
    U64 key = (blockers * ROOK_MAGICS[sq]) >> ROOK_SHIFTS[sq];

    // Return the preinitialized attack set bitboard from the table
    return rookAttacksTable[sq][key];
}

inline U64 queenAttacks(Sq sq, U64 occupancy)
{
    return bishopAttacks(sq, occupancy) | rookAttacks(sq, occupancy);
}

}


#endif // #ifndef MAGICBITBOARDS_HPP