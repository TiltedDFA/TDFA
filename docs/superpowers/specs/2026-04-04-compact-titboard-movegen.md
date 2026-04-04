# Compact TITBOARD Movegen: Endpoint-Based Table Splitting

## Background

TDFA is a chess engine using a novel sliding piece move generation system called **TITBOARD** (Ternary Index Table BOARDs). The system uses base-3 (ternary) encoding to distinguish us/them/empty on each square along a ray, providing a precomputed lookup for sliding piece attacks and moves.

### Current Architecture (as of commit ~20c6a66, branch `claude`)

The engine maintains **two TITBOARD tables**:

1. **Attack-only table** — `BitBoard SLIDING_ATTACKS[64][4][2187]` (~4.3 MB)
   - Used by `IsSquareAttacked()` (legality checks in perft, search)
   - Each entry: 8-byte BitBoard of attacked squares
   - Already heavily optimized with line-mask pre-filtering, check reordering

2. **Move-info table** — `move_info SLIDING_ATTACK_CONFIG[64][4][2187]` (~12.8 MB)  
   - Used by `BishopMoves()`, `RookMoves()`, `QueenMoves()` for movegen
   - Each entry: 24 bytes (`std::array<Move, 7>` = 14B, `U8 count` = 1B, `BitBoard attacks` = 8B, +1B padding)
   - Moves are bulk-copied into MoveList via `merge()` → `std::copy_n`

**Total: ~17.1 MB** of lookup tables.

### Table Indexing

Both tables share the same indexing scheme:
- Dimension 1: `[64]` — piece square (0-63)
- Dimension 2: `[4]` — direction (File=0, Rank=1, Diagonal=2, AntiDiagonal=3)
- Dimension 3: `[2187]` — blocker configuration (3^7 = 2187, base-3 encoding of 7 squares along the ray, excluding the piece's own square)

The index is computed via:
1. PEXT extraction of occupancy along the relevant line
2. Base-3 conversion using precomputed `base_2_to_3_us[8][256]` lookup table
3. Formula: `base_2_to_3_us[sq][us_bits] + (base_2_to_3_us[sq][them_bits] << 1)`

**Critical constraint**: The TITBOARD ternary encoding system must be preserved. This is the engine's core identity.

### Current Performance

- **74M NPS** on perft benchmark (standard perft, not bulk)
- Profile breakdown: MakeMove 30%, IsSquareAttacked 34%, UnmakeMove 16%, Perft loop 8%, Movegen 8%
- The attack-only table (4.3 MB) is well-optimized
- The move-info table (12.8 MB) is the target for this optimization

### How Movegen Currently Works

For a bishop, movegen does:
```cpp
// 2 TITBOARD lookups into 12.8 MB table, each returns move_info* (24 bytes)
ml->merge(GetMovesForSliding<Diagonal>(sq, us, them));      // bulk copy up to 7 moves
ml->merge(GetMovesForSliding<AntiDiagonal>(sq, us, them));   // bulk copy up to 7 moves
```

For a rook: same with File + Rank (2 lookups).
For a queen: File + Rank + Diagonal + AntiDiagonal (4 lookups).

Each `merge()` does `std::copy_n` of pre-encoded `Move` values (U16) from the table into the MoveList. Fast per-call, but the 12.8 MB table causes cache pressure.

### Key Redundancy in move_info

Each `move_info` entry stores up to 7 fully-encoded `Move` values (U16 each). A `Move` encodes:
- Bits 0-5: `from` square (6 bits)
- Bits 6-11: `to` square (6 bits)  
- Bits 12-15: move type (4 bits, but only quiet=0 or capture=3 for sliding pieces)

**The redundancy**: `from` is the SAME for all 7 moves in an entry (it's the piece square, already known at call time). And the move type is always quiet except possibly the last move (which may be a capture). The intermediate squares between `from` and the ray endpoint are DETERMINISTIC given the direction — they follow a fixed step pattern (+8 for north, +1 for east, +9 for NE, etc.).

So 14 bytes of encoded moves can be reduced to just knowing the **endpoint** and **capture flag** per sub-direction.

---

## The Proposal: Endpoint-Based Table Splitting

### Core Insight

Each TITBOARD direction entry covers TWO sub-directions (e.g., File covers both North and South). For each sub-direction, the blocker-dependent information is just:
- **How far does the ray extend?** (0-7 squares, 3 bits)
- **Is the last square a capture?** (1 bit)

Everything else (which squares are between start and endpoint, the `from` square, the direction step) is static geometry computable at startup.

### The 1-Byte Endpoint Encoding

Both sub-directions of a TITBOARD direction fit in a single byte:

```
Bit layout of endpoint byte:
  [7]      [6]      [5:3]        [2:0]
  neg_cap  pos_cap  neg_offset   pos_offset

- pos_offset (3 bits): distance in positive sub-direction (0 = no moves, 1-7 = number of squares)
- neg_offset (3 bits): distance in negative sub-direction (0 = no moves, 1-7 = number of squares)  
- pos_cap (1 bit): last square in positive direction is a capture (only meaningful when pos_offset > 0)
- neg_cap (1 bit): last square in negative direction is a capture (only meaningful when neg_offset > 0)
```

Sub-direction mapping per TITBOARD direction:
- File: positive = North (+8), negative = South (-8)
- Rank: positive = East (+1), negative = West (-1)
- Diagonal: positive = NE (+9), negative = SW (-9)
- AntiDiagonal: positive = NW (+7), negative = SE (-7)

Max offset is 7 (e.g., rook on a1 to a8 = 7 squares north). Fits in 3 bits.

### Proposed Table Structure

#### Level 1: Endpoint Table (TITBOARD, blocker-dependent)

```cpp
// Replaces SLIDING_ATTACK_CONFIG (12.8 MB → 547 KB)
U8 SLIDING_ENDPOINTS[64][4][2187];  // 64 × 4 × 2187 × 1 = 559,872 bytes ≈ 547 KB
```

Same TITBOARD ternary indexing as before. Same `GetSlidingAttacks`-style index computation. Just stores 1 byte instead of 24.

#### Level 2: Pre-encoded Move Table (geometric, blocker-independent)

```cpp
// Maps (from_sq, direction, endpoint_byte) → pre-encoded moves ready for copy_n
struct ray_moves {
    Move moves[7];  // pre-encoded Move values (U16), from known, to sequential, types set
    U8 count;       // number of valid moves (0-7)
};

ray_moves MOVE_LOOKUP[64][4][256];  // 64 × 4 × 256 × 15 = 983,040 bytes ≈ 960 KB
```

This table is purely geometric — no blocker configs. For each (square, direction, endpoint_byte), the moves are deterministic. Pre-encode them at startup.

Many of the 256 endpoint values per (square, direction) are geometrically invalid (e.g., a rook on a1 can't have neg_offset=5 southward since there's no room). Invalid entries have count=0.

**Combined Level 1 + Level 2: ~1.5 MB** (down from 12.8 MB for move_info alone).

#### Attack Table (unchanged)

```cpp
// Stays as-is — already optimized, used by IsSquareAttacked
BitBoard SLIDING_ATTACKS[64][4][2187];  // 4.3 MB, unchanged
```

### Table Sizes Comparison

| Table | Current | Proposed |
|-------|---------|----------|
| Attack-only (`SLIDING_ATTACKS`) | 4.3 MB | 4.3 MB (unchanged) |
| Movegen (`SLIDING_ATTACK_CONFIG` / endpoints+moves) | **12.8 MB** | **~1.5 MB** (547 KB + 960 KB) |
| **Total** | **17.1 MB** | **5.8 MB** |

### Runtime Flow for Movegen

**Current** (bishop example):
```cpp
// 2 lookups into 12.8 MB table, 2 bulk copies
ml->merge(GetMovesForSliding<Diagonal>(sq, us, them));       // 24 bytes touched
ml->merge(GetMovesForSliding<AntiDiagonal>(sq, us, them));   // 24 bytes touched
```

**Proposed** (bishop example):
```cpp
// 2 lookups into 547 KB table (1 byte each), 2 lookups into 960 KB table, 2 bulk copies
U8 ep_diag = GetEndpoint<Diagonal>(sq, us, them);            // 1 byte from Level 1
U8 ep_adiag = GetEndpoint<AntiDiagonal>(sq, us, them);       // 1 byte from Level 1
ml->merge_ray(&MOVE_LOOKUP[sq][Diagonal][ep_diag]);          // copy_n from Level 2
ml->merge_ray(&MOVE_LOOKUP[sq][AntiDiagonal][ep_adiag]);     // copy_n from Level 2
```

The `merge_ray` function is identical to the current `merge()` — `std::copy_n` of pre-encoded Move values. The difference is WHERE the data comes from: a 960 KB table (fits in L2) instead of a 12.8 MB table (spills to L3).

---

## Future Extension: Level 3 Composite Lookup (Speculative)

### Idea

Instead of doing 2 separate Level 2 lookups for a bishop (one per direction), combine both endpoint bytes into a composite key and do ONE lookup that returns ALL moves for the piece.

### Composite Key

Strip capture flags from the key (they only affect move type of the last square per sub-direction, can be patched after copy):

```
// 12-bit composite key from two endpoint bytes
U16 key = (diag_pos_off)           // bits 0-2
        | (diag_neg_off << 3)      // bits 3-5
        | (antidiag_pos_off << 6)  // bits 6-8
        | (antidiag_neg_off << 9); // bits 9-11

// 4096 possible keys, but only ~200-400 geometrically valid per square
```

### Level 3 Table Structure

```cpp
// Indirect indexing: composite key → dense index → move pool
U16 BISHOP_INDEX[64][4096];           // ~512 KB, maps key to dense pool offset (0xFFFF = invalid)
struct combined_moves {
    Move moves[13];                   // max 13 for bishop (7 + 6 on two diagonals)
    U8 count;
};
combined_moves BISHOP_POOL[~20000];   // ~540 KB, all valid move sequences

// Similar for ROOK_INDEX/ROOK_POOL (max 14 moves: 7 file + 7 rank)
```

### Level 3 Runtime Flow (bishop)

```cpp
U8 ep_diag = GetEndpoint<Diagonal>(sq, us, them);
U8 ep_adiag = GetEndpoint<AntiDiagonal>(sq, us, them);

// Extract offsets (strip capture flags)
U16 key = (ep_diag & 0x3F) | ((ep_adiag & 0x3F) << 6);
U16 pool_idx = BISHOP_INDEX[sq][key];
const auto& entry = BISHOP_POOL[pool_idx];

// One bulk copy of ALL bishop moves
std::copy_n(entry.moves, entry.count, ml->data() + ml->len());
ml->advance(entry.count);

// Patch capture flags on up to 4 sub-direction endpoints
if (ep_diag & 0x40)  patch_last_move_capture(...);   // pos diagonal
if (ep_diag & 0x80)  patch_last_move_capture(...);   // neg diagonal
if (ep_adiag & 0x40) patch_last_move_capture(...);   // pos anti-diagonal
if (ep_adiag & 0x80) patch_last_move_capture(...);   // neg anti-diagonal
```

### Level 3 Concerns

- **Complexity**: Index tables, pool allocation, capture patching adds code complexity
- **Patching cost**: 4 conditional patches per bishop (branches + random MoveList writes)
- **Uncertain benefit**: Movegen is only 8% of runtime. Even a 50% movegen improvement = ~4% total NPS gain
- **Queen handling**: Queen = bishop_pool_lookup + rook_pool_lookup (2 lookups, still an improvement over 4)

### Recommendation

**Implement Level 1 + Level 2 first.** This is the safe, high-confidence change:
- 12.8 MB → 1.5 MB table reduction (8.5x smaller)
- Same `copy_n` bulk copy mechanism
- No complex patching or indirect indexing
- Straightforward implementation

Benchmark, then decide if Level 3 is worth the complexity.

---

## Implementation Guide

### Files to Modify

| File | Changes |
|------|---------|
| `src/Types.hpp` | Keep `move_info` struct (may still be useful), add `ray_moves` struct |
| `src/MoveGen.cpp` | Add `PrecomputeEndpoints()` and `PrecomputeMoveLookup()` functions |
| `src/MoveGen.hpp` | Add `GetEndpoint<Dir>()` function, update `BishopMoves`/`RookMoves`/`QueenMoves`, add externs for new tables |
| `src/MoveList.hpp` | Add `merge_ray(ray_moves const*)` or reuse existing `merge()` adapted for `ray_moves` |
| `src/MagicConstants.hpp` | Add direction step constants |

### PrecomputeEndpoints()

Same triple-nested loop as existing `PrecomputeTitboards()` (sq × us × them), same skip conditions, same index computation. But instead of encoding individual moves, just track:
- For each sub-direction: how many squares were reachable, and was the last one a capture?
- Pack into the 1-byte format: `(neg_cap << 7) | (pos_cap << 6) | (neg_offset << 3) | pos_offset`

### PrecomputeMoveLookup()

For each (from_sq, direction, endpoint_byte):
1. Decode the endpoint byte: pos_offset, neg_offset, pos_cap, neg_cap
2. Walk the positive sub-direction: for i in 1..pos_offset, encode `Move(from, from + i*step, quiet/capture)`
3. Walk the negative sub-direction: for i in 1..neg_offset, encode `Move(from, from - i*step, quiet/capture)`
4. The last move in each sub-direction gets `mt_Capture` if the corresponding capture flag is set

Direction steps: File=(+8,-8), Rank=(+1,-1), Diagonal=(+9,-9), AntiDiagonal=(+7,-7).

**Important**: For Rank direction, steps of ±1 need file-boundary checking during precomputation. For diagonals, steps of ±7/±9 need both rank and file boundary checking. These constraints are implicitly handled by the TITBOARD's ternary encoding (invalid configs are never looked up), but the Level 2 precomputation must independently validate square positions.

### GetEndpoint<Dir>()

Identical to `GetSlidingAttacks<Dir>()` — same PEXT extraction, same base-3 conversion, same index computation. Just accesses `SLIDING_ENDPOINTS` instead of `SLIDING_ATTACKS` and returns `U8` instead of `BitBoard`.

### Updated Movegen Functions

```cpp
template<Colour C>
constexpr void BishopMoves(Position const* pos, MoveList* ml)
{
    BitBoard bishops = pos->Pieces(C, pt_Bishop);
    if (!bishops) return;
    const BitBoard us = pos->Pieces(C);
    const BitBoard them = pos->Pieces(!C);
    while (bishops)
    {
        const Sq sq = Magics::PopNRetLS1B(bishops);
        U8 ep_diag  = GetEndpoint<Diagonal>(sq, us, them);
        U8 ep_adiag = GetEndpoint<AntiDiagonal>(sq, us, them);
        if (ep_diag)  ml->merge_ray(&MOVE_LOOKUP[sq][Diagonal][ep_diag]);
        if (ep_adiag) ml->merge_ray(&MOVE_LOOKUP[sq][AntiDiagonal][ep_adiag]);
    }
}
```

Note: `ep == 0` means both offsets are 0 and both capture flags are 0 — no moves in either sub-direction. This is the early-out.

### Verification

```bash
cd /home/malikt/Documents/TDFA/build && cmake --build . --config Release -j$(nproc) 2>&1 && ./TDFA bench
```

All 6 perft tests must pass with correct node counts:
- Test 1: 119,060,324 nodes at depth 6 (startpos)
- Test 2: 193,690,690 nodes at depth 5 (kiwipete)
- Test 3: 178,633,661 nodes at depth 7 (perft pos 3)
- Test 4: 706,045,033 nodes at depth 6 (perft pos 4)
- Test 5: 89,941,194 nodes at depth 5 (perft pos 5)
- Test 6: 164,075,551 nodes at depth 5 (perft pos 6)

Current baseline: ~74M NPS mean. Target: ≥75M NPS.

---

## Profiling Results (Level 1+2 implemented)

### Performance

- **Perft NPS**: 74.0M → 74.7M (+1% overall)
- **Isolated movegen**: 830M → 847M moves/sec (+2% in A/B benchmark with 95 slider-heavy positions)
- **Total table memory**: 17.1 MB → 5.8 MB (3x reduction)

### A/B Profiling (isolated movegen, same binary)

| Hotspot | NEW (endpoint) | OLD (move_info) | Insight |
|---------|---------------|-----------------|---------|
| `GetBaseThreeUsThem` | 9.41% | 9.81% | Base-3 index — identical code, single biggest hotspot |
| `GenerateMovesFromBB` | 8.44% | 7.48% | Bit-scan loop for pawn/knight — same code |
| `EncodeMove` | 6.87% | 6.34% | Move encoding — same code |
| `copy_n` (merge) | **6.07%** | **9.76%** | **KEY: OLD pays 60% more due to 12.8MB table cache misses** |
| `_pext_u64` | 5.36% | 5.18% | PEXT extraction — same |
| Table access | **3.56%** | **4.64%** | **NEW: 1B from 547KB table vs OLD: 24B from 12.8MB table** |

**Where the 2% speedup comes from**: `copy_n` savings (3.69%) + table access savings (1.08%) = ~4.77% reduction in cache-sensitive operations. Half is masked by unchanged code paths (pawn/knight gen, encoding).

### Key Observation

`GetBaseThreeUsThem` at ~9.5% is the single biggest hotspot in movegen. It does two lookups into `base_2_to_3_us[8][256]` (4KB table). This is a potential target for further optimization — the base-3 conversion is the price we pay for ternary encoding.

---

## Key Constraints

1. **TITBOARD ternary encoding is sacred** — the base-3 us/them/empty encoding and lookup mechanism must be preserved
2. **Do not modify `src/Testing.hpp`** — the perft benchmark must remain identical for fair comparison with Codex (dev branch)
3. **The attack-only table (`SLIDING_ATTACKS`) is unchanged** — IsSquareAttacked optimizations are already done
4. **Benchmark timeout: 60 seconds** — anything longer indicates an error
5. **Build: g++ 14.2, C++20, `-O3 -march=native -flto`, Ninja generator**
6. **Build flags include**: `--param max-inline-insns-auto=2000 --param inline-unit-growth=500` (for aggressive LTO inlining of MakeMove/UnmakeMove into Perft)
