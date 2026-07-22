#include <catch2/catch_test_macros.hpp>

#include "Testing.hpp"
#include "support/BlindFixtureData.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>

namespace
{

struct PerftOracle
{
    std::string_view id;
    std::string_view fen;
    std::array<U64, 3> nodes;
};

// Frozen independently generated chess-oracle values.  They are deliberately
// literals here: the ordinary implementation is not checked against BulkPerft
// or against another result computed by the engine under test.
constexpr std::array<PerftOracle, 6> kPerftOracles{
    PerftOracle{"start",
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                {U64{20}, U64{400}, U64{8902}}},
    PerftOracle{
        "kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        {U64{48}, U64{2039}, U64{97862}}},
    PerftOracle{"perft_endgame",
                "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                {U64{14}, U64{191}, U64{2812}}},
    PerftOracle{
        "perft_promotions",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        {U64{6}, U64{264}, U64{9467}}},
    PerftOracle{"perft_castling",
                "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                {U64{44}, U64{1486}, U64{62379}}},
    PerftOracle{
        "perft_tactics",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        {U64{46}, U64{2079}, U64{89890}}},
};

struct PublicPositionSnapshot
{
    std::array<Piece, 64> squares{};
    std::array<BitBoard, 2> colours{};
    std::array<BitBoard, 7> types{};
    Colour turn = White;
    U8 castling = 0;
    std::optional<Sq> en_passant_square;
    BitBoard en_passant_board = 0;
    U8 half_moves = 0;
    U16 full_moves = 0;

    friend bool operator==(const PublicPositionSnapshot &,
                           const PublicPositionSnapshot &) = default;
};

PublicPositionSnapshot snapshot(const Position &position)
{
    PublicPositionSnapshot result;
    for (unsigned square = 0; square < result.squares.size(); ++square)
        result.squares[square] = position.PieceOn(static_cast<Sq>(square));
    result.colours = {position.Pieces(White), position.Pieces(Black)};
    for (unsigned type = pt_King; type <= pt_All; ++type)
        result.types[type] = position.Pieces(static_cast<PieceType>(type));
    result.turn = position.ColourToMove();
    result.castling = position.CastlingRights();
    result.en_passant_board = position.EnPasBB();
    if (result.en_passant_board != 0)
        result.en_passant_square = position.EnPasSq();
    result.half_moves = position.HalfMoves();
    result.full_moves = position.FullMoves();
    return result;
}

void require_perft_oracle_source()
{
    using namespace tdfa_test::blind;
    constexpr std::string_view path = "tests/data/oracle_perft.tsv";
    constexpr std::string_view file_hash =
        "96a933a8203be5095a7e8592cf09b56a27f1602df4ec8795dd7aa6f3ca38c849";
    constexpr std::string_view data_hash =
        "8f7643f4c8168c93b0590634e17ea11db52ddfe167e7c04d04710cb986f40789";

    const std::string whole_file = read_fixture_bytes(path);
    REQUIRE(sha256_hex(whole_file) == file_hash);
    const auto fixture = load_tsv_fixture(path);
    REQUIRE(fixture.metadata.at("data_sha256") == data_hash);
    const auto rows = oracle_perft_rows(fixture);
    REQUIRE(rows.size() == 18);

    using Key = std::pair<std::string, unsigned>;
    std::map<Key, std::pair<std::string, std::uint64_t>> indexed;
    for (const auto &row : rows)
        REQUIRE(indexed.emplace(Key{row.case_id, row.depth},
                                std::pair{row.fen, row.nodes})
                    .second);

    for (const auto &expected : kPerftOracles)
    {
        for (unsigned depth = 1; depth <= 3; ++depth)
        {
            const auto found = indexed.find(
                Key{std::string(expected.id), depth});
            REQUIRE(found != indexed.end());
            REQUIRE(found->second.first == expected.fen);
            REQUIRE(found->second.second == expected.nodes[depth - 1]);
        }
    }
}

class BoundedStreamBuf final : public std::streambuf
{
public:
    explicit BoundedStreamBuf(const std::size_t limit) : limit_(limit) {}

    [[nodiscard]] const std::string &retained() const { return retained_; }
    [[nodiscard]] std::size_t total_bytes() const { return total_bytes_; }

protected:
    int_type overflow(const int_type value) override
    {
        if (traits_type::eq_int_type(value, traits_type::eof()))
            return traits_type::not_eof(value);
        const char character = traits_type::to_char_type(value);
        append(&character, 1);
        return value;
    }

    std::streamsize xsputn(const char *characters,
                           const std::streamsize count) override
    {
        if (count > 0)
            append(characters, static_cast<std::size_t>(count));
        return count;
    }

private:
    void append(const char *characters, const std::size_t count)
    {
        total_bytes_ += count;
        const std::size_t available = limit_ - retained_.size();
        retained_.append(characters, std::min(available, count));
    }

    std::size_t limit_;
    std::size_t total_bytes_ = 0;
    std::string retained_;
};

class BoundedCoutCapture final
{
public:
    explicit BoundedCoutCapture(const std::size_t limit)
        : buffer_(limit), previous_state_(std::cout.rdstate())
    {
        previous_buffer_ = std::cout.rdbuf(&buffer_);
        std::cout.clear();
    }

    ~BoundedCoutCapture()
    {
        std::cout.rdbuf(previous_buffer_);
        std::cout.clear(previous_state_);
    }

    BoundedCoutCapture(const BoundedCoutCapture &) = delete;
    BoundedCoutCapture &operator=(const BoundedCoutCapture &) = delete;

    [[nodiscard]] const std::string &retained() const
    {
        return buffer_.retained();
    }
    [[nodiscard]] std::size_t total_bytes() const
    {
        return buffer_.total_bytes();
    }

private:
    BoundedStreamBuf buffer_;
    std::streambuf *previous_buffer_ = nullptr;
    std::ios::iostate previous_state_;
};

class ScopedOwnedWorkingDirectory final
{
public:
    ScopedOwnedWorkingDirectory() : original_(std::filesystem::current_path())
    {
        static std::atomic<unsigned long long> sequence{0};
        const auto base = std::filesystem::temp_directory_path();
        const auto clock_token = std::chrono::steady_clock::now()
                                     .time_since_epoch()
                                     .count();

        for (unsigned attempt = 0; attempt < 100; ++attempt)
        {
            const auto name =
                std::string{"tdfa-owned-perft-audit-"} +
                std::to_string(clock_token) + "-" +
                std::to_string(sequence.fetch_add(1)) + "-" +
                std::to_string(attempt);
            const auto candidate = base / name;
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error))
            {
                owned_ = candidate;
                std::error_code change_error;
                std::filesystem::current_path(owned_, change_error);
                if (change_error)
                {
                    std::error_code cleanup_error;
                    std::filesystem::remove_all(owned_, cleanup_error);
                    owned_.clear();
                    throw std::runtime_error(
                        "cannot enter isolated perft directory: " +
                        change_error.message());
                }
                return;
            }
            if (error && error != std::errc::file_exists)
                throw std::runtime_error("cannot create isolated perft directory: " +
                                         error.message());
        }
        throw std::runtime_error("cannot reserve a unique perft audit directory");
    }

    ~ScopedOwnedWorkingDirectory()
    {
        std::error_code restore_error;
        std::filesystem::current_path(original_, restore_error);
        if (!restore_error && !owned_.empty())
        {
            std::error_code cleanup_error;
            std::filesystem::remove_all(owned_, cleanup_error);
        }
    }

    ScopedOwnedWorkingDirectory(const ScopedOwnedWorkingDirectory &) = delete;
    ScopedOwnedWorkingDirectory &
    operator=(const ScopedOwnedWorkingDirectory &) = delete;

private:
    std::filesystem::path original_;
    std::filesystem::path owned_;
};

} // namespace

TEST_CASE("Audit ordinary perft matches frozen depths and restores public FEN semantics",
          "[audit][perft]")
{
    require_perft_oracle_source();
    for (const auto &oracle : kPerftOracles)
    {
        DYNAMIC_SECTION("fixture=" << oracle.id)
        {
            Position position(oracle.fen);
            const PublicPositionSnapshot root = snapshot(position);
            PerftHandler handler;

            for (int depth = 1; depth <= 3; ++depth)
            {
                handler.RunPerft<false>(depth, &position);
                const U64 expected =
                    oracle.nodes[static_cast<std::size_t>(depth - 1)];
                INFO("fixture=" << oracle.id << " depth=" << depth
                                 << " expected=" << expected
                                 << " actual=" << handler.GetNodes());
                CHECK(handler.GetNodes() == expected);
                REQUIRE(snapshot(position) == root);
            }
        }
    }
}

TEST_CASE("Audit legacy perft reporting and splitting stay bounded",
          "[audit][perft][legacy]")
{
    constexpr std::size_t kOutputLimit = 64 * 1024;
    BoundedCoutCapture output(kOutputLimit);

    const U64 passing_nps = TestPerft<true>(2, U64{400}, U16{501}, STARTPOS);
    const U64 failing_nps = TestPerft<false>(2, U64{401}, U16{502}, STARTPOS);
    (void)passing_nps;
    (void)failing_nps;

    const auto ordinary = Split("alpha;beta;gamma;", ";");
    const auto without_delimiter = Split("no delimiter", ";");
    const auto empty_segments = Split(";leading;;", ";");
    CAPTURE(ordinary, without_delimiter, empty_segments);

    CHECK(output.total_bytes() < kOutputLimit);
    CAPTURE(output.retained());
}

TEST_CASE("Audit legacy perft suite owns its working file and bounded output",
          "[audit][perft][legacy]")
{
    constexpr std::size_t kOutputLimit = 64 * 1024;
    ScopedOwnedWorkingDirectory working_directory;

    {
        BoundedCoutCapture missing_output(kOutputLimit);
        const bool missing_result = RunPerftSuite<false>();
        CHECK(missing_output.total_bytes() < kOutputLimit);
        CAPTURE(missing_result, missing_output.retained());
    }

    {
        std::ofstream suite("perftsuite.epd", std::ios::binary | std::ios::trunc);
        REQUIRE(suite.is_open());
        suite << "# isolated audit fixture\n"
              << STARTPOS
              << " ;D1 20 ;D2 400 ;D3 100000001 ;\n";
        suite.close();
        REQUIRE(suite.good());
    }

    BoundedCoutCapture output(kOutputLimit);
    const bool suite_result = RunPerftSuite<false>();
    CHECK(output.total_bytes() < kOutputLimit);
    CAPTURE(suite_result, output.retained());
}
