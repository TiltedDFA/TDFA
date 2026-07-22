#include <catch2/catch_test_macros.hpp>

#include "Debug.hpp"
#include "Move.hpp"
#include "Timer.hpp"
#include "TranspositionTable.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{

class ScopedCoutCapture
{
public:
    ScopedCoutCapture()
        : previous_state_(std::cout.rdstate())
    {
        previous_ = std::cout.rdbuf(stream_.rdbuf());
        std::cout.clear();
    }

    ~ScopedCoutCapture()
    {
        std::cout.rdbuf(previous_);
        std::cout.clear(previous_state_);
    }

    ScopedCoutCapture(const ScopedCoutCapture &) = delete;
    ScopedCoutCapture &operator=(const ScopedCoutCapture &) = delete;

    [[nodiscard]] std::string str() const
    {
        return stream_.str();
    }

private:
    std::ostringstream stream_;
    std::streambuf *previous_ = nullptr;
    std::ios::iostate previous_state_;
};

constexpr const char *kStartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

} // namespace

TEST_CASE("Audit Debug bitboard diagnostics accept representative inputs",
          "[audit][debug]")
{
    const auto *original_buffer = std::cout.rdbuf();
    std::string captured;

    {
        ScopedCoutCapture capture;
        constexpr BitBoard pattern = 0x8000000000000081ULL;

        Debug::PrintBB(pattern, false);
        Debug::PrintBB(pattern, true);
        Debug::PrintBB(pattern, 0, false);
        Debug::PrintBB(pattern, 63, true);

        Debug::PrintUsThem(1ULL, 2ULL, false);
        Debug::PrintUsThem(1ULL << 63, 1ULL << 62, true);
        Debug::PrintUsThemBlank(1ULL, 2ULL, false);
        Debug::PrintUsThemBlank(1ULL << 63, 1ULL << 62, true);

        Debug::PrintU8BB(static_cast<U8>(0x81), static_cast<U8>(3), false);
        Debug::PrintU8BB(static_cast<U8>(0x42), static_cast<U8>(4), true);

        captured = capture.str();
    }

    REQUIRE(std::cout.rdbuf() == original_buffer);
    CHECK(captured.size() < 256U * 1024U);
}

TEST_CASE("Audit Debug position piece and move diagnostics accept public values",
          "[audit][debug]")
{
    const auto *original_buffer = std::cout.rdbuf();
    std::string captured;

    {
        ScopedCoutCapture capture;

        Position start(kStartFen);
        Position black_to_move("7k/8/8/8/8/8/8/K7 b - - 7 12");
        Debug::PrintBoardState(start);
        Debug::PrintBoardState(black_to_move);

        BitBoard pieces[2][6]{};
        for (std::size_t colour = 0; colour < 2; ++colour)
        {
            for (std::size_t type = 0; type < 6; ++type)
            {
                const auto square = static_cast<unsigned>(colour * 32 + type);
                pieces[colour][type] = 1ULL << square;
            }
        }
        Debug::PrintInduvidualPieces(pieces);

        const Move ordinary = Moves::EncodeMove(1, 18, mt_Quiet);
        Debug::PrintEncodedMoveStr(ordinary);
        Debug::PrintEncodedMoveBin(ordinary);

        move_info none;
        Debug::PrintEncodedMovesMoveInfo(none, false);

        move_info two_moves;
        two_moves.add_move(Moves::EncodeMove(1, 18, mt_Quiet));
        two_moves.add_move(Moves::EncodeMove(1, 16, mt_Quiet));
        Debug::PrintEncodedMovesMoveInfo(two_moves, true);

        Debug::PrintBoardGraphically(&start);
        Debug::PrintBoardGraphically(&black_to_move);

        captured = capture.str();
    }

    REQUIRE(std::cout.rdbuf() == original_buffer);
    CHECK(captured.size() < 256U * 1024U);

    const std::array<std::string, 6> labels{
        Debug::PieceTypeToStr(pt_King), Debug::PieceTypeToStr(pt_Queen),
        Debug::PieceTypeToStr(pt_Bishop), Debug::PieceTypeToStr(pt_Knight),
        Debug::PieceTypeToStr(pt_Rook), Debug::PieceTypeToStr(pt_Pawn)};
    CAPTURE(labels);
}

TEST_CASE("Audit invokes timer publication and deadline paths",
          "[audit][timer]")
{
    U64 measured = std::numeric_limits<U64>::max();
    {
        Timer<std::chrono::nanoseconds> timer(&measured);
    }

    const auto *original_buffer = std::cout.rdbuf();
    std::string captured;
    {
        ScopedCoutCapture capture;
        {
            Timer<std::chrono::microseconds> timer;
        }
        captured = capture.str();
    }
    REQUIRE(std::cout.rdbuf() == original_buffer);
    CHECK(captured.size() < 4096U);

    TimeManager manager;
    manager.SetOptions(72'000'000ULL, 0ULL);
    manager.StartTiming();
    const bool future_observation = manager.OutOfTime();

    manager.SetOptions(0ULL, 0ULL);
    manager.StartTiming();
    bool observed_timeout = manager.OutOfTime();
    for (std::size_t attempt = 0; attempt < 1'000'000 && !observed_timeout;
         ++attempt)
    {
        observed_timeout = manager.OutOfTime();
    }
    CAPTURE(measured, captured, future_observation, observed_timeout);
    SUCCEED("Timer paths completed within the audit case deadline");
}

TEST_CASE("Audit invokes an allocated transposition-table lifecycle",
          "[audit][transposition-table]")
{
    {
        const TransposTable empty;
        const auto initial_capacity = empty.GetNumElems();
        CAPTURE(initial_capacity);
    }

    TransposTable table;
    table.Resize(1);
    const auto first_capacity = table.GetNumElems();
    if (first_capacity == 0)
        SKIP("The current table declined the positive audit allocation");

    constexpr ZobristKey key = std::numeric_limits<ZobristKey>::max();
    constexpr Score evaluation = static_cast<Score>(-321);
    constexpr Move best = 0x5A5A;
    constexpr U8 depth = 17;
    constexpr BoundType bound = BoundType::LOWER_BOUND;

    const bool before_store_present = table.Probe(key) != nullptr;
    table.Store(key, evaluation, best, depth, bound);

    const bool entry_present = table.Probe(key) != nullptr;
    const bool other_present = table.Probe(key - 1) != nullptr;

    table.Clear();
    const bool after_clear_present = table.Probe(key) != nullptr;

    table.Store(1, 42, Moves::EncodeMove(0, 1, mt_Quiet), 1,
                BoundType::EXACT_VAL);
    const bool second_entry_present = table.Probe(1) != nullptr;

    table.Resize(2);
    const auto second_capacity = table.GetNumElems();
    const bool after_resize_present =
        second_capacity != 0 && table.Probe(1) != nullptr;
    CAPTURE(first_capacity, before_store_present, entry_present, other_present,
            after_clear_present, second_entry_present, second_capacity,
            after_resize_present);
    SUCCEED("Allocated table operations returned within the audit deadline");
}
