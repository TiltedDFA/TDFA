#include "MagicBitboards.hpp"

U64 MagicBitBoards::internal::pawnAttacks[2][64];
U64 MagicBitBoards::internal::knightAttacks[64];
U64 MagicBitBoards::internal::kingAttacks[64];

U64 MagicBitBoards::internal::bishopAttacksEmptyBoard[64];
U64 MagicBitBoards::internal::rookAttacksEmptyBoard[64];
U64 MagicBitBoards::internal::bishopAttacksTable[64][1ULL << 9ULL]; 
U64 MagicBitBoards::internal::rookAttacksTable[64][1ULL << 12ULL];