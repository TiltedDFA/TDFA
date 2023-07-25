// THIS IS MAINLY A TEST FILE
#include <stdio.h>
#include <stdlib.h>

//#define USE_32_BIT_MULTIPLICATIONS

typedef unsigned long long uint64;

uint64 random_uint64() {
  uint64 u1, u2, u3, u4;
  u1 = (uint64)(random()) & 0xFFFF; u2 = (uint64)(random()) & 0xFFFF;
  u3 = (uint64)(random()) & 0xFFFF; u4 = (uint64)(random()) & 0xFFFF;
  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

uint64 random_uint64_fewbits() {
  return random_uint64() & random_uint64() & random_uint64();
}

int count_1s(uint64 b) {
  int r;
  for(r = 0; b; r++, b &= b - 1);
  return r;
}

const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};

int pop_1st_bit(uint64 *bb) {
  uint64 b = *bb ^ (*bb - 1);
  unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
  *bb &= (*bb - 1);
  return BitTable[(fold * 0x783a9b23) >> 26];
}

uint64 index_to_uint64(int index, int bits, uint64 m) {
  int i, j;
  uint64 result = 0ULL;
  for(i = 0; i < bits; i++) {
    j = pop_1st_bit(&m);
    if(index & (1 << i)) result |= (1ULL << j);
  }
  return result;
}

uint64 rmask(int sq) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1; r <= 6; r++) result |= (1ULL << (fl + r*8));
  for(r = rk-1; r >= 1; r--) result |= (1ULL << (fl + r*8));
  for(f = fl+1; f <= 6; f++) result |= (1ULL << (f + rk*8));
  for(f = fl-1; f >= 1; f--) result |= (1ULL << (f + rk*8));
  return result;
}

uint64 bmask(int sq) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r=rk+1, f=fl+1; r<=6 && f<=6; r++, f++) result |= (1ULL << (f + r*8));
  for(r=rk+1, f=fl-1; r<=6 && f>=1; r++, f--) result |= (1ULL << (f + r*8));
  for(r=rk-1, f=fl+1; r>=1 && f<=6; r--, f++) result |= (1ULL << (f + r*8));
  for(r=rk-1, f=fl-1; r>=1 && f>=1; r--, f--) result |= (1ULL << (f + r*8));
  return result;
}

uint64 ratt(int sq, uint64 block) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1; r <= 7; r++) {
    result |= (1ULL << (fl + r*8));
    if(block & (1ULL << (fl + r*8))) break;
  }
  for(r = rk-1; r >= 0; r--) {
    result |= (1ULL << (fl + r*8));
    if(block & (1ULL << (fl + r*8))) break;
  }
  for(f = fl+1; f <= 7; f++) {
    result |= (1ULL << (f + rk*8));
    if(block & (1ULL << (f + rk*8))) break;
  }
  for(f = fl-1; f >= 0; f--) {
    result |= (1ULL << (f + rk*8));
    if(block & (1ULL << (f + rk*8))) break;
  }
  return result;
}

uint64 batt(int sq, uint64 block) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  return result;
}


int transform(uint64 b, uint64 magic, int bits) {
#if defined(USE_32_BIT_MULTIPLICATIONS)
  return
    (unsigned)((int)b*(int)magic ^ (int)(b>>32)*(int)(magic>>32)) >> (32-bits);
#else
  return (int)((b * magic) >> (64 - bits));
#endif
}

uint64 find_magic(int sq, int m, int bishop) {
  uint64 mask, b[4096], a[4096], used[4096], magic;
  int i, j, k, n, fail;

  mask = bishop? bmask(sq) : rmask(sq);
  n = count_1s(mask);

  for(i = 0; i < (1 << n); i++) {
    b[i] = index_to_uint64(i, n, mask);
    a[i] = bishop? batt(sq, b[i]) : ratt(sq, b[i]);
  }
  for(k = 0; k < 100000000; k++) {
    magic = random_uint64_fewbits();
    if(count_1s((mask * magic) & 0xFF00000000000000ULL) < 6) continue;
    for(i = 0; i < 4096; i++) used[i] = 0ULL;
    for(i = 0, fail = 0; !fail && i < (1 << n); i++) {
      j = transform(b[i], magic, m);
      if(used[j] == 0ULL) used[j] = a[i];
      else if(used[j] != a[i]) fail = 1;
    }
    if(!fail) return magic;
  }
  printf("***Failed***\n");
  return 0ULL;
}

int RBits[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

int BBits[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

int main() {
  int square;

  printf("const uint64 RMagic[64] = {\n");
  for(square = 0; square < 64; square++)
    printf("  0x%llxULL,\n", find_magic(square, RBits[square], 0));
  printf("};\n\n");

  printf("const uint64 BMagic[64] = {\n");
  for(square = 0; square < 64; square++)
    printf("  0x%llxULL,\n", find_magic(square, BBits[square], 1));
  printf("};\n\n");

  return 0;
}
/*
#define MOVE int
#define SQUARE int
//OG
struct move_info
{
    short move[7];
    short count;
} move_lookup[64][4][2187];

MOVE *copy_bishop_moves(MOVE *first_move, SQUARE square)
{
    int direction;
    int index;
    for (d = 0; d < 2; d++)
    {
        index = base_2_to_3[File(square)][collapsed_files_index(our_occ_bb & mask[square][d])] +
            2 * base_2_to_3[File(square)][collapsed_files_index(opp_occ_bb & mask[square][d])];
        first_move = copy_moves(first_move, move_lookup[square][d][index]);
    }
}
//ESTIMATED
//Generally, it is empty, own, opponent.
struct move_info
{
    short move[7];
    short count;
} move_lookup[64][4][2187];

int base_2_to_3[256];

MOVE *copy_bishop_moves(MOVE *first_move, SQUARE square)
{
    int direction;
    int index;
    for (int d = 0; d < 2; d++)
    {
        index = base_2_to_3[File(square)][collapsed_files_index(our_occ_bb & mask[square][d])] +
            2 * base_2_to_3[File(square)][collapsed_files_index(opp_occ_bb & mask[square][d])];
        *first_move++ = move_lookup[square][d][index];
    }
}

#include <intrin.h>

short* moveCpy(short* pt /* unaligned */, const short* ps /* xmm aligned */)
{
  __m128i moves = _mm_load_si128((__m128i*) ps); // aligned load
  _mm_storeu_si128 ((__m128i*) pt, moves); // unaligned store
  return pt + ps[7]; // return pointer for next store
}


int a[8] = {0,0,1,0,0,1,0,1};
// Index:   0 1 2 3 4 5 6 7
int file = 5; //      ^
int file_copy = file - 1;
while(a[file_copy] == 0 and (0 <= file_copy) and (file_copy <= 7)){
    file_copy -= 1;
}
cout<< "First piece to the left is at file " << file_copy << endl;


    // bugged if there's no pieces to the right?
file = 5;
file_copy = file + 1;
while((file_copy > -1) and (file_copy < 8) and a[file_copy] == 0 ){
    file_copy += 1;
}
cout<< "First piece to the right is at file " << file_copy << endl;


#include <iostream>
#include <array>
#include <vector>
#include <bitset>

constexpr int NUM_SQUARES = 64;
constexpr int NUM_DIRECTIONS = 4;
constexpr int NUM_TERNARY_DIGITS = 7;
constexpr int NUM_TERNARY_COMBINATIONS = 2187; // 3^NUM_TERNARY_DIGITS

// Bitboard type (64-bit unsigned integer) to represent the chessboard
using BitBoard = uint64_t;

// Move representation (simplified)
struct Move {
    int from_square;
    int to_square;
    // Other move details...
};

// Function to generate all possible bishop moves from a given square
std::vector<Move> generate_bishop_moves(int square) {
    std::vector<Move> moves;

    // Implement the logic to generate all possible moves for a bishop from the given square
    // (This would involve checking all diagonal directions from the square and adding valid moves to the 'moves' vector.)

    return moves;
}

// Function to compute the ternary index for a given 8-bit number
int compute_ternary_index(uint8_t num) {
    int index = 0;
    for (int j = 0; j < NUM_TERNARY_DIGITS; ++j) {
        index += ((num >> j) % 2) * pow(3, j);
    }
    return index;
}

// Function to precompute and store bishop moves for each square
void precompute_bishop_moves() {
    // 3-dimensional array to store precomputed bishop moves for each square, direction, and ternary combination
    std::array<std::array<std::vector<Move>, NUM_DIRECTIONS>, NUM_SQUARES> move_lookup;

    // Loop through all squares on the board
    for (int square = 0; square < NUM_SQUARES; ++square) {
        // Loop through all diagonal directions (NE, NW, SE, SW)
        for (int direction = 0; direction < NUM_DIRECTIONS; ++direction) {
            // 2-dimensional array to store bishop moves for each ternary combination
            std::array<std::vector<Move>, NUM_TERNARY_COMBINATIONS> moves_per_combination;

            // Loop through all possible combinations of occupied squares in ternary representation
            for (int combination = 0; combination < NUM_TERNARY_COMBINATIONS; ++combination) {
                // Generate and store bishop moves for the current square, direction, and combination
                moves_per_combination[combination] = generate_bishop_moves(square);
                // The 'generate_bishop_moves' function would handle move generation for the bishop.

                // Increment 'combination' to get the next combination for the bishop moves
            }

            // Store the precomputed move data for the current square and direction
            move_lookup[square][direction] = std::move(moves_per_combination);
        }
    }

    // Now you have precomputed move data for bishops in the 'move_lookup' array.
    // The 'move_lookup' array will allow you to efficiently access possible moves for each square,
    // direction, and occupied combination during move generation.
}

int main() {
    // Call the precompute function to generate and store bishop moves
    precompute_bishop_moves();

    // Your chess engine can now access the precomputed data in the 'move_lookup' array when generating moves for bishops.
    // ...

    return 0;
}
