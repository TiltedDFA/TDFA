#pragma once

#include "BlindFixtureData.hpp"
#include "MagicConstants.hpp"
#include "Position.hpp"
#include "Util.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tdfa_test
{

inline constexpr std::string_view kCasesDataSha256 =
    "7c6bfab18aecbf89e6da7cdcc29b258edb13bf916804c9f211cb9752c7cb4e77";
inline constexpr std::string_view kPerftDataSha256 =
    "8f7643f4c8168c93b0590634e17ea11db52ddfe167e7c04d04710cb986f40789";

inline std::vector<std::string> split_preserving_empty(std::string_view text, char delimiter)
{
    std::vector<std::string> result;
    std::size_t begin = 0;
    while (true)
    {
        const auto end = text.find(delimiter, begin);
        result.emplace_back(text.substr(begin, end == std::string_view::npos ? end : end - begin));
        if (end == std::string_view::npos)
            break;
        begin = end + 1;
    }
    return result;
}

struct TsvTable
{
    std::string scoped_bytes;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> columns;
    std::map<std::string, std::size_t> column_index;
    std::vector<std::vector<std::string>> rows;

    [[nodiscard]] const std::string& at(const std::vector<std::string>& row,
                                        std::string_view column) const
    {
        const auto found = column_index.find(std::string(column));
        if (found == column_index.end() || found->second >= row.size())
            throw std::runtime_error("missing TSV column: " + std::string(column));
        return row[found->second];
    }
};

inline TsvTable load_tsv(const std::string& path)
{
    const std::string bytes = blind::canonical_lf(blind::read_fixture_bytes(path));
    if (bytes.empty() || bytes.back() != '\n')
        throw std::runtime_error("TSV fixture must end in LF: " + path);

    std::size_t header_offset = 0;
    while (header_offset < bytes.size() && bytes[header_offset] == '#')
    {
        const auto newline = bytes.find('\n', header_offset);
        if (newline == std::string::npos)
            throw std::runtime_error("unterminated TSV metadata: " + path);
        header_offset = newline + 1;
    }
    if (header_offset == bytes.size())
        throw std::runtime_error("fixture has no TSV header: " + path);

    std::istringstream input(bytes);

    TsvTable table;
    table.scoped_bytes = bytes.substr(header_offset);
    std::string line;
    while (std::getline(input, line))
    {
        if (line.starts_with('#'))
        {
            const auto equal = line.find('=');
            if (equal != std::string::npos && equal > 2)
                table.metadata.emplace(line.substr(2, equal - 2), line.substr(equal + 1));
            continue;
        }
        if (table.columns.empty())
        {
            table.columns = split_preserving_empty(line, '\t');
            for (std::size_t i = 0; i < table.columns.size(); ++i)
                table.column_index.emplace(table.columns[i], i);
            continue;
        }
        auto row = split_preserving_empty(line, '\t');
        if (row.size() != table.columns.size())
            throw std::runtime_error("wrong TSV field count in " + path);
        table.rows.push_back(std::move(row));
    }
    if (table.columns.empty())
        throw std::runtime_error("fixture has no TSV header: " + path);
    const auto digest = table.metadata.find("data_sha256");
    if (digest == table.metadata.end())
        throw std::runtime_error("fixture has no data_sha256 metadata: " + path);
    if (blind::sha256_hex(table.scoped_bytes) != digest->second)
        throw std::runtime_error("fixture scoped bytes do not match data_sha256: " + path);
    return table;
}

struct OracleCase
{
    std::string case_id;
    std::string root_fen;
    std::string fen;
    std::vector<std::string> legal_uci;
};

inline std::vector<std::string> split_space_list(const std::string& value)
{
    if (value == "-")
        return {};
    return split_preserving_empty(value, ' ');
}

inline std::pair<TsvTable, std::vector<OracleCase>> load_oracle_cases()
{
    auto table = load_tsv("tests/data/oracle_cases.tsv");
    std::vector<OracleCase> cases;
    cases.reserve(table.rows.size());
    for (const auto& row : table.rows)
    {
        cases.push_back(OracleCase{
            table.at(row, "case_id"),
            table.at(row, "root_fen"),
            table.at(row, "fen"),
            split_space_list(table.at(row, "legal_uci")),
        });
    }
    return {std::move(table), std::move(cases)};
}

inline std::pair<TsvTable, std::vector<std::string>> load_oracle_perft_fens()
{
    auto table = load_tsv("tests/data/oracle_perft.tsv");
    std::vector<std::string> fens;
    fens.reserve(table.rows.size());
    for (const auto& row : table.rows)
        fens.push_back(table.at(row, "fen"));
    return {std::move(table), std::move(fens)};
}

inline std::array<std::string, 6> split_fen_independently(std::string_view fen)
{
    std::array<std::string, 6> fields;
    std::size_t begin = 0;
    for (std::size_t i = 0; i < 5; ++i)
    {
        const auto end = fen.find(' ', begin);
        if (end == std::string_view::npos)
            throw std::runtime_error("FEN has fewer than six fields");
        fields[i] = std::string(fen.substr(begin, end - begin));
        begin = end + 1;
    }
    if (fen.find(' ', begin) != std::string_view::npos)
        throw std::runtime_error("FEN has more than six fields");
    fields[5] = std::string(fen.substr(begin));
    return fields;
}

inline unsigned parse_decimal_independently(std::string_view text)
{
    if (text.empty())
        throw std::runtime_error("empty decimal FEN field");
    unsigned value = 0;
    for (const char ch : text)
    {
        if (ch < '0' || ch > '9')
            throw std::runtime_error("non-decimal FEN counter");
        value = value * 10u + static_cast<unsigned>(ch - '0');
    }
    return value;
}

inline Piece piece_from_fen_letter(char letter)
{
    switch (letter)
    {
    case 'K': return p_WhiteKing;
    case 'Q': return p_WhiteQueen;
    case 'B': return p_WhiteBishop;
    case 'N': return p_WhiteKnight;
    case 'R': return p_WhiteRook;
    case 'P': return p_WhitePawn;
    case 'k': return p_BlackKing;
    case 'q': return p_BlackQueen;
    case 'b': return p_BlackBishop;
    case 'n': return p_BlackKnight;
    case 'r': return p_BlackRook;
    case 'p': return p_BlackPawn;
    default: throw std::runtime_error("invalid FEN piece letter");
    }
}

inline int model_colour_index(Piece piece)
{
    switch (piece)
    {
    case p_WhiteKing:
    case p_WhiteQueen:
    case p_WhiteBishop:
    case p_WhiteKnight:
    case p_WhiteRook:
    case p_WhitePawn: return 0;
    case p_BlackKing:
    case p_BlackQueen:
    case p_BlackBishop:
    case p_BlackKnight:
    case p_BlackRook:
    case p_BlackPawn: return 1;
    default: return -1;
    }
}

inline int model_type_index(Piece piece)
{
    switch (piece)
    {
    case p_WhiteKing:
    case p_BlackKing: return 0;
    case p_WhiteQueen:
    case p_BlackQueen: return 1;
    case p_WhiteBishop:
    case p_BlackBishop: return 2;
    case p_WhiteKnight:
    case p_BlackKnight: return 3;
    case p_WhiteRook:
    case p_BlackRook: return 4;
    case p_WhitePawn:
    case p_BlackPawn: return 5;
    default: return -1;
    }
}

inline std::string model_square_label(unsigned index)
{
    if (index >= 64)
        throw std::runtime_error("model square index out of range");
    std::string label(2, ' ');
    label[0] = static_cast<char>('a' + index % 8u);
    label[1] = static_cast<char>('1' + index / 8u);
    return label;
}

struct ExpectedPosition
{
    std::array<Piece, 64> pieces{};
    std::array<std::set<std::string>, 2> colour_squares;
    std::array<std::set<std::string>, 6> type_squares;
    std::set<std::string> occupied_squares;
    Colour turn = White;
    std::set<char> castling_rights;
    std::optional<std::string> en_passant;
    unsigned halfmove = 0;
    unsigned fullmove = 1;
};

inline ExpectedPosition decode_fen_independently(std::string_view fen)
{
    const auto fields = split_fen_independently(fen);
    ExpectedPosition result;
    result.pieces.fill(p_None);

    const auto ranks = split_preserving_empty(fields[0], '/');
    if (ranks.size() != 8)
        throw std::runtime_error("FEN placement does not have eight ranks");
    for (unsigned fen_rank = 0; fen_rank < 8; ++fen_rank)
    {
        unsigned file = 0;
        for (const char token : ranks[fen_rank])
        {
            if (token >= '1' && token <= '8')
            {
                file += static_cast<unsigned>(token - '0');
                continue;
            }
            if (file >= 8)
                throw std::runtime_error("FEN rank overflow");
            const unsigned rank_from_one = 8u - fen_rank;
            const unsigned index = file + 8u * (rank_from_one - 1u);
            result.pieces[index] = piece_from_fen_letter(token);
            ++file;
        }
        if (file != 8)
            throw std::runtime_error("FEN rank does not expand to eight files");
    }

    if (fields[1] == "w") result.turn = White;
    else if (fields[1] == "b") result.turn = Black;
    else throw std::runtime_error("invalid FEN active colour");

    if (fields[2] != "-")
    {
        for (const char right : fields[2])
        {
            switch (right)
            {
            case 'K':
            case 'Q':
            case 'k':
            case 'q':
                if (!result.castling_rights.insert(right).second)
                    throw std::runtime_error("duplicate FEN castling right");
                break;
            default: throw std::runtime_error("invalid FEN castling right");
            }
        }
    }

    if (fields[3] != "-")
        result.en_passant = fields[3];
    result.halfmove = parse_decimal_independently(fields[4]);
    result.fullmove = parse_decimal_independently(fields[5]);

    for (unsigned index = 0; index < 64; ++index)
    {
        const Piece piece = result.pieces[index];
        if (piece == p_None)
            continue;
        const auto label = model_square_label(index);
        result.colour_squares[static_cast<std::size_t>(model_colour_index(piece))].insert(label);
        result.type_squares[static_cast<std::size_t>(model_type_index(piece))].insert(label);
        result.occupied_squares.insert(label);
    }
    return result;
}

inline std::set<std::string> observed_labels(BitBoard board)
{
    std::set<std::string> result;
    for (unsigned index = 0; index < 64; ++index)
    {
        const BitBoard bit = U64{1} << index;
        if ((board & bit) != 0)
            result.insert(UTIL::Square(static_cast<Sq>(index)));
    }
    return result;
}

struct ObservedPosition
{
    std::map<std::string, Piece> pieces;
    std::array<std::set<std::string>, 2> colour_squares;
    std::array<std::set<std::string>, 6> type_squares;
    std::set<std::string> occupied_squares;
    bool square_labels_unique = true;
    Colour turn = White;
    U8 castling = 0;
    std::set<char> castling_rights;
    bool castling_has_unknown_bits = false;
    BitBoard en_passant_board = 0;
    std::set<std::string> en_passant_labels;
    std::optional<std::string> en_passant_square;
    U8 halfmove = 0;
    U16 fullmove = 0;

    bool operator==(const ObservedPosition&) const = default;
};

inline ObservedPosition observe_position(const Position& position)
{
    ObservedPosition result;
    for (unsigned index = 0; index < 64; ++index)
    {
        const auto label = UTIL::Square(static_cast<Sq>(index));
        const bool inserted =
            result.pieces.emplace(label, position.PieceOn(static_cast<Sq>(index))).second;
        result.square_labels_unique = result.square_labels_unique && inserted;
    }
    result.colour_squares[0] = observed_labels(position.Pieces(White));
    result.colour_squares[1] = observed_labels(position.Pieces(Black));
    constexpr std::array<PieceType, 6> types{
        pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
    };
    for (std::size_t i = 0; i < types.size(); ++i)
        result.type_squares[i] = observed_labels(position.Pieces(types[i]));
    result.occupied_squares = observed_labels(position.Pieces(pt_All));
    result.turn = position.ColourToMove();
    result.castling = position.CastlingRights();
    constexpr std::array<std::pair<char, U8>, 4> named_rights{
        std::pair{'K', Magics::CASTLE_K_W},
        std::pair{'Q', Magics::CASTLE_Q_W},
        std::pair{'k', Magics::CASTLE_K_B},
        std::pair{'q', Magics::CASTLE_Q_B},
    };
    U8 known_castling_bits = 0;
    for (const auto [name, bits] : named_rights)
    {
        known_castling_bits = static_cast<U8>(known_castling_bits | bits);
        if (bits != 0 && (result.castling & bits) == bits)
            result.castling_rights.insert(name);
    }
    result.castling_has_unknown_bits =
        (result.castling & static_cast<U8>(~known_castling_bits)) != 0;
    result.en_passant_board = position.EnPasBB();
    result.en_passant_labels = observed_labels(result.en_passant_board);
    if (result.en_passant_board != 0)
        result.en_passant_square = UTIL::Square(position.EnPasSq());
    result.halfmove = position.HalfMoves();
    result.fullmove = position.FullMoves();
    return result;
}

inline bool position_matches(const ExpectedPosition& expected,
                             const ObservedPosition& actual,
                             std::string& reason)
{
    if (!actual.square_labels_unique || actual.pieces.size() != 64)
    {
        reason = "UTIL::Square labels are not a 64-element bijection";
        return false;
    }
    for (unsigned index = 0; index < 64; ++index)
    {
        const auto label = model_square_label(index);
        const auto found = actual.pieces.find(label);
        if (found == actual.pieces.end() || found->second != expected.pieces[index])
        {
            reason = "piece mismatch at " + label;
            return false;
        }
    }
    for (std::size_t colour = 0; colour < 2; ++colour)
    {
        if (actual.colour_squares[colour] != expected.colour_squares[colour])
        {
            reason = "colour occupancy mismatch at colour index " + std::to_string(colour);
            return false;
        }
    }
    for (std::size_t type = 0; type < 6; ++type)
    {
        if (actual.type_squares[type] != expected.type_squares[type])
        {
            reason = "piece-type occupancy mismatch at type index " + std::to_string(type);
            return false;
        }
    }
    if (actual.occupied_squares != expected.occupied_squares)
    {
        reason = "pt_All occupancy mismatch";
        return false;
    }
    if (actual.turn != expected.turn)
    {
        reason = "active colour mismatch";
        return false;
    }
    if (actual.castling_has_unknown_bits ||
        actual.castling_rights != expected.castling_rights)
    {
        reason = "castling-right set mismatch";
        return false;
    }
    if (expected.en_passant)
    {
        if (actual.en_passant_labels != std::set<std::string>{*expected.en_passant} ||
            actual.en_passant_square != expected.en_passant)
        {
            reason = "present en-passant target mismatch";
            return false;
        }
    }
    else if (actual.en_passant_board != 0)
    {
        reason = "absent en-passant target has a nonzero bitboard";
        return false;
    }
    if (actual.halfmove != expected.halfmove)
    {
        reason = "halfmove counter mismatch";
        return false;
    }
    if (actual.fullmove != expected.fullmove)
    {
        reason = "fullmove counter mismatch";
        return false;
    }
    return true;
}

inline bool placement_and_side_match(const ExpectedPosition& expected,
                                     const ObservedPosition& actual,
                                     std::string& reason)
{
    if (!actual.square_labels_unique || actual.pieces.size() != 64)
    {
        reason = "UTIL::Square labels are not a 64-element bijection";
        return false;
    }
    for (unsigned index = 0; index < 64; ++index)
    {
        const auto label = model_square_label(index);
        const auto found = actual.pieces.find(label);
        if (found == actual.pieces.end() || found->second != expected.pieces[index])
        {
            reason = "piece mismatch at " + label;
            return false;
        }
    }
    if (actual.colour_squares != expected.colour_squares)
    {
        reason = "colour occupancy mismatch";
        return false;
    }
    if (actual.type_squares != expected.type_squares)
    {
        reason = "piece-type occupancy mismatch";
        return false;
    }
    if (actual.occupied_squares != expected.occupied_squares)
    {
        reason = "pt_All occupancy mismatch";
        return false;
    }
    if (actual.turn != expected.turn)
    {
        reason = "active colour mismatch";
        return false;
    }
    return true;
}

inline std::vector<std::string> corpus_position_fens()
{
    const auto [case_table, cases] = load_oracle_cases();
    const auto [perft_table, perft_fens] = load_oracle_perft_fens();
    (void)case_table;
    (void)perft_table;
    std::set<std::string> unique;
    for (const auto& entry : cases)
        unique.insert(entry.fen);
    unique.insert(perft_fens.begin(), perft_fens.end());
    return {unique.begin(), unique.end()};
}

inline std::vector<std::string> fen_001_fixtures()
{
    std::set<std::string> unique;
    const auto corpus = corpus_position_fens();
    unique.insert(corpus.begin(), corpus.end());
    constexpr std::array<std::string_view, 6> boundary_fens{
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
        "4k3/8/8/8/8/8/8/4K3 b - - 0 1",
        "r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1",
        "7k/8/8/p7/8/8/8/K7 w - a6 0 2",
        "7k/8/8/8/7P/8/8/K7 b - h3 0 1",
        "7k/8/8/8/8/8/R7/K7 w - - 255 65535",
    };
    for (const auto fen : boundary_fens)
        unique.emplace(fen);
    return {unique.begin(), unique.end()};
}

using BoardModel = std::array<Piece, 64>;

inline BoardModel empty_board_model()
{
    BoardModel result;
    result.fill(p_None);
    return result;
}

inline BitBoard model_board_mask(const BoardModel& model, int colour, int type)
{
    BitBoard result = 0;
    for (unsigned index = 0; index < 64; ++index)
    {
        const Piece piece = model[index];
        if (piece == p_None)
            continue;
        if (colour >= 0 && model_colour_index(piece) != colour)
            continue;
        if (type >= 0 && model_type_index(piece) != type)
            continue;
        result |= U64{1} << index;
    }
    return result;
}

inline bool board_matches(const Board& board, const BoardModel& expected, std::string& reason)
{
    for (unsigned index = 0; index < 64; ++index)
    {
        if (board.PieceOn(static_cast<Sq>(index)) != expected[index])
        {
            reason = "piece mismatch at raw square " + std::to_string(index);
            return false;
        }
    }
    constexpr std::array<PieceType, 6> types{
        pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
    };
    if (board.Pieces(White) != model_board_mask(expected, 0, -1))
    {
        reason = "white occupancy mismatch";
        return false;
    }
    if (board.Pieces(Black) != model_board_mask(expected, 1, -1))
    {
        reason = "black occupancy mismatch";
        return false;
    }
    if (board.Pieces(pt_All) != model_board_mask(expected, -1, -1))
    {
        reason = "pt_All occupancy mismatch";
        return false;
    }
    for (std::size_t type = 0; type < types.size(); ++type)
    {
        if (board.Pieces(types[type]) != model_board_mask(expected, -1, static_cast<int>(type)))
        {
            reason = "type occupancy mismatch at type index " + std::to_string(type);
            return false;
        }
    }
    return true;
}

inline unsigned model_popcount(BitBoard board)
{
    unsigned count = 0;
    for (unsigned index = 0; index < 64; ++index)
    {
        const BitBoard coefficient = U64{1} << index;
        count += static_cast<unsigned>((board / coefficient) % U64{2});
    }
    return count;
}

inline std::vector<unsigned> model_set_indices(BitBoard board)
{
    std::vector<unsigned> result;
    for (unsigned index = 0; index < 64; ++index)
    {
        const BitBoard coefficient = U64{1} << index;
        if ((board / coefficient) % U64{2} != 0)
            result.push_back(index);
    }
    return result;
}

inline BitBoard model_board_from_indices(const std::vector<unsigned>& indices,
                                         std::size_t begin,
                                         std::size_t end)
{
    BitBoard result = 0;
    for (std::size_t i = begin; i < end; ++i)
        result += U64{1} << indices[i];
    return result;
}

class SplitMix64
{
public:
    explicit SplitMix64(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next()
    {
        state_ += UINT64_C(0x9e3779b97f4a7c15);
        std::uint64_t value = state_;
        value = (value ^ (value >> 30u)) * UINT64_C(0xbf58476d1ce4e5b9);
        value = (value ^ (value >> 27u)) * UINT64_C(0x94d049bb133111eb);
        return value ^ (value >> 31u);
    }

private:
    std::uint64_t state_;
};

inline std::string hex_u64(std::uint64_t value)
{
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

} // namespace tdfa_test
