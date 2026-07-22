#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace tdfa_test::blind {

inline std::filesystem::path fixture_path(const std::string_view repository_relative) {
    const std::array<std::filesystem::path, 5> roots{
        std::filesystem::path{"."}, std::filesystem::path{".."},
        std::filesystem::path{"../.."}, std::filesystem::path{"../../.."},
        std::filesystem::path{"../../../.."}};
    for (const auto& root : roots) {
        const auto candidate = root / std::filesystem::path{repository_relative};
        if (std::filesystem::exists(candidate)) return candidate;
    }
    throw std::runtime_error("fixture not found: " + std::string(repository_relative));
}

inline std::string read_fixture_bytes(const std::string_view repository_relative) {
    const auto path = fixture_path(repository_relative);
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open fixture: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

inline std::uint32_t sha_rotr(const std::uint32_t value, const unsigned shift) {
    return (value >> shift) | (value << (32U - shift));
}

inline std::string sha256_hex(const std::string_view bytes) {
    static constexpr std::array<std::uint32_t, 64> k{
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};
    std::vector<std::uint8_t> message(bytes.begin(), bytes.end());
    const std::uint64_t bit_length = static_cast<std::uint64_t>(message.size()) * 8U;
    message.push_back(0x80U);
    while ((message.size() % 64U) != 56U) message.push_back(0U);
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<std::uint8_t>((bit_length >> shift) & 0xffU));
    }
    std::array<std::uint32_t, 8> hash{
        0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
        0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};
    for (std::size_t block = 0; block < message.size(); block += 64U) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            const std::size_t offset = block + i * 4U;
            w[i] = (static_cast<std::uint32_t>(message[offset]) << 24U) |
                   (static_cast<std::uint32_t>(message[offset + 1]) << 16U) |
                   (static_cast<std::uint32_t>(message[offset + 2]) << 8U) |
                   static_cast<std::uint32_t>(message[offset + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = sha_rotr(w[i-15], 7) ^ sha_rotr(w[i-15], 18) ^ (w[i-15] >> 3U);
            const std::uint32_t s1 = sha_rotr(w[i-2], 17) ^ sha_rotr(w[i-2], 19) ^ (w[i-2] >> 10U);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        auto [a,b,c,d,e,f,g,h] = hash;
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = sha_rotr(e,6) ^ sha_rotr(e,11) ^ sha_rotr(e,25);
            const std::uint32_t choose = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + choose + k[i] + w[i];
            const std::uint32_t s0 = sha_rotr(a,2) ^ sha_rotr(a,13) ^ sha_rotr(a,22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        hash[0]+=a; hash[1]+=b; hash[2]+=c; hash[3]+=d;
        hash[4]+=e; hash[5]+=f; hash[6]+=g; hash[7]+=h;
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto word : hash) out << std::setw(8) << word;
    return out.str();
}

inline std::vector<std::string> split_preserving_empty(const std::string_view text, const char delimiter) {
    std::vector<std::string> fields;
    std::size_t begin = 0;
    for (;;) {
        const std::size_t end = text.find(delimiter, begin);
        fields.emplace_back(text.substr(begin, end == std::string_view::npos
            ? text.size() - begin : end - begin));
        if (end == std::string_view::npos) return fields;
        begin = end + 1;
    }
}

struct TsvFixture {
    std::string bytes;
    std::string scoped_bytes;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
};

inline TsvFixture load_tsv_fixture(const std::string_view path) {
    TsvFixture fixture;
    fixture.bytes = read_fixture_bytes(path);
    if (fixture.bytes.empty() || fixture.bytes.back() != '\n') {
        throw std::runtime_error("TSV fixture must end in LF");
    }
    std::size_t cursor = 0;
    std::size_t header_offset = std::string::npos;
    while (cursor < fixture.bytes.size()) {
        const std::size_t newline = fixture.bytes.find('\n', cursor);
        if (newline == std::string::npos) throw std::runtime_error("TSV line missing LF");
        const std::string_view line{fixture.bytes.data() + cursor, newline - cursor};
        if (line.find('\r') != std::string_view::npos) throw std::runtime_error("TSV contains CR");
        if (!line.empty() && line.front() == '#') {
            const std::size_t equals = line.find('=');
            if (equals == std::string_view::npos || equals <= 2) throw std::runtime_error("malformed TSV metadata");
            const std::string key{line.substr(2, equals - 2)};
            if (!fixture.metadata.emplace(key, std::string{line.substr(equals + 1)}).second) {
                throw std::runtime_error("duplicate TSV metadata key");
            }
        } else if (header_offset == std::string::npos) {
            if (line.empty()) throw std::runtime_error("blank line before TSV header");
            header_offset = cursor;
            fixture.columns = split_preserving_empty(line, '\t');
            std::set<std::string> unique(fixture.columns.begin(), fixture.columns.end());
            if (unique.size() != fixture.columns.size()) throw std::runtime_error("duplicate TSV column");
        } else {
            if (line.empty()) throw std::runtime_error("blank TSV data row");
            auto fields = split_preserving_empty(line, '\t');
            if (fields.size() != fixture.columns.size()) throw std::runtime_error("TSV column count mismatch");
            fixture.rows.push_back(std::move(fields));
        }
        cursor = newline + 1;
    }
    if (header_offset == std::string::npos) throw std::runtime_error("missing TSV header");
    fixture.scoped_bytes = fixture.bytes.substr(header_offset);
    const auto declared_digest = fixture.metadata.find("data_sha256");
    if (declared_digest == fixture.metadata.end()) {
        throw std::runtime_error("TSV metadata is missing data_sha256");
    }
    if (sha256_hex(fixture.scoped_bytes) != declared_digest->second) {
        throw std::runtime_error("TSV scoped data does not match data_sha256");
    }
    return fixture;
}

inline std::size_t column_index(const TsvFixture& fixture, const std::string_view name) {
    for (std::size_t i = 0; i < fixture.columns.size(); ++i) {
        if (fixture.columns[i] == name) return i;
    }
    throw std::runtime_error("missing TSV column: " + std::string(name));
}

inline std::vector<std::string> space_tokens_or_empty(const std::string_view field) {
    if (field == "-") return {};
    if (field.empty() || field.front() == ' ' || field.back() == ' ' || field.find("  ") != std::string_view::npos) {
        throw std::runtime_error("noncanonical space-separated fixture field");
    }
    return split_preserving_empty(field, ' ');
}

struct OracleCase {
    std::string case_id;
    std::string source;
    std::string root_fen;
    std::vector<std::string> history;
    std::string fen;
    char turn{};
    bool in_check{};
    bool is_checkmate{};
    bool is_stalemate{};
    std::vector<std::string> legal_moves;
};

inline std::vector<OracleCase> oracle_cases(const TsvFixture& fixture) {
    const auto c_case = column_index(fixture, "case_id");
    const auto c_source = column_index(fixture, "source");
    const auto c_root = column_index(fixture, "root_fen");
    const auto c_history = column_index(fixture, "history_uci");
    const auto c_fen = column_index(fixture, "fen");
    const auto c_turn = column_index(fixture, "turn");
    const auto c_check = column_index(fixture, "in_check");
    const auto c_mate = column_index(fixture, "is_checkmate");
    const auto c_stale = column_index(fixture, "is_stalemate");
    const auto c_legal = column_index(fixture, "legal_uci");
    std::vector<OracleCase> cases;
    for (const auto& row : fixture.rows) {
        if (row[c_turn] != "w" && row[c_turn] != "b") throw std::runtime_error("oracle turn token");
        auto bit = [&](const std::size_t column) {
            if (row[column] == "0") return false;
            if (row[column] == "1") return true;
            throw std::runtime_error("oracle boolean token");
        };
        cases.push_back({row[c_case], row[c_source], row[c_root],
                         space_tokens_or_empty(row[c_history]), row[c_fen], row[c_turn][0],
                         bit(c_check), bit(c_mate), bit(c_stale),
                         space_tokens_or_empty(row[c_legal])});
    }
    return cases;
}

struct PerftRow {
    std::string case_id;
    std::string fen;
    unsigned depth{};
    std::uint64_t nodes{};
};

inline std::vector<PerftRow> oracle_perft_rows(const TsvFixture& fixture) {
    const auto c_case = column_index(fixture, "case_id");
    const auto c_fen = column_index(fixture, "fen");
    const auto c_depth = column_index(fixture, "depth");
    const auto c_nodes = column_index(fixture, "nodes");
    std::vector<PerftRow> rows;
    for (const auto& row : fixture.rows) {
        const auto is_ascii_unsigned = [](const std::string& text) {
            if (text.empty()) return false;
            for (const unsigned char value : text) {
                if (value < static_cast<unsigned char>('0') ||
                    value > static_cast<unsigned char>('9')) return false;
            }
            return true;
        };
        if (!is_ascii_unsigned(row[c_depth]) || !is_ascii_unsigned(row[c_nodes]))
            throw std::runtime_error("perft numerics must be nonempty ASCII unsigned decimals");
        std::size_t consumed = 0;
        const unsigned long depth = std::stoul(row[c_depth], &consumed);
        if (consumed != row[c_depth].size() || depth == 0 || depth > std::numeric_limits<unsigned>::max()) {
            throw std::runtime_error("invalid perft depth");
        }
        consumed = 0;
        const unsigned long long nodes = std::stoull(row[c_nodes], &consumed);
        if (consumed != row[c_nodes].size() ||
            nodes > std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("invalid perft node count");
        rows.push_back({row[c_case], row[c_fen], static_cast<unsigned>(depth),
                        static_cast<std::uint64_t>(nodes)});
    }
    return rows;
}

} // namespace tdfa_test::blind
