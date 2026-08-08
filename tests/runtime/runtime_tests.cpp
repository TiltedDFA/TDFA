#include <catch2/catch_test_macros.hpp>

#include "../support/BlindFixtureData.hpp"
#include "../support/PrimitivesPositionTestModels.hpp"

#include "Evaluate.hpp"
#include "Testing.hpp"
#include "TranspositionTable.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(__linux__) && defined(__clang__)
#if __has_feature(address_sanitizer)
#define TDFA_RUNTIME_LINUX_CLANG_ASAN 1
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#endif

namespace
{

using LabelSet = std::set<std::string>;

constexpr std::array<PieceType, 6> kOrdinaryTypes{
    pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
};

constexpr std::array<std::string_view, 6> kTypeNames{
    "pt_King", "pt_Queen", "pt_Bishop", "pt_Knight", "pt_Rook", "pt_Pawn",
};

constexpr std::array<Colour, 2> kColours{White, Black};
constexpr std::array<std::string_view, 2> kColourNames{"White", "Black"};

std::string piece_name(const Piece piece)
{
  switch (piece)
  {
  case p_WhiteKing:
    return "p_WhiteKing";
  case p_WhiteQueen:
    return "p_WhiteQueen";
  case p_WhiteBishop:
    return "p_WhiteBishop";
  case p_WhiteKnight:
    return "p_WhiteKnight";
  case p_WhiteRook:
    return "p_WhiteRook";
  case p_WhitePawn:
    return "p_WhitePawn";
  case p_BlackKing:
    return "p_BlackKing";
  case p_BlackQueen:
    return "p_BlackQueen";
  case p_BlackBishop:
    return "p_BlackBishop";
  case p_BlackKnight:
    return "p_BlackKnight";
  case p_BlackRook:
    return "p_BlackRook";
  case p_BlackPawn:
    return "p_BlackPawn";
  case p_None:
    return "p_None";
  }
  return "invalid-Piece";
}

std::string colour_name(const Colour colour)
{
  if (colour == White)
    return "White";
  if (colour == Black)
    return "Black";
  return "invalid-Colour";
}

std::string print_labels(const LabelSet &labels)
{
  std::ostringstream out;
  out << '{';
  bool first = true;
  for (const auto &label : labels)
  {
    if (!first)
      out << ',';
    first = false;
    out << label;
  }
  out << '}';
  return out.str();
}

std::string print_rights(const std::set<char> &rights)
{
  if (rights.empty())
    return "-";
  return {rights.begin(), rights.end()};
}

std::string print_differences(const std::vector<std::string> &differences)
{
  if (differences.empty())
    return "none";
  std::ostringstream out;
  for (std::size_t index = 0; index < differences.size(); ++index)
  {
    if (index != 0)
      out << " | ";
    out << differences[index];
  }
  return out.str();
}

LabelSet intersection(const LabelSet &lhs, const LabelSet &rhs)
{
  LabelSet result;
  std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                        std::inserter(result, result.end()));
  return result;
}

struct FullFenSemanticSnapshot
{
  bool square_labels_are_a_bijection = true;
  std::map<std::string, Piece> pieces;
  std::array<LabelSet, 2> by_colour;
  std::array<LabelSet, 6> by_type;
  std::array<std::array<LabelSet, 6>, 2> by_colour_and_type;
  LabelSet occupied;
  Colour active_colour = White;
  std::set<char> castling_rights;
  LabelSet en_passant_labels;
  std::optional<std::string> en_passant_square;
  U8 halfmove_clock = 0;
  U16 fullmove_number = 0;
};

FullFenSemanticSnapshot capture_full_fen_semantics(const Position &position)
{
  FullFenSemanticSnapshot result;
  for (unsigned raw_square = 0; raw_square < 64; ++raw_square)
  {
    const auto square = static_cast<Sq>(raw_square);
    const std::string label = UTIL::Square(square);
    const bool canonical = label.size() == 2 && label[0] >= 'a' &&
                           label[0] <= 'h' && label[1] >= '1' &&
                           label[1] <= '8';
    const bool inserted =
        result.pieces.emplace(label, position.PieceOn(square)).second;
    result.square_labels_are_a_bijection =
        result.square_labels_are_a_bijection && canonical && inserted;
  }

  for (std::size_t colour = 0; colour < kColours.size(); ++colour)
  {
    result.by_colour[colour] =
        tdfa_test::observed_labels(position.Pieces(kColours[colour]));
    for (std::size_t type = 0; type < kOrdinaryTypes.size(); ++type)
    {
      result.by_colour_and_type[colour][type] = tdfa_test::observed_labels(
          position.Pieces(kColours[colour], kOrdinaryTypes[type]));
    }
  }
  for (std::size_t type = 0; type < kOrdinaryTypes.size(); ++type)
    result.by_type[type] =
        tdfa_test::observed_labels(position.Pieces(kOrdinaryTypes[type]));
  result.occupied = tdfa_test::observed_labels(position.Pieces(pt_All));

  result.active_colour = position.ColourToMove();
  const U8 encoded_rights = position.CastlingRights();
  constexpr std::array<std::pair<char, U8>, 4> named_rights{
      std::pair{'K', Magics::CASTLE_K_W},
      std::pair{'Q', Magics::CASTLE_Q_W},
      std::pair{'k', Magics::CASTLE_K_B},
      std::pair{'q', Magics::CASTLE_Q_B},
  };
  for (const auto [name, bits] : named_rights)
  {
    if (bits != 0 && (encoded_rights & bits) == bits)
      result.castling_rights.insert(name);
  }

  const BitBoard en_passant = position.EnPasBB();
  result.en_passant_labels = tdfa_test::observed_labels(en_passant);
  if (en_passant != 0)
    result.en_passant_square = UTIL::Square(position.EnPasSq());
  result.halfmove_clock = position.HalfMoves();
  result.fullmove_number = position.FullMoves();
  return result;
}

void append_set_difference(std::vector<std::string> &differences,
                           const std::string &field, const LabelSet &expected,
                           const LabelSet &actual)
{
  if (expected != actual)
  {
    differences.push_back(field + " expected=" + print_labels(expected) +
                          " actual=" + print_labels(actual));
  }
}

std::vector<std::string>
fixture_differences(const tdfa_test::ExpectedPosition &expected,
                    const FullFenSemanticSnapshot &actual)
{
  std::vector<std::string> differences;
  if (!actual.square_labels_are_a_bijection || actual.pieces.size() != 64)
    differences.emplace_back(
        "UTIL::Square is not a canonical 64-label bijection");

  for (unsigned oracle_square = 0; oracle_square < 64; ++oracle_square)
  {
    const std::string label = tdfa_test::model_square_label(oracle_square);
    const auto found = actual.pieces.find(label);
    if (found == actual.pieces.end())
    {
      differences.push_back("PieceOn[" + label + "] is unobservable");
    }
    else if (found->second != expected.pieces[oracle_square])
    {
      differences.push_back("PieceOn[" + label + "] expected=" +
                            piece_name(expected.pieces[oracle_square]) +
                            " actual=" + piece_name(found->second));
    }
  }

  for (std::size_t colour = 0; colour < kColours.size(); ++colour)
  {
    append_set_difference(
        differences, "Pieces(" + std::string(kColourNames[colour]) + ")",
        expected.colour_squares[colour], actual.by_colour[colour]);
    for (std::size_t type = 0; type < kOrdinaryTypes.size(); ++type)
    {
      append_set_difference(differences,
                            "Pieces(" + std::string(kColourNames[colour]) +
                                ',' + std::string(kTypeNames[type]) + ')',
                            intersection(expected.colour_squares[colour],
                                         expected.type_squares[type]),
                            actual.by_colour_and_type[colour][type]);
    }
  }
  for (std::size_t type = 0; type < kOrdinaryTypes.size(); ++type)
  {
    append_set_difference(differences,
                          "Pieces(" + std::string(kTypeNames[type]) + ")",
                          expected.type_squares[type], actual.by_type[type]);
  }
  append_set_difference(differences, "Pieces(pt_All)",
                        expected.occupied_squares, actual.occupied);

  if (actual.active_colour != expected.turn)
  {
    differences.push_back(
        "ColourToMove expected=" + colour_name(expected.turn) +
        " actual=" + colour_name(actual.active_colour));
  }
  if (actual.castling_rights != expected.castling_rights)
  {
    differences.push_back("named castling rights expected=" +
                          print_rights(expected.castling_rights) +
                          " actual=" + print_rights(actual.castling_rights));
  }
  const LabelSet expected_en_passant =
      expected.en_passant ? LabelSet{*expected.en_passant} : LabelSet{};
  append_set_difference(differences, "EnPasBB semantic labels",
                        expected_en_passant, actual.en_passant_labels);
  if (actual.en_passant_square != expected.en_passant)
  {
    differences.push_back("EnPasSq semantic target expected=" +
                          expected.en_passant.value_or("-") +
                          " actual=" + actual.en_passant_square.value_or("-"));
  }
  if (static_cast<unsigned>(actual.halfmove_clock) != expected.halfmove)
  {
    differences.push_back(
        "HalfMoves expected=" + std::to_string(expected.halfmove) + " actual=" +
        std::to_string(static_cast<unsigned>(actual.halfmove_clock)));
  }
  if (static_cast<unsigned>(actual.fullmove_number) != expected.fullmove)
  {
    differences.push_back(
        "FullMoves expected=" + std::to_string(expected.fullmove) + " actual=" +
        std::to_string(static_cast<unsigned>(actual.fullmove_number)));
  }
  return differences;
}

std::vector<std::string>
snapshot_differences(const FullFenSemanticSnapshot &expected,
                     const FullFenSemanticSnapshot &actual)
{
  std::vector<std::string> differences;
  if (expected.square_labels_are_a_bijection !=
      actual.square_labels_are_a_bijection)
    differences.emplace_back("square-label bijection changed");

  std::set<std::string> labels;
  for (const auto &[label, piece] : expected.pieces)
  {
    (void)piece;
    labels.insert(label);
  }
  for (const auto &[label, piece] : actual.pieces)
  {
    (void)piece;
    labels.insert(label);
  }
  for (const auto &label : labels)
  {
    const auto before = expected.pieces.find(label);
    const auto after = actual.pieces.find(label);
    if (before == expected.pieces.end() || after == actual.pieces.end())
    {
      differences.push_back("PieceOn label domain changed at " + label);
    }
    else if (before->second != after->second)
    {
      differences.push_back("PieceOn[" + label +
                            "] before=" + piece_name(before->second) +
                            " after=" + piece_name(after->second));
    }
  }

  for (std::size_t colour = 0; colour < kColours.size(); ++colour)
  {
    append_set_difference(differences,
                          "Pieces(" + std::string(kColourNames[colour]) + ")",
                          expected.by_colour[colour], actual.by_colour[colour]);
    for (std::size_t type = 0; type < kOrdinaryTypes.size(); ++type)
    {
      append_set_difference(differences,
                            "Pieces(" + std::string(kColourNames[colour]) +
                                ',' + std::string(kTypeNames[type]) + ')',
                            expected.by_colour_and_type[colour][type],
                            actual.by_colour_and_type[colour][type]);
    }
  }
  for (std::size_t type = 0; type < kOrdinaryTypes.size(); ++type)
  {
    append_set_difference(differences,
                          "Pieces(" + std::string(kTypeNames[type]) + ")",
                          expected.by_type[type], actual.by_type[type]);
  }
  append_set_difference(differences, "Pieces(pt_All)", expected.occupied,
                        actual.occupied);

  if (expected.active_colour != actual.active_colour)
  {
    differences.push_back(
        "ColourToMove before=" + colour_name(expected.active_colour) +
        " after=" + colour_name(actual.active_colour));
  }
  if (expected.castling_rights != actual.castling_rights)
  {
    differences.push_back("named castling rights before=" +
                          print_rights(expected.castling_rights) +
                          " after=" + print_rights(actual.castling_rights));
  }
  append_set_difference(differences, "EnPasBB semantic labels",
                        expected.en_passant_labels, actual.en_passant_labels);
  if (expected.en_passant_square != actual.en_passant_square)
  {
    differences.push_back("EnPasSq semantic target before=" +
                          expected.en_passant_square.value_or("-") +
                          " after=" + actual.en_passant_square.value_or("-"));
  }
  if (expected.halfmove_clock != actual.halfmove_clock)
  {
    differences.push_back(
        "HalfMoves before=" +
        std::to_string(static_cast<unsigned>(expected.halfmove_clock)) +
        " after=" +
        std::to_string(static_cast<unsigned>(actual.halfmove_clock)));
  }
  if (expected.fullmove_number != actual.fullmove_number)
  {
    differences.push_back(
        "FullMoves before=" +
        std::to_string(static_cast<unsigned>(expected.fullmove_number)) +
        " after=" +
        std::to_string(static_cast<unsigned>(actual.fullmove_number)));
  }
  return differences;
}

FullFenSemanticSnapshot
require_semantically_valid_fixture(Position &position,
                                   const std::string_view fixture_id,
                                   const std::string_view fen)
{
  INFO("fixture-id=" << fixture_id);
  INFO("fen=" << fen);
  const auto expected = tdfa_test::decode_fen_independently(fen);
  const auto before_is_ok = capture_full_fen_semantics(position);
  const auto setup_differences = fixture_differences(expected, before_is_ok);
  INFO("semantic-fixture-validation=" << print_differences(setup_differences));
  REQUIRE(setup_differences.empty());

  const bool is_ok = position.IsOk();
  INFO("IsOk=" << std::boolalpha << is_ok);
  REQUIRE(is_ok);

  const auto after_is_ok = capture_full_fen_semantics(position);
  const auto observer_differences =
      snapshot_differences(before_is_ok, after_is_ok);
  INFO("IsOk-public-semantic-diff=" << print_differences(observer_differences));
  REQUIRE(observer_differences.empty());
  return after_is_ok;
}

void require_snapshot_unchanged(const FullFenSemanticSnapshot &before,
                                const Position &position,
                                const std::string_view operation)
{
  const auto after = capture_full_fen_semantics(position);
  const auto differences = snapshot_differences(before, after);
  INFO("operation=" << operation);
  INFO("full-public-FEN-semantic-diff=" << print_differences(differences));
  REQUIRE(differences.empty());
}

struct NamedFen
{
  std::string_view id;
  std::string_view fen;
};

constexpr std::array<NamedFen, 9> kWhiteMaterialFixtures{
    NamedFen{"base", "7k/8/8/8/8/8/8/K7 w - - 0 1"},
    NamedFen{"pawn", "7k/8/8/8/8/8/1P6/K7 w - - 0 1"},
    NamedFen{"knight", "7k/8/8/8/8/2N5/8/K7 w - - 0 1"},
    NamedFen{"bishop", "7k/8/8/8/2B5/8/8/K7 w - - 0 1"},
    NamedFen{"rook", "7k/8/8/8/4R3/8/8/K7 w - - 0 1"},
    NamedFen{"queen", "7k/8/8/8/8/8/6Q1/K7 w - - 0 1"},
    NamedFen{"combined", "7k/8/8/8/2B1R3/2N5/1P4Q1/K7 w - - 0 1"},
    NamedFen{"isolation_base", "7k/8/8/8/8/8/Q7/K7 w - - 0 1"},
    NamedFen{"isolation_opponent_added", "7k/2qrbn1p/8/8/8/8/Q7/K7 w - - 0 1"},
};

constexpr std::array<NamedFen, 9> kBlackMaterialFixtures{
    NamedFen{"base", "k7/8/8/8/8/8/8/7K b - - 0 1"},
    NamedFen{"pawn", "k7/1p6/8/8/8/8/8/7K b - - 0 1"},
    NamedFen{"knight", "k7/8/2n5/8/8/8/8/7K b - - 0 1"},
    NamedFen{"bishop", "k7/8/8/2b5/8/8/8/7K b - - 0 1"},
    NamedFen{"rook", "k7/8/8/4r3/8/8/8/7K b - - 0 1"},
    NamedFen{"queen", "k7/6q1/8/8/8/8/8/7K b - - 0 1"},
    NamedFen{"combined", "k7/1p4q1/2n5/2b1r3/8/8/8/7K b - - 0 1"},
    NamedFen{"isolation_base", "k7/q7/8/8/8/8/8/7K b - - 0 1"},
    NamedFen{"isolation_opponent_added", "k7/q7/8/8/8/8/2QRBN1P/7K b - - 0 1"},
};

std::string material_fixture_serialization()
{
  std::string bytes;
  const auto append = [&bytes](const std::string_view colour,
                               const std::array<NamedFen, 9> &fixtures)
  {
    for (const auto &fixture : fixtures)
    {
      bytes.append(colour);
      bytes.push_back('\t');
      bytes.append(fixture.id);
      bytes.push_back('\t');
      bytes.append(fixture.fen);
      bytes.push_back('\n');
    }
  };
  append("White", kWhiteMaterialFixtures);
  append("Black", kBlackMaterialFixtures);
  return bytes;
}

using WideScore = std::intmax_t;
static_assert(std::numeric_limits<WideScore>::is_signed);
static_assert(std::numeric_limits<WideScore>::digits >=
              std::numeric_limits<Score>::digits + 3);

struct MaterialObservation
{
  Score raw;
  WideScore widened;
};

template <Colour Selected>
MaterialObservation observe_material(const std::string_view colour,
                                     const NamedFen &fixture)
{
  Position position{fixture.fen};
  const std::string fixture_id =
      std::string(colour) + '/' + std::string(fixture.id);
  (void)require_semantically_valid_fixture(position, fixture_id, fixture.fen);
  const Score raw = Eval::CountMaterial<Selected>(&position);
  return {raw, static_cast<WideScore>(raw)};
}

template <Colour Selected>
void exercise_material_branch(const std::string_view colour,
                              const std::array<NamedFen, 9> &fixtures)
{
  const std::array<WideScore, 5> named_constants{
      static_cast<WideScore>(Eval::PAWN_VAL),
      static_cast<WideScore>(Eval::KNIGHT_VAL),
      static_cast<WideScore>(Eval::BISHOP_VAL),
      static_cast<WideScore>(Eval::ROOK_VAL),
      static_cast<WideScore>(Eval::QUEEN_VAL),
  };
  INFO("selected-colour=" << colour);
  INFO("Score-value-bits="
       << std::numeric_limits<Score>::digits
       << " WideScore-value-bits=" << std::numeric_limits<WideScore>::digits
       << " WideScore-min=" << std::numeric_limits<WideScore>::min()
       << " WideScore-max=" << std::numeric_limits<WideScore>::max());

  const MaterialObservation base =
      observe_material<Selected>(colour, fixtures[0]);
  for (std::size_t piece = 0; piece < named_constants.size(); ++piece)
  {
    const MaterialObservation single =
        observe_material<Selected>(colour, fixtures[piece + 1]);
    const WideScore difference = single.widened - base.widened;
    INFO("equation=single-minus-base role="
         << fixtures[piece + 1].id << " base-raw=" << base.raw
         << " single-raw=" << single.raw << " base-W=" << base.widened
         << " single-W=" << single.widened << " actual-difference-W="
         << difference << " named-constant-W=" << named_constants[piece]);
    CHECK(difference == named_constants[piece]);
  }

  const MaterialObservation combined =
      observe_material<Selected>(colour, fixtures[6]);
  WideScore named_sum = 0;
  for (const WideScore constant : named_constants)
    named_sum += constant;
  const WideScore combined_difference = combined.widened - base.widened;
  INFO("equation=combined-minus-base base-raw="
       << base.raw << " combined-raw=" << combined.raw
       << " base-W=" << base.widened << " combined-W=" << combined.widened
       << " actual-difference-W=" << combined_difference
       << " five-constant-sum-W=" << named_sum);
  CHECK(combined_difference == named_sum);

  const MaterialObservation isolation_base =
      observe_material<Selected>(colour, fixtures[7]);
  const MaterialObservation isolation_opponent =
      observe_material<Selected>(colour, fixtures[8]);
  INFO("equation=opponent-isolation base-raw="
       << isolation_base.raw << " opponent-added-raw=" << isolation_opponent.raw
       << " base-W=" << isolation_base.widened
       << " opponent-added-W=" << isolation_opponent.widened);
  CHECK(isolation_base.widened == isolation_opponent.widened);
}

constexpr std::array<NamedFen, 3> kEvaluateFixtures{
    NamedFen{"start",
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
    NamedFen{"en_passant_legal", "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2"},
    NamedFen{"random_000003", "rnbqkb1r/1p2pp2/2p2np1/1P1p3p/p3P2P/N4PPN/"
                              "P1PP1K2/R1BQ1B1R b kq - 1 10"},
};

void require_evaluate_fixture_source()
{
  constexpr std::string_view path = "tests/data/oracle_cases.tsv";
  constexpr std::string_view file_hash =
      "d4a6bfd2b0fc73de89ee95552d1be7fd313c38e25fe45197b277ba6a34df4c69";
  constexpr std::string_view data_hash =
      "7c6bfab18aecbf89e6da7cdcc29b258edb13bf916804c9f211cb9752c7cb4e77";
  constexpr std::string_view selected_hash =
      "b4ee87db66f0fa8934d4489a4fb5b67b608df0fd14cd1fc7f84bbb8265ad7448";

  const std::string whole_file = tdfa_test::blind::canonical_lf(
      tdfa_test::blind::read_fixture_bytes(path));
  REQUIRE(tdfa_test::blind::sha256_hex(whole_file) == file_hash);
  const auto fixture = tdfa_test::blind::load_tsv_fixture(path);
  REQUIRE(fixture.metadata.at("data_sha256") == data_hash);

  const std::size_t case_column =
      tdfa_test::blind::column_index(fixture, "case_id");
  const std::size_t fen_column = tdfa_test::blind::column_index(fixture, "fen");
  std::string selected_bytes;
  for (const auto &expected : kEvaluateFixtures)
  {
    std::size_t matches = 0;
    for (const auto &row : fixture.rows)
    {
      if (row[case_column] != expected.id)
        continue;
      ++matches;
      REQUIRE(row[fen_column] == expected.fen);
    }
    REQUIRE(matches == 1);
    selected_bytes.append(expected.id);
    selected_bytes.push_back('\t');
    selected_bytes.append(expected.fen);
    selected_bytes.push_back('\n');
  }
  REQUIRE(selected_bytes.size() == 199);
  REQUIRE(tdfa_test::blind::sha256_hex(selected_bytes) == selected_hash);
}

struct PerftCase
{
  std::string_view id;
  std::string_view fen;
  std::array<U64, 3> nodes;
};

constexpr std::array<PerftCase, 6> kPerftCases{
    PerftCase{"start",
              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
              {U64{20}, U64{400}, U64{8902}}},
    PerftCase{
        "kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        {U64{48}, U64{2039}, U64{97862}}},
    PerftCase{"perft_endgame",
              "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
              {U64{14}, U64{191}, U64{2812}}},
    PerftCase{
        "perft_promotions",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        {U64{6}, U64{264}, U64{9467}}},
    PerftCase{"perft_castling",
              "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
              {U64{44}, U64{1486}, U64{62379}}},
    PerftCase{"perft_tactics",
              "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w "
              "- - 0 10",
              {U64{46}, U64{2079}, U64{89890}}},
};

static_assert(std::numeric_limits<U64>::max() >= U64{97862});

void require_perft_fixture_source()
{
  constexpr std::string_view path = "tests/data/oracle_perft.tsv";
  constexpr std::string_view file_hash =
      "96a933a8203be5095a7e8592cf09b56a27f1602df4ec8795dd7aa6f3ca38c849";
  constexpr std::string_view data_hash =
      "8f7643f4c8168c93b0590634e17ea11db52ddfe167e7c04d04710cb986f40789";

  const std::string whole_file = tdfa_test::blind::canonical_lf(
      tdfa_test::blind::read_fixture_bytes(path));
  REQUIRE(tdfa_test::blind::sha256_hex(whole_file) == file_hash);
  const auto fixture = tdfa_test::blind::load_tsv_fixture(path);
  REQUIRE(fixture.metadata.at("data_sha256") == data_hash);
  const auto rows = tdfa_test::blind::oracle_perft_rows(fixture);
  REQUIRE(rows.size() == 18);

  using Key = std::pair<std::string, unsigned>;
  std::map<Key, std::pair<std::string, std::uint64_t>> indexed;
  for (const auto &row : rows)
  {
    REQUIRE(
        indexed
            .emplace(Key{row.case_id, row.depth}, std::pair{row.fen, row.nodes})
            .second);
  }
  for (const auto &expected : kPerftCases)
  {
    for (unsigned depth = 1; depth <= 3; ++depth)
    {
      const auto found = indexed.find(Key{std::string(expected.id), depth});
      REQUIRE(found != indexed.end());
      REQUIRE(found->second.first == expected.fen);
      REQUIRE(found->second.second ==
              static_cast<std::uint64_t>(expected.nodes[depth - 1]));
    }
  }
}

#if defined(TDFA_RUNTIME_LINUX_CLANG_ASAN)

constexpr std::size_t kDiagnosticStreamLimit = 65536;
constexpr std::string_view kTtChildEnvironment = "TDFA_RUNTIME_TT_CHILD_V3";

struct CapturedStream
{
  std::string retained;
  std::uint64_t total_bytes = 0;

  [[nodiscard]] bool truncated() const { return total_bytes > retained.size(); }

  [[nodiscard]] std::uint64_t omitted_bytes() const
  {
    return total_bytes - static_cast<std::uint64_t>(retained.size());
  }

  void append(const char *bytes, const std::size_t count)
  {
    total_bytes += count;
    const std::size_t available = kDiagnosticStreamLimit - retained.size();
    retained.append(bytes, std::min(available, count));
  }
};

struct TtChildResult
{
  CapturedStream stdout_stream;
  CapturedStream stderr_stream;
  int wait_status = 0;
};

void set_nonblocking(const int descriptor)
{
  const int flags = ::fcntl(descriptor, F_GETFL, 0);
  if (flags < 0 || ::fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::runtime_error("fcntl failed for TT child pipe");
}

void close_descriptor(const int descriptor)
{
  if (descriptor >= 0)
    (void)::close(descriptor);
}

void drain_descriptor(const int descriptor, CapturedStream &capture, bool &open)
{
  std::array<char, 4096> bytes{};
  for (;;)
  {
    const ssize_t count = ::read(descriptor, bytes.data(), bytes.size());
    if (count > 0)
    {
      capture.append(bytes.data(), static_cast<std::size_t>(count));
      continue;
    }
    if (count == 0)
    {
      close_descriptor(descriptor);
      open = false;
      return;
    }
    if (errno == EINTR)
      continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return;
    throw std::runtime_error("read failed for TT child pipe");
  }
}

TtChildResult run_isolated_tt_child()
{
  int stdout_pipe[2]{-1, -1};
  int stderr_pipe[2]{-1, -1};
  if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0)
  {
    close_descriptor(stdout_pipe[0]);
    close_descriptor(stdout_pipe[1]);
    close_descriptor(stderr_pipe[0]);
    close_descriptor(stderr_pipe[1]);
    throw std::runtime_error("pipe failed for TT child");
  }

  const std::string executable =
      std::filesystem::read_symlink("/proc/self/exe").string();

  const pid_t child = ::fork();
  if (child == 0)
  {
    const std::string marker = std::to_string(static_cast<long long>(::getppid())) +
                               ":" +
                               std::to_string(static_cast<long long>(::getpid()));
    constexpr std::string_view controlled_asan =
        "detect_leaks=0:halt_on_error=1:abort_on_error=1:exitcode=86:"
        "symbolize=1";
    if (::setenv(std::string(kTtChildEnvironment).c_str(), marker.c_str(), 1) != 0 ||
        ::setenv("ASAN_OPTIONS", std::string(controlled_asan).c_str(), 1) != 0)
      _exit(126);

    close_descriptor(stdout_pipe[0]);
    close_descriptor(stderr_pipe[0]);
    if (::dup2(stdout_pipe[1], STDOUT_FILENO) < 0 ||
        ::dup2(stderr_pipe[1], STDERR_FILENO) < 0)
    {
      _exit(126);
    }
    close_descriptor(stdout_pipe[1]);
    close_descriptor(stderr_pipe[1]);
    std::array<char *, 5> arguments{
        const_cast<char *>(executable.c_str()),
        const_cast<char *>("[T-RTM-TT-001]"),
        const_cast<char *>("--reporter"),
        const_cast<char *>("console"),
        nullptr,
    };
    ::execv(executable.c_str(), arguments.data());
    constexpr std::string_view message = "execv failed for TT child\n";
    (void)::write(STDERR_FILENO, message.data(), message.size());
    _exit(127);
  }

  close_descriptor(stdout_pipe[1]);
  close_descriptor(stderr_pipe[1]);
  if (child < 0)
  {
    close_descriptor(stdout_pipe[0]);
    close_descriptor(stderr_pipe[0]);
    throw std::runtime_error("fork failed for TT child");
  }

  set_nonblocking(stdout_pipe[0]);
  set_nonblocking(stderr_pipe[0]);
  TtChildResult result;
  bool stdout_open = true;
  bool stderr_open = true;
  while (stdout_open || stderr_open)
  {
    std::array<pollfd, 2> descriptors{
        pollfd{stdout_open ? stdout_pipe[0] : -1, POLLIN, 0},
        pollfd{stderr_open ? stderr_pipe[0] : -1, POLLIN, 0},
    };
    int polled = 0;
    do
    {
      polled = ::poll(descriptors.data(), descriptors.size(), -1);
    } while (polled < 0 && errno == EINTR);
    if (polled < 0)
      throw std::runtime_error("poll failed for TT child");

    if (stdout_open &&
        (descriptors[0].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) != 0)
      drain_descriptor(stdout_pipe[0], result.stdout_stream, stdout_open);
    if (stderr_open &&
        (descriptors[1].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) != 0)
      drain_descriptor(stderr_pipe[0], result.stderr_stream, stderr_open);
  }

  pid_t waited = -1;
  do
  {
    waited = ::waitpid(child, &result.wait_status, 0);
  } while (waited < 0 && errno == EINTR);
  if (waited != child)
    throw std::runtime_error("waitpid failed for TT child");
  return result;
}

bool is_spawned_tt_child()
{
  const char *raw = std::getenv(std::string(kTtChildEnvironment).c_str());
  if (raw == nullptr)
    return false;
  const std::string_view marker(raw);
  const std::size_t separator = marker.find(':');
  if (separator == std::string_view::npos || separator == 0 ||
      separator + 1 == marker.size())
    return false;

  long long expected_parent = -1;
  long long expected_child = -1;
  const auto parent_result = std::from_chars(
      marker.data(), marker.data() + separator, expected_parent);
  const auto child_result = std::from_chars(
      marker.data() + separator + 1, marker.data() + marker.size(),
      expected_child);
  if (parent_result.ec != std::errc{} ||
      parent_result.ptr != marker.data() + separator ||
      child_result.ec != std::errc{} ||
      child_result.ptr != marker.data() + marker.size())
    return false;
  return expected_parent == static_cast<long long>(::getppid()) &&
         expected_child == static_cast<long long>(::getpid());
}

void report_stream(const std::string_view stream_name,
                   const CapturedStream &stream)
{
  INFO("stream=" << stream_name
                 << " captured-byte-count=" << stream.retained.size()
                 << " total-byte-count=" << stream.total_bytes
                 << " truncated=" << std::boolalpha << stream.truncated()
                 << " omitted-byte-count=" << stream.omitted_bytes());
  INFO("stream=" << stream_name << " retained-bytes=" << stream.retained);
}

#endif

} // namespace

TEST_CASE("T-RTM-TT-001 Fresh zero-capacity TT observation and unallocated "
          "destruction",
          "[runtime][tt][T-RTM-TT-001][adversarial]")
{
#if defined(TDFA_RUNTIME_LINUX_CLANG_ASAN)
  if (is_spawned_tt_child())
  {
    std::size_t first = 1;
    std::size_t second = 1;
    {
      TransposTable table;
      first = table.GetNumElems();
      second = table.GetNumElems();
      INFO("test-id=T-RTM-TT-001 operation-ordinal=1 first-size_t="
           << first << " second-size_t=" << second);
      CHECK(first == std::size_t{0});
      CHECK(second == std::size_t{0});
    }
    return;
  }

  const TtChildResult child = run_isolated_tt_child();
  INFO("test-id=T-RTM-TT-001 profile=linux-clang-address compiler=Clang "
       "sanitizer=AddressSanitizer child-count=1");
  report_stream("stdout", child.stdout_stream);
  report_stream("stderr", child.stderr_stream);
  INFO("child-wait-status="
       << child.wait_status << " exited=" << std::boolalpha
       << static_cast<bool>(WIFEXITED(child.wait_status)) << " exit-code="
       << (WIFEXITED(child.wait_status) ? WEXITSTATUS(child.wait_status) : -1)
       << " signaled=" << static_cast<bool>(WIFSIGNALED(child.wait_status))
       << " signal="
       << (WIFSIGNALED(child.wait_status) ? WTERMSIG(child.wait_status) : 0));

  REQUIRE_FALSE(child.stdout_stream.truncated());
  REQUIRE_FALSE(child.stderr_stream.truncated());
  const bool sanitizer_diagnostic =
      child.stdout_stream.retained.find("AddressSanitizer") !=
          std::string::npos ||
      child.stderr_stream.retained.find("AddressSanitizer") !=
          std::string::npos;
  INFO("symbolization-status=" << (sanitizer_diagnostic
                                       ? "diagnostic-present"
                                       : "not-applicable-no-diagnostic"));
  REQUIRE_FALSE(sanitizer_diagnostic);
  REQUIRE(WIFEXITED(child.wait_status));
  REQUIRE(WEXITSTATUS(child.wait_status) == 0);
#else
  SKIP("T-RTM-TT-001 executes only in the frozen linux-clang-address profile");
#endif
}

TEST_CASE("T-RTM-EVAL-001 Selected-colour named material additivity and "
          "opponent isolation",
          "[runtime][eval][T-RTM-EVAL-001][metamorphic]")
{
  constexpr std::string_view fixture_hash =
      "6da29a8150856eaddbb188e0abad1d0603264856e316653a522eae63b83bd14b";
  const std::string serialized_fixtures = material_fixture_serialization();
  INFO("material-fixture-scope-sha256=" << fixture_hash);
  REQUIRE(tdfa_test::blind::sha256_hex(serialized_fixtures) == fixture_hash);

  DYNAMIC_SECTION("selected-colour=White")
  {
    exercise_material_branch<White>("White", kWhiteMaterialFixtures);
  }
  DYNAMIC_SECTION("selected-colour=Black")
  {
    exercise_material_branch<Black>("Black", kBlackMaterialFixtures);
  }
}

TEST_CASE("T-RTM-EVAL-002 Evaluate repeats on identical semantic FEN and "
          "preserves public FEN fields",
          "[runtime][eval][T-RTM-EVAL-002][metamorphic][regression]")
{
  require_evaluate_fixture_source();
  for (const auto &fixture : kEvaluateFixtures)
  {
    DYNAMIC_SECTION("fixture=" << fixture.id)
    {
      INFO("selected-scope-sha256="
           "b4ee87db66f0fa8934d4489a4fb5b67b608df0fd14cd1fc7f84bbb8265ad7448");
      Position first_position{fixture.fen};
      Position second_position{fixture.fen};
      const auto first_before = require_semantically_valid_fixture(
          first_position, std::string(fixture.id) + "/A", fixture.fen);
      const auto second_before = require_semantically_valid_fixture(
          second_position, std::string(fixture.id) + "/B", fixture.fen);

      const Score first_call = Eval::Evaluate(&first_position);
      INFO("object=A call-ordinal=1 returned-Score=" << first_call);
      require_snapshot_unchanged(first_before, first_position,
                                 "Evaluate A call 1");

      const Score second_call = Eval::Evaluate(&first_position);
      INFO("object=A call-ordinal=2 returned-Score=" << second_call);
      require_snapshot_unchanged(first_before, first_position,
                                 "Evaluate A call 2");

      const Score equivalent_call = Eval::Evaluate(&second_position);
      INFO("object=B call-ordinal=1 returned-Score=" << equivalent_call);
      require_snapshot_unchanged(second_before, second_position,
                                 "Evaluate B call 1");

      CHECK(second_call == first_call);
      CHECK(equivalent_call == first_call);
    }
  }
}

TEST_CASE("T-RTM-PERFT-001 Sequential bulk-perft replaces totals and restores "
          "public FEN semantics",
          "[runtime][perft][T-RTM-PERFT-001][exhaustive][regression]")
{
  require_perft_fixture_source();
  for (const auto &fixture : kPerftCases)
  {
    DYNAMIC_SECTION("fixture=" << fixture.id)
    {
      Position position{fixture.fen};
      const auto root =
          require_semantically_valid_fixture(position, fixture.id, fixture.fen);
      PerftHandler handler;
      std::optional<U64> prior_total;
      for (int depth = 1; depth <= 3; ++depth)
      {
        handler.RunBulkPerft<false>(depth, &position);
        const U64 actual = handler.GetNodes();
        const U64 expected = fixture.nodes[static_cast<std::size_t>(depth - 1)];
        INFO("case-id=" << fixture.id << " fen=" << fixture.fen
                        << " corpus-file-sha256="
                           "96a933a8203be5095a7e8592cf09b56a27f1602df4ec8795dd7"
                           "aa6f3ca38c849"
                        << " corpus-data-sha256="
                           "8f7643f4c8168c93b0590634e17ea11db52ddfe167e7c04d047"
                           "10cb986f40789"
                        << " run-ordinal=" << depth << " depth=" << depth
                        << " expected-nodes=" << expected
                        << " actual-nodes=" << actual << " prior-covered-total="
                        << (prior_total ? std::to_string(*prior_total) : "none")
                        << " caller-ResetData-called=false");
        CHECK(actual == expected);
        require_snapshot_unchanged(root, position, "RunBulkPerft<false>");
        prior_total = actual;
      }
    }
  }
}

TEST_CASE(
    "T-RTM-PERFT-002 ResetData clears an existing nonzero perft session only",
    "[runtime][perft][T-RTM-PERFT-002][metamorphic][regression]")
{
  require_perft_fixture_source();
  for (const auto &fixture : kPerftCases)
  {
    DYNAMIC_SECTION("fixture=" << fixture.id)
    {
      Position position{fixture.fen};
      const auto root =
          require_semantically_valid_fixture(position, fixture.id, fixture.fen);
      PerftHandler handler;
      handler.RunBulkPerft<false>(1, &position);
      const U64 before_reset = handler.GetNodes();
      INFO("case-id=" << fixture.id << " fen=" << fixture.fen
                      << " depth=1 expected-nodes=" << fixture.nodes[0]
                      << " actual-nodes=" << before_reset
                      << " caller-ResetData-before-run=false");
      REQUIRE(before_reset == fixture.nodes[0]);
      REQUIRE(before_reset != U64{0});
      require_snapshot_unchanged(root, position, "depth-1 RunBulkPerft<false>");

      handler.ResetData();
      const U64 after_reset = handler.GetNodes();
      INFO("ResetData-ordinal=1 pre-reset-total="
           << before_reset << " post-reset-total=" << after_reset);
      CHECK(after_reset == U64{0});
      require_snapshot_unchanged(root, position, "ResetData outside Position");
    }
  }
}
