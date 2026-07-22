# TDFA Sanitized API Dossier

Snapshot: `6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592`  
Generator version: `1`

> This is intentionally non-compilable authoring input. Source comments,
> literals, initializers, and executable bodies are omitted. It states no
> behavioral contract; use the separately approved contract cards.

## `src/Board.hpp`

```cpp
#define TDFA_BOARD_HPP <replacement omitted>

class Board
{
public:
    constexpr void ResetBoard()
    ; /* body omitted */
    constexpr Board()
    ; /* body omitted */
    [[nodiscard]] constexpr BitBoard Pieces(const PieceType pt) const
    ; /* body omitted */
    [[nodiscard]] constexpr BitBoard Pieces(const Colour c) const
    ; /* body omitted */
    [[nodiscard]] constexpr BitBoard Pieces(Colour, Colour) const
    ; /* body omitted */
    template<typename... PieceTypes>
    [[nodiscard]] constexpr BitBoard Pieces(const PieceType p, PieceTypes... pt) const
    ; /* body omitted */
    template<typename... PieceTypes>
    [[nodiscard]] constexpr BitBoard Pieces(const Colour c, PieceTypes... p) const
    ; /* body omitted */
    [[nodiscard]] constexpr Piece PieceOn(const Sq s) const
    ; /* body omitted */
    [[nodiscard, gnu::always_inline]]
    static constexpr inline Piece MakePiece(const Colour c, const PieceType t)
    ; /* body omitted */
    constexpr void pedantic_check(Sq s, bool add, Piece p = Piece::p_None) const
    ; /* body omitted */
    constexpr void pedantic_check(Sq s, bool add, Piece p = Piece::p_None) const
    ; /* body omitted */
    constexpr void AddPiece(const Piece p, const Sq s)
    ; /* body omitted */
    constexpr void RemovePiece(const Sq s)
    ; /* body omitted */
    constexpr Piece PopPiece(const Sq s)
    ; /* body omitted */
    constexpr void MovePiece(const Sq from, const Sq to)
    ; /* body omitted */
private:
    Piece    board_[64];
    BitBoard by_colour_[2];
    BitBoard by_type_  [7];                           
};
```

## `src/BoardUtils.hpp`

```cpp
#define BOARDUTILS_HPP <replacement omitted>

constexpr std::string_view RemoveWhiteSpace(std::string_view str)
; /* body omitted */
constexpr void SplitFen(std::string_view fen, std::array<std::string_view,6>& fen_sections)
; /* body omitted */
constexpr bool IsDigit(const char i) ; /* body omitted */
```

## `src/Debug.hpp`

```cpp
#define DEBUG_HPP <replacement omitted>

#define PRINT <replacement omitted>
#define PRINTNL <replacement omitted>
#define PRINT <replacement omitted>
#define PRINTNL <replacement omitted>

namespace Debug
{

    void PrintBB(BitBoard board, bool mirrored = false);

    void PrintBB(BitBoard board, int board_center, bool mirrored = false);

    void PrintBoardState(const Position& pos);

    void PrintInduvidualPieces(const BitBoard (&board)[2][6]);

    std::string PieceTypeToStr(PieceType piece);

    void PrintEncodedMoveStr(Move move);

    void PrintEncodedMoveBin(Move move);

    void PrintUsThem(BitBoard us, BitBoard them, bool mirrored = false);

    void PrintUsThemBlank(BitBoard us, BitBoard them, bool mirrored = false);

    void PrintEncodedMovesMoveInfo(const move_info& move, bool mirrored = false);

    void PrintU8BB(U8 bb, U8 board_center, bool mirrored = false);

    void PrintBoardGraphically(Position* pos);
}
```

## `src/Evaluate.hpp`

```cpp
#define EVALUATE_HPP <replacement omitted>

namespace Eval
{

    constexpr Score POS_INF = <initializer omitted>;
    constexpr Score NEG_INF = <initializer omitted>;
    constexpr Score OUT_OF_TIME = <initializer omitted>;
    constexpr Score PAWN_VAL    = <initializer omitted>;
    constexpr Score KNIGHT_VAL  = <initializer omitted>;
    constexpr Score BISHOP_VAL  = <initializer omitted>;
    constexpr Score ROOK_VAL    = <initializer omitted>;
    constexpr Score QUEEN_VAL   = <initializer omitted>;
    constexpr std::array<Score, 7> PAWN_PROGRESS_BONUS = <initializer omitted>;
    inline bool is_middle_game;

    template<Colour colour_to_move>
    constexpr Score CountMaterial(Position const* pos)
    ; /* body omitted */
    constexpr void UpdateData(Position const* pos)
    ; /* body omitted */
    template<Colour colour_to_move>
    constexpr Score Mobility(Position const* pos)
    ; /* body omitted */
    template<Colour colour_to_move>
    constexpr Score PawnProgress(Position const* pos)
    ; /* body omitted */
    constexpr Score Evaluate(Position const* pos)
    ; /* body omitted */
}
```

## `src/MagicConstants.hpp`

```cpp
#define MAGICCONSTANTS_HPP <replacement omitted>

namespace Magics
{

    template<Sq N>
    consteval BitBoard SqToBB() noexcept ; /* body omitted */

    constexpr Sq FileOf(U8 index) noexcept ; /* body omitted */

    constexpr U16 EncodeKing(Sq start, Sq target) ; /* body omitted */

    constexpr BitBoard FILE_ABB = <initializer omitted>;
    constexpr BitBoard FILE_BBB = <initializer omitted>;
    constexpr BitBoard FILE_CBB = <initializer omitted>;
    constexpr BitBoard FILE_DBB = <initializer omitted>;
    constexpr BitBoard FILE_EBB = <initializer omitted>;
    constexpr BitBoard FILE_FBB = <initializer omitted>;
    constexpr BitBoard FILE_GBB = <initializer omitted>;
    constexpr BitBoard FILE_HBB = <initializer omitted>;

    constexpr BitBoard RANK_1BB = <initializer omitted>;
    constexpr BitBoard RANK_2BB = <initializer omitted>;
    constexpr BitBoard RANK_3BB = <initializer omitted>;
    constexpr BitBoard RANK_4BB = <initializer omitted>;
    constexpr BitBoard RANK_5BB = <initializer omitted>;
    constexpr BitBoard RANK_6BB = <initializer omitted>;
    constexpr BitBoard RANK_7BB = <initializer omitted>;
    constexpr BitBoard RANK_8BB = <initializer omitted>;

    constexpr U8 EP_NULL = <initializer omitted>;

    constexpr BitBoard CROSS_DIAG = <initializer omitted>;                   
    constexpr BitBoard ANTI_CROSS_DIAG = <initializer omitted>;              

    constexpr BitBoard ROOK_START_SQS = <initializer omitted>;

    constexpr U8 CASTLE_K_W = <initializer omitted>;
    constexpr U8 CASTLE_Q_W = <initializer omitted>;
    constexpr U8 CASTLE_K_B = <initializer omitted>;
    constexpr U8 CASTLE_Q_B = <initializer omitted>;
    constexpr U8 CASTLE_ALL = <initializer omitted>;

    constexpr U8 NO_CASTLE_W = <initializer omitted>;
    constexpr U8 NO_CASTLE_B = <initializer omitted>;

    constexpr BitBoard ROOK_TO_FROM_W_Q = <initializer omitted>;
    constexpr BitBoard ROOK_TO_FROM_W_K = <initializer omitted>;
    constexpr BitBoard ROOK_TO_FROM_B_Q = <initializer omitted>;
    constexpr BitBoard ROOK_TO_FROM_B_K = <initializer omitted>;

    constexpr BitBoard ROOK_TO_FROM_ARR_BB[5] = <initializer omitted>;
    constexpr Sq ROOK_TO_FROM_ARR[5][2] = <initializer omitted>;
    constexpr ZobristKey CASTLING_ZOB_KEYS[5] = <initializer omitted>;
    constexpr BitBoard GetLS1B(BitBoard bb) noexcept ; /* body omitted */

    constexpr Sq FindLS1B(BitBoard bb) noexcept ; /* body omitted */

    constexpr U8 PopCnt(BitBoard bb) noexcept ; /* body omitted */

    constexpr Sq FindLS1B(BitBoard bb)  noexcept ; /* body omitted */

    constexpr uint8_t PopCnt(BitBoard bb) noexcept ; /* body omitted */

    constexpr double pow(double x, unsigned int y) noexcept ; /* body omitted */

    constexpr Sq FindMS1B(BitBoard board) noexcept ; /* body omitted */

    constexpr BitBoard PopLS1B(BitBoard board) noexcept ; /* body omitted */

    constexpr Sq PopNRetLS1B(BitBoard& board) noexcept
    ; /* body omitted */

    constexpr bool ValidSq(int index) noexcept ; /* body omitted */

    constexpr BitBoard SqToBB(Sq index) ; /* body omitted */

    [[nodiscard, gnu::always_inline]]
    constexpr PieceType TypeOf(Piece p)
    ; /* body omitted */
    [[nodiscard, gnu::always_inline]]
    constexpr Colour ColourOf(Piece p)
    ; /* body omitted */

    constexpr Sq RankOf(Sq index) ; /* body omitted */

    constexpr U8 BBFileOf(Sq square) noexcept ; /* body omitted */

    constexpr U8 BBRankOf(Sq square) noexcept ; /* body omitted */

    constexpr U8 CollapsedFilesIndex(BitBoard b) noexcept
    ; /* body omitted */

    constexpr U8 CollapsedRanksIndex(BitBoard b) noexcept
    ; /* body omitted */
    constexpr U8 CollapsedRanksIndex(BitBoard b, U8 file) noexcept
    ; /* body omitted */
    constexpr BitBoard PopMS1B(const BitBoard board) noexcept
    ; /* body omitted */

    template<MD D>
    constexpr BitBoard Shift(BitBoard b) noexcept
    ; /* body omitted */

    consteval std::array<BitBoard, 64> KnightAttackingMask() noexcept
    ; /* body omitted */

    template<bool gen_us>
    consteval std::array<std::array<U16, 256>, 8> compute_base_2_to_3() noexcept
    ; /* body omitted */

    consteval std::array<std::array<BitBoard, 4> ,64> PrecomputeMask() noexcept
    ; /* body omitted */

    consteval std::array<BitBoard, 64> KingAttackingMask() noexcept
    ; /* body omitted */

    inline constexpr std::array<std::array<U16, 256>, 8> base_2_to_3_us = <initializer omitted>;

    inline constexpr std::array<std::array<U16, 256>, 8> base_2_to_3_them = <initializer omitted>;

    inline constexpr U16 GetBaseThreeUsThem(U8 us, U8 them, Sq piece_square) noexcept
    ; /* body omitted */

    inline constexpr std::array<std::array<BitBoard, 4>, 64> SLIDING_ATTACKS_MASK = <initializer omitted>;

    inline constexpr std::array<BitBoard, 64> KNIGHT_ATTACK_MASKS = <initializer omitted>;

    inline constexpr std::array<BitBoard, 64> KING_ATTACK_MASKS = <initializer omitted>;
}
```

## `src/Move.hpp`

```cpp
#define MOVE_HPP <replacement omitted>

namespace Moves
{
    constexpr U32 START_SQ_MASK    = <initializer omitted>;
    constexpr U32 END_SQ_MASK      = <initializer omitted>;
    constexpr U32 PIECE_TYPE_MASK  = <initializer omitted>;
    constexpr U32 COLOUR_MASK      = <initializer omitted>;
    constexpr U16 END_SQ_SHIFT     = <initializer omitted>;
    constexpr U16 PIECE_TYPE_SHIFT = <initializer omitted>;

    constexpr PieceType BAD_MOVE = <initializer omitted>;
    constexpr Move NULL_MOVE = <initializer omitted>;

    constexpr Move EncodeMove(const Sq start_index, const Sq target_index, const MoveType move_type)
    ; /* body omitted */
    constexpr void DecodeMove(const Move move, Sq* __restrict__ start_index, Sq* __restrict__ target_index, MoveType* __restrict__ move_type)
    ; /* body omitted */
    constexpr U8 TargetSq(const Move move)     ; /* body omitted */

    constexpr U8 StartSq(const Move move)      ; /* body omitted */

    constexpr bool IsPromotionMove(const Move move) ; /* body omitted */
    constexpr PieceType PTypeOfProm(const Move move)
    ; /* body omitted */
}
```

## `src/MoveGen.hpp`

```cpp
#define MOVEGEN_HPP <replacement omitted>

extern std::array<std::array<std::array<move_info, 2187>, 4>, 64> SLIDING_ATTACK_CONFIG;
namespace MoveGen
{
    constexpr void GenerateMovesFromBB(BitBoard b, MoveList* ml, const Sq from, const MoveType type)
    ; /* body omitted */

    template<AttackDirection direction>
    constexpr move_info const* GetMovesForSliding(Sq piece_sq, BitBoard us, BitBoard them) noexcept
    ; /* body omitted */

    void WhitePawnMoves(Position const* pos, MoveList* ml) noexcept;

    void BlackPawnMoves(Position const* pos, MoveList* ml) noexcept;

    template<Colour colour_to_move>
    constexpr void BishopMoves(Position const* pos, MoveList* ml)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr void RookMoves(Position const* pos, MoveList* ml)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr void QueenMoves(Position const* pos, MoveList* ml)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr void KnightMoves(Position const* pos, MoveList* ml)
    ; /* body omitted */

    template<Colour colour_to_move>
    void KingMoves(Position const* pos, MoveList* ml) 
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr BitBoard PawnAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr BitBoard KingAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr BitBoard KnightAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr BitBoard BishopAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr BitBoard RookAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr BitBoard QueenAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    bool InCheck(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    BitBoard GenerateAllAttacks(Position const* pos)
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr void Castling(Position const* pos, MoveList* ml) noexcept
    ; /* body omitted */

    template<Colour colour_to_move>
    constexpr void GeneratePseudoLegalMoves(Position const* __restrict__  pos, MoveList* __restrict__ ml)
    ; /* body omitted */

    template<Colour colour_to_move>
    void GenerateLegalMoves(Position* __restrict__  pos, MoveList* __restrict__  ml)
    ; /* body omitted */
};
```

## `src/MoveList.hpp`

```cpp
#define MOVELIST_HPP <replacement omitted>

class MoveList
{
public:
    constexpr MoveList()
; /* body omitted */

    constexpr void add(const Move m) noexcept  ; /* body omitted */

    constexpr Move operator[](const size_t index) const noexcept ; /* body omitted */

    constexpr void merge(move_info const* src)
    ; /* body omitted */

    [[nodiscard]] constexpr std::array<Move, MAX_MOVES>& all() noexcept ; /* body omitted */

    [[nodiscard]] constexpr size_t len()const noexcept ; /* body omitted */

    [[nodiscard]] constexpr bool contains(const Move m) const ; /* body omitted */

private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};
```

## `src/Position.hpp`

```cpp
#define BITBOARD_HPP <replacement omitted>

struct StateInfo
{
public:
    constexpr StateInfo()
; /* body omitted */
public:
    U8          castling_rights_;
    U8          half_moves_;
    U8          en_passant_sq_;
    Piece       captured_type_;
    ZobristKey  zobrist_key_;
};

struct PositionTestProbe;

class Position final : public Board
{
public:
    constexpr Position()
; /* body omitted */

    Position(std::string_view fen)
; /* body omitted */
    void Reset()
    ; /* body omitted */

    void ImportFen(std::string_view fen);

    void MakeMove(Move m);

    void UnmakeMove(Move m);

    constexpr BitBoard EmptySqs()const ; /* body omitted */

    constexpr BitBoard EnPasBB()const ; /* body omitted */

    constexpr Sq EnPasSq()const ; /* body omitted */

    constexpr U8 CastlingRights()const ; /* body omitted */

    constexpr Colour ColourToMove()const ; /* body omitted */

    constexpr ZobristKey ZKey()const ; /* body omitted */

    constexpr U8 HalfMoves()const ; /* body omitted */

    constexpr U16 FullMoves()const ; /* body omitted */

    bool IsOk() const
    ; /* body omitted */
    ZobristKey HashCurrentPostion();

private:
    friend struct PositionTestProbe;

    void UpdateCastlingRights();
private:
    StateInfo info_;
    Colour turn_;
    U16 full_moves_;
    std::vector<StateInfo> previous_state_info;
};
```

## `src/Search.hpp`

```cpp
#define SEARCH_HPP <replacement omitted>

class Search
{
public:

    Score GoSearch(TransposTable* __restrict__  tt, Position* __restrict__  pos, U16 depth, TimeManager const* tm, Score a = Eval::NEG_INF, Score b = Eval::POS_INF);

    Move FindBestMove(Position* __restrict__  pos, TransposTable* __restrict__  tt, TimeManager const* __restrict__ tm);
private:
};
```

## `src/Testing.hpp`

```cpp
#define TESTING_HPP <replacement omitted>

#define TESTFEN1 <replacement omitted>
#define TESTFEN2 <replacement omitted>
#define TESTFEN3 <replacement omitted>
#define TESTFEN4 <replacement omitted>
#define TESTFEN5 <replacement omitted>
#define TESTFEN6 <replacement omitted>
#define TESTFEN7 <replacement omitted>
#define TESTFEN8 <replacement omitted>
#define TESTFEN9 <replacement omitted>
#define TESTFEN10 <replacement omitted>

#define STARTPOS <replacement omitted>
#define KIWIPETE <replacement omitted>
#define PERFTPOS3 <replacement omitted>
#define PERFTPOS4 <replacement omitted>
#define PERFTPOS5 <replacement omitted>
#define PERFTPOS6 <replacement omitted>
#define TRICKYENDGAMEPOS <replacement omitted>
#define PERPETUALCHECK <replacement omitted>
class PerftHandler
{
public:
    PerftHandler()
; /* body omitted */

    void ResetData(); /* body omitted */

    template<bool output_perft_paths>
    void RunPerft(int depth, Position* pos); /* body omitted */
    template<bool output_perft_paths>
    void RunBulkPerft(int depth, Position* pos); /* body omitted */

    U64 GetNodes() const ; /* body omitted */

    void PrintData()
    ; /* body omitted */
private:
    template<bool is_root, bool output_perft_paths>
    U64 Perft(int depth, Position* pos)
    ; /* body omitted */
    template<bool is_root, bool output_perft_paths>
    U64 BulkPerft(int depth, Position* pos)
    ; /* body omitted */
private:
    std::vector<std::string> perft_data_;
    U64 total_nodes_;
};
inline void TestSearch()
; /* body omitted */

template<bool output_perft_paths>
U64 TestPerft(unsigned depth, U64 expected_nodes, U16 test_number, const std::string& fen)
; /* body omitted */
template<bool output_perft_paths>
U64 TestBulkPerft(unsigned depth, U64 expected_nodes, U16 test_number, const std::string& fen)
; /* body omitted */
template<bool output_perft_paths>
void RunBenchmark()
; /* body omitted */
template<bool output_perft_paths>
void RunBulkBenchmark()
; /* body omitted */
static std::vector<std::string> Split(const std::string& line, const std::string& delimiter)
; /* body omitted */
template<bool output_perft_paths>
bool RunPerftSuite()
; /* body omitted */
```

## `src/Timer.hpp`

```cpp
#define TIMER_HPP <replacement omitted>

template<typename T>
class Timer
{
public:
    constexpr Timer()
; /* body omitted */

    constexpr Timer(U64* ptr)
; /* body omitted */

    constexpr ~Timer()
    ; /* body omitted */
private:
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    U64* time_out;
};
class TimeManager
{
private:

    U64 GetTimeAllowance()const
    ; /* body omitted */
public:
    void SetOptions(U64 time, U64 increment)
    ; /* body omitted */
    void StartTiming()
    ; /* body omitted */
    bool OutOfTime()const ; /* body omitted */

private:
    U64 our_time_;
    U64 our_increment_;
    std::chrono::time_point<std::chrono::steady_clock> end_;
};
```

## `src/TranspositionTable.hpp`

```cpp
#define TRANSPOSITIONTABLE_HPP <replacement omitted>

struct HashEntry
{
    ZobristKey key_;
    Score eval_;
    Move best_;
    U8 depth_;
    BoundType bound_;
    U8 padding_[2];
};
static_assert(sizeof(HashEntry) == 16);
class TransposTable
{
public:
    TransposTable()
; /* body omitted */
    ~TransposTable(); /* body omitted */;
    void Resize(size_t size_in_mB);
    void Store(ZobristKey, Score, Move, U8, BoundType) const;
    [[nodiscard]] HashEntry const* Probe(ZobristKey)const;
    void Clear() const;
    [[nodiscard]] size_t GetNumElems()const; /* body omitted */
private:
    size_t num_elements_;
    HashEntry* table_ptr_;
};
```

## `src/Types.hpp`

```cpp
#define TYPES_HPP <replacement omitted>

#define USE_TITBOARDS <replacement omitted>
#define USE_TRANSPOSITION_TABLE <replacement omitted>
#define DEBUG_TRANPOSITION_TABLE <replacement omitted>
#define TDFA_DEBUG <replacement omitted>

#define NDEBUG <replacement omitted>
#define _AT <replacement omitted>
#define _AT <replacement omitted>

#define INLINE <replacement omitted>
#define INLINE <replacement omitted>

using U8  = unsigned char;
using U16 = unsigned short;
using U32 = unsigned int;
using U64 = unsigned long long;
using I16 = short;

using Move      = U16;
using BitBoard  = U64;

using Sq        = U8;
using Castling  = U8;
using Score     = I16;

constexpr std::size_t MAX_MOVES = <initializer omitted>;

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
    constexpr move_info()
; /* body omitted */
    inline constexpr void add_move(const Move m) noexcept ; /* body omitted */

    std::array<Move, 7> encoded_move_;
    U8 count_;
    BitBoard attacks_;
};
namespace loc
{
    constexpr U8 BLACK = <initializer omitted>;
    constexpr U8 WHITE = <initializer omitted>;
    constexpr U8 KING  = <initializer omitted>;
    constexpr U8 QUEEN = <initializer omitted>;
    constexpr U8 BISHOP= <initializer omitted>;
    constexpr U8 KNIGHT= <initializer omitted>;
    constexpr U8 ROOK  = <initializer omitted>;
    constexpr U8 PAWN  = <initializer omitted>;
}
enum class PromType : U8
{
    NOPROMO,
    QUEEN,
    BISHOP,
    KNIGHT,
    ROOK
};
enum PieceType
{
    pt_begin_it,
    pt_King = <initializer omitted>,
    pt_Queen,
    pt_Bishop,
    pt_Knight,
    pt_Rook,
    pt_end_it,
    pt_Pawn = <initializer omitted>,
    pt_All,
    pt_None,
    pt_prom_queen = <initializer omitted>,
    pt_prom_bishop,
    pt_prom_knight,
    pt_prom_rook,
};
enum MoveType
{
    mt_Quiet = <initializer omitted>,
    mt_EnPassant,
    mt_Castling,
    mt_Capture,
    mt_Promotion = <initializer omitted>,
    mt_QueenPromotion = <initializer omitted>,
    mt_BishopPromotion,
    mt_KnightPromotion,
    mt_RookPromotion
};
enum Colour
{
    White,
    Black
};
constexpr Colour operator!(const Colour c)
; /* body omitted */
enum Piece : U8
{
    p_WhiteKing = <initializer omitted>,
    p_WhiteQueen,
    p_WhiteBishop,
    p_WhiteKnight,
    p_WhiteRook,
    p_WhitePawn,
    p_BlackKing = <initializer omitted>,
    p_BlackQueen,
    p_BlackBishop,
    p_BlackKnight,
    p_BlackRook,
    p_BlackPawn,
    p_None
};

enum AttackDirection : U8
{
    File,
    Rank,
    Diagonal,
    AntiDiagonal
};
enum class BoundType : U8
{
    EXACT_VAL,                      
    UPPER_BOUND,                           
    LOWER_BOUND                              
};
template<typename T>
float FloatDiv(T dividend, T divisor)
; /* body omitted */
```

## `src/Uci.hpp`

```cpp
#define UCI_HPP <replacement omitted>

using CmdMap = std::unordered_map<std::string_view, U8>;
using ArgList = std::vector<std::string_view>;
class Uci
{
public:
    Uci()
; /* body omitted */
    void Loop();
private:

    void HandleUci();
    void HandleIsReady();
    void HandleGo(const ArgList&);
    void HandlePosition(const ArgList&);

    static void HandleStop();
    void HandleNewGame();
    void HandleSetOption(const ArgList&);
    static void HandleBench(const ArgList&);
    void HandlePrint(const ArgList&);
private:

    size_t tt_size_;
    Position pos_;    
    TransposTable tt_;
    TimeManager time_manager_;
    Search search_;
private:

    static constexpr const char* ENGINE_NAME = <initializer omitted>;
    static constexpr const char* ENGINE_AUTHOR = <initializer omitted>;
    static inline const CmdMap COMMAND_VALUES = <initializer omitted>;
};
```

## `src/Util.hpp`

```cpp
#define UTIL_HPP <replacement omitted>

namespace UTIL
{
    inline std::string Square(Sq sq)
    ; /* body omitted */
    inline char PromotionChar(PieceType p)
    ; /* body omitted */
    inline std::string MoveToStr(const Move m)
    ; /* body omitted */
    inline Move UciToMove(const std::string_view str, const Position& pos)
    ; /* body omitted */
}
```

## `src/ZobristConstants.hpp`

```cpp
#define ZOBRISTSCONSTANTS_HPP <replacement omitted>

using ZobristKey = U64;
using PieceZobArr = std::array<std::array<std::array<ZobristKey, 64>, 6>, 2>;
using EnPZobArr =  std::array<ZobristKey, 64>;
using CastlingZobArr = std::array<ZobristKey, 16>;

namespace Zobrist
{
    namespace _
    {
        struct tmp
        {
            PieceZobArr pieces;
            EnPZobArr en_passant;
            CastlingZobArr castling;
            ZobristKey side_to_move;
        };
        constexpr U64 ZobRand64(U64& s)
        ; /* body omitted */
        consteval PieceZobArr InitPieces(U64& prng_seed)
        ; /* body omitted */
        consteval EnPZobArr InitEnPassant(U64& prng_seed)
        ; /* body omitted */
        consteval CastlingZobArr InitCastling(U64& prng_seed)
        ; /* body omitted */
        consteval tmp InitTmp()
        ; /* body omitted */
    }

    constexpr inline  _::tmp t; /* body omitted */;
    constexpr inline PieceZobArr    const&  PIECES; /* body omitted */;
    constexpr inline EnPZobArr      const&  EN_PASSANT; /* body omitted */;
    constexpr inline CastlingZobArr const&  CASTLING; /* body omitted */;
    constexpr inline ZobristKey     const&  SIDE_TO_MOVE; /* body omitted */;
}
```

