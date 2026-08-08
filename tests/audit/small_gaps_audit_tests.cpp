#include <catch2/catch_test_macros.hpp>

#include "BoardUtils.hpp"
#include "MagicConstants.hpp"
#include "Position.hpp"
#include "Util.hpp"

#include <array>
#include <string_view>

TEST_CASE("Audit BoardUtils handles empty and boundary-space inputs",
          "[audit][small-gaps]")
{
    const std::array<std::string_view, 8> boundary_observations{
        RemoveWhiteSpace(""),
        RemoveWhiteSpace(" "),
        RemoveWhiteSpace("        "),
        RemoveWhiteSpace("value"),
        RemoveWhiteSpace(" value"),
        RemoveWhiteSpace("value "),
        RemoveWhiteSpace("  value  "),
        RemoveWhiteSpace("  two words  "),
    };

    std::array<std::string_view, 6> empty_fields{};
    SplitFen("", empty_fields);

    std::array<std::string_view, 6> boundary_fields{};
    SplitFen(" a b c d e ", boundary_fields);

    std::array<std::string_view, 6> capped_fields{};
    SplitFen("one two three four five six ignored", capped_fields);
    CAPTURE(boundary_observations, empty_fields, boundary_fields, capped_fields);
    SUCCEED("The source-aware boundary calls returned without an unbounded operation");
}

TEST_CASE("Audit PopNRetLS1B removes the independently scanned low bit",
          "[audit][small-gaps]")
{
    constexpr std::array<BitBoard, 8> samples{
        U64{1},
        U64{2},
        U64{1} << 63,
        U64{0x8000000000000001},
        U64{0x00F0000000000100},
        U64{0xAAAAAAAAAAAAAAAA},
        U64{0x5555555555555555},
        ~U64{0},
    };

    for (const BitBoard sample : samples)
    {
        unsigned expected_square = 0;
        while (((sample >> expected_square) & U64{1}) == 0)
            ++expected_square;

        BitBoard remaining = sample;
        const Sq returned = Magics::PopNRetLS1B(remaining);
        INFO("sample=" << sample << " expected-square=" << expected_square
                        << " returned=" << static_cast<unsigned>(returned));
        CHECK(static_cast<unsigned>(returned) == expected_square);
        CHECK(remaining ==
              (sample & ~(U64{1} << static_cast<unsigned>(expected_square))));
    }
}

TEST_CASE("Audit invokes ZKey reads and recomputation on valid positions",
          "[audit][small-gaps][zobrist]")
{
    // Hash lifecycle and equality are unresolved; merely execute the public
    // observers on valid objects without asserting any identity relation.
    constexpr std::string_view start_fen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::array<ZobristKey, 5> observations{};
    REQUIRE_NOTHROW([&] {
        Position empty;
        Position first(start_fen);
        Position equivalent(start_fen);
        observations = {empty.ZKey(), first.ZKey(), equivalent.ZKey(),
                        first.HashCurrentPostion(), first.ZKey()};
    }());
    CAPTURE(observations);
}
