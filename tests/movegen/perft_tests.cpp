#include <catch2/catch_test_macros.hpp>

#include "../support/BlindChessModel.hpp"
#include "../support/BlindFixtureData.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

using namespace tdfa_test::blind;

struct EpdRow {
    std::size_t physical_line{};
    std::string bytes;
    std::string fen;
    std::map<unsigned, std::uint64_t> counts;
};

std::uint64_t parse_u64_exact(const std::string& token) {
    if (token.empty() || token.front() == '-' || token.front() == '+') {
        throw std::runtime_error("EPD count is not an unsigned decimal");
    }
    for (const char value : token) {
        if (value < '0' || value > '9') throw std::runtime_error("EPD count contains non-digit");
    }
    std::size_t consumed = 0;
    const unsigned long long parsed = std::stoull(token, &consumed);
    if (consumed != token.size()) throw std::runtime_error("EPD count suffix");
    return static_cast<std::uint64_t>(parsed);
}

std::vector<EpdRow> parse_epd(const std::string& bytes) {
    if (bytes.empty() || bytes.back() != '\n') throw std::runtime_error("EPD must end in LF");
    std::vector<EpdRow> rows;
    std::size_t cursor = 0;
    std::size_t line_number = 1;
    while (cursor < bytes.size()) {
        const std::size_t newline = bytes.find('\n', cursor);
        if (newline == std::string::npos) throw std::runtime_error("EPD missing final LF");
        std::string line = bytes.substr(cursor, newline - cursor);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.find('\r') != std::string::npos) throw std::runtime_error("blank/embedded-CR EPD line");
        const std::size_t operations = line.find(" ;D");
        if (operations == std::string::npos) throw std::runtime_error("EPD missing depth operations");
        EpdRow row;
        row.physical_line = line_number;
        row.bytes = line;
        row.fen = line.substr(0, operations);
        if (RefPosition::from_fen(row.fen).fen() != row.fen) throw std::runtime_error("noncanonical EPD FEN");
        std::size_t segment_begin = operations + 1;
        while (segment_begin < line.size()) {
            if (line[segment_begin] != ';') throw std::runtime_error("EPD operation missing semicolon");
            ++segment_begin;
            const std::size_t next = line.find(';', segment_begin);
            std::string segment = line.substr(segment_begin,
                next == std::string::npos ? std::string::npos : next - segment_begin);
            while (!segment.empty() && segment.back() == ' ') segment.pop_back();
            if (segment.size() < 4 || segment.front() != 'D') throw std::runtime_error("unknown EPD operation");
            const std::size_t separator = segment.find(' ');
            if (separator == std::string::npos || separator == 1 || segment.find(' ', separator + 1) != std::string::npos) {
                throw std::runtime_error("malformed EPD depth operation");
            }
            const std::string depth_text = segment.substr(1, separator - 1);
            for (const char value : depth_text) {
                if (value < '0' || value > '9') throw std::runtime_error("non-numeric EPD depth");
            }
            const unsigned long depth_value = std::stoul(depth_text);
            if (depth_value == 0 || depth_value > 6) throw std::runtime_error("unknown EPD depth tag");
            const auto count = parse_u64_exact(segment.substr(separator + 1));
            if (!row.counts.emplace(static_cast<unsigned>(depth_value), count).second) {
                throw std::runtime_error("duplicate EPD depth tag");
            }
            if (next == std::string::npos) break;
            segment_begin = next;
        }
        if (row.counts.empty()) throw std::runtime_error("EPD row has no counts");
        rows.push_back(std::move(row));
        cursor = newline + 1;
        ++line_number;
    }
    return rows;
}

} // namespace

TEST_CASE("T-PERFT-001 Compact perft corpus format checksum and complete row ingestion",
          "[movegen][perft][T-PERFT-001][fixture][regression]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_perft.tsv");
    const std::vector<std::string> expected_columns{"case_id","fen","depth","nodes"};
    REQUIRE(fixture.columns == expected_columns);
    REQUIRE(fixture.metadata.at("format") == "tdfa-chess-oracle-v1");
    REQUIRE(fixture.metadata.at("profile") == "committed");
    REQUIRE(fixture.metadata.at("generation_args") == "--profile committed");
    REQUIRE(fixture.metadata.at("position_count") == "6");
    REQUIRE(fixture.metadata.at("max_depth") == "3");
    REQUIRE(fixture.metadata.at("row_count") == "18");
    REQUIRE(fixture.metadata.at("checksum_scope") == "column-header-and-data-rows-with-lf");
    const std::string expected_hash = "8f7643f4c8168c93b0590634e17ea11db52ddfe167e7c04d04710cb986f40789";
    REQUIRE(fixture.metadata.at("data_sha256") == expected_hash);
    REQUIRE(sha256_hex(fixture.scoped_bytes) == expected_hash);

    const auto rows = oracle_perft_rows(fixture);
    REQUIRE(rows.size() == 18);
    std::map<std::string, std::set<unsigned>> depths_by_case;
    std::set<std::pair<std::string, unsigned>> keys;
    std::map<std::string, std::string> fen_by_case;
    for (std::size_t physical = 0; physical < rows.size(); ++physical) {
        const auto& row = rows[physical];
        INFO("physical-data-row=" << physical + 1 << " case_id=" << row.case_id
             << " depth=" << row.depth << " nodes=" << row.nodes);
        REQUIRE(row.depth >= 1);
        REQUIRE(row.depth <= 3);
        REQUIRE(row.nodes > 0);
        REQUIRE(keys.emplace(row.case_id, row.depth).second);
        depths_by_case[row.case_id].insert(row.depth);
        const auto [iterator, inserted] = fen_by_case.emplace(row.case_id, row.fen);
        REQUIRE((inserted || iterator->second == row.fen));
        REQUIRE(RefPosition::from_fen(row.fen).fen() == row.fen);
    }
    const std::set<unsigned> expected_depths{1,2,3};
    REQUIRE(depths_by_case.size() == 6);
    for (const auto& [case_id, depths] : depths_by_case) {
        INFO("case_id=" << case_id);
        REQUIRE(depths == expected_depths);
    }
    const std::set<std::string> expected_cases{
        "start","kiwipete","perft_endgame","perft_promotions","perft_castling","perft_tactics"};
    std::set<std::string> actual_cases;
    for (const auto& [case_id, depths] : depths_by_case) actual_cases.insert(case_id);
    REQUIRE(actual_cases == expected_cases);
}

TEST_CASE("T-PERFT-003 EPD suite checksum parser cardinality and depth completeness",
          "[movegen][perft][T-PERFT-003][fixture][exhaustive][regression]") {
    const std::string bytes = read_fixture_bytes("perftsuite.epd");
    REQUIRE(sha256_hex(bytes) == "363e5205dfff3205fc50b376fb1770f9156464456282372c8c0a0e5b45a0bf07");
    const auto rows = parse_epd(bytes);
    REQUIRE(rows.size() == 126);

    std::map<std::string, std::vector<std::size_t>> physical_lines_by_bytes;
    std::map<unsigned, std::size_t> physical_depth_rows;
    std::map<unsigned, std::uint64_t> physical_totals;
    std::set<std::string> unique_fens;
    std::map<std::string, std::map<unsigned, std::uint64_t>> unique;
    std::size_t physical_expectations = 0;
    for (const auto& row : rows) {
        INFO("physical-line=" << row.physical_line << " fen=" << row.fen);
        physical_lines_by_bytes[row.bytes].push_back(row.physical_line);
        unique_fens.insert(row.fen);
        ++physical_expectations;
        physical_expectations += row.counts.size() - 1;
        for (const auto& [depth, count] : row.counts) {
            ++physical_depth_rows[depth];
            physical_totals[depth] += count;
            const auto [iterator, inserted] = unique[row.fen].emplace(depth, count);
            REQUIRE((inserted || iterator->second == count));
        }
    }
    REQUIRE(unique_fens.size() == 125);
    REQUIRE(physical_expectations == 755);

    std::size_t unique_expectations = 0;
    std::map<unsigned, std::size_t> unique_depth_rows;
    std::map<unsigned, std::uint64_t> unique_totals;
    for (const auto& [fen, counts] : unique) {
        unique_expectations += counts.size();
        for (const auto& [depth, count] : counts) {
            ++unique_depth_rows[depth];
            unique_totals[depth] += count;
        }
    }
    REQUIRE(unique_expectations == 749);

    const std::map<unsigned, std::size_t> expected_physical_rows{
        {1,126},{2,126},{3,126},{4,126},{5,126},{6,125}};
    const std::map<unsigned, std::size_t> expected_unique_rows{
        {1,125},{2,125},{3,125},{4,125},{5,125},{6,124}};
    const std::map<unsigned, std::uint64_t> expected_physical_totals{
        {1,1408},{2,21134},{3,486175},{4,12225539},{5,382938880},{6,4387232996ULL}};
    const std::map<unsigned, std::uint64_t> expected_unique_totals{
        {1,1406},{2,21098},{3,486032},{4,12221902},{5,382923987},{6,4386841489ULL}};
    REQUIRE(physical_depth_rows == expected_physical_rows);
    REQUIRE(unique_depth_rows == expected_unique_rows);
    REQUIRE(physical_totals == expected_physical_totals);
    REQUIRE(unique_totals == expected_unique_totals);

    std::vector<std::pair<std::string, std::vector<std::size_t>>> duplicates;
    for (const auto& [line, physical_lines] : physical_lines_by_bytes) {
        if (physical_lines.size() > 1) duplicates.emplace_back(line, physical_lines);
    }
    REQUIRE(duplicates.size() == 1);
    const std::string duplicate_fen = "6KQ/8/8/8/8/8/8/7k b - - 0 1";
    const std::vector<std::size_t> expected_duplicate_lines{60,63};
    REQUIRE(duplicates.front().second == expected_duplicate_lines);
    REQUIRE(duplicates.front().first.starts_with(duplicate_fen + " "));
    const std::map<unsigned, std::uint64_t> expected_duplicate_counts{
        {1,2},{2,36},{3,143},{4,3637},{5,14893},{6,391507}};
    REQUIRE(unique.at(duplicate_fen) == expected_duplicate_counts);

    std::size_t missing_d6 = 0;
    std::string missing_d6_fen;
    for (const auto& [fen, counts] : unique) {
        if (!counts.contains(6)) { ++missing_d6; missing_d6_fen = fen; }
    }
    REQUIRE(missing_d6 == 1);
    REQUIRE(missing_d6_fen == "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
}
