#include <catch2/catch_test_macros.hpp>

#include "Search.hpp"

#include "../support/BlindChessModel.hpp"
#include "../support/BlindFixtureData.hpp"
#include "../support/BlindTdfaAdapter.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{

using namespace tdfa_test::blind;

constexpr std::string_view kStartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr std::string_view kBlackStartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";

const SquareBijection &squares()
{
  static const SquareBijection value;
  return value;
}

struct FenSemanticSnapshot
{
  PublicPlacementSnapshot placement;
  Colour turn{White};
  U8 castling_rights{};
  BitBoard en_passant{};
  std::optional<Sq> en_passant_square;
  U8 halfmove{};
  U16 fullmove{};

  friend bool operator==(const FenSemanticSnapshot &,
                         const FenSemanticSnapshot &) = default;
};

FenSemanticSnapshot fen_semantic_snapshot(const Position &position)
{
  FenSemanticSnapshot snapshot;
  snapshot.placement = public_placement(position);
  snapshot.turn = position.ColourToMove();
  snapshot.castling_rights = position.CastlingRights();
  snapshot.en_passant = position.EnPasBB();
  if (snapshot.en_passant != 0)
    snapshot.en_passant_square = position.EnPasSq();
  snapshot.halfmove = position.HalfMoves();
  snapshot.fullmove = position.FullMoves();
  return snapshot;
}

const std::vector<OracleCase> &oracle_cases_fixture()
{
  static const std::vector<OracleCase> cases =
      oracle_cases(load_tsv_fixture("tests/data/oracle_cases.tsv"));
  return cases;
}

const OracleCase &oracle_case(const std::string_view id)
{
  const auto &cases = oracle_cases_fixture();
  const auto found =
      std::find_if(cases.begin(), cases.end(),
                   [&](const OracleCase &row) { return row.case_id == id; });
  if (found == cases.end())
    throw std::runtime_error("required oracle case is absent: " +
                             std::string(id));
  return *found;
}

void validate_fixture(
    Position &position, const RefPosition &reference,
    const std::optional<bool> expected_in_check = std::nullopt,
    const std::optional<bool> expected_has_legal = std::nullopt)
{
  const Side side = reference.turn;
  const auto expected_legal = uci_set(reference.legal_moves(side));
  const FenSemanticSnapshot before_generation =
      fen_semantic_snapshot(position);

  REQUIRE(public_placement(position) ==
          expected_public_placement(reference, squares()));
  REQUIRE(oracle_side(position.ColourToMove()) == side);
  REQUIRE(production_in_check(position, side) == reference.in_check(side));

  MoveList actual_moves;
  generate_legal(position, side, actual_moves);
  REQUIRE(observed_uci_set(actual_moves, squares()) == expected_legal);
  REQUIRE(fen_semantic_snapshot(position) == before_generation);

  if (expected_in_check.has_value())
    REQUIRE(reference.in_check(side) == *expected_in_check);
  if (expected_has_legal.has_value())
    REQUIRE((!expected_legal.empty()) == *expected_has_legal);
}

Position
validated_position(const std::string_view fen,
                   const std::optional<bool> expected_in_check = std::nullopt,
                   const std::optional<bool> expected_has_legal = std::nullopt)
{
  RefPosition reference = RefPosition::from_fen(fen);
  Position position(fen);
  validate_fixture(position, reference, expected_in_check, expected_has_legal);
  return position;
}

Position validated_oracle_position(const std::string_view id)
{
  const OracleCase &row = oracle_case(id);
  RefPosition reference = RefPosition::from_fen(row.fen);
  REQUIRE(reference.in_check(reference.turn) == row.in_check);
  REQUIRE(
      uci_set(reference.legal_moves(reference.turn)) ==
      std::set<std::string>(row.legal_moves.begin(), row.legal_moves.end()));

  Position position(row.fen);
  validate_fixture(position, reference, row.in_check, !row.legal_moves.empty());
  return position;
}

TimeManager three_second_manager()
{
  TimeManager manager;
  // GetTimeAllowance currently derives 3 seconds from this public input.
  // The search depths below remain independently bounded even if that policy
  // changes; the deadline is only a liveness backstop.
  manager.SetOptions(60'000ULL, 0ULL);
  manager.StartTiming();
  (void)manager.OutOfTime();
  return manager;
}

struct ExpiredManagerObservation
{
  TimeManager manager;
  bool observed_timeout{false};
};

ExpiredManagerObservation expired_manager()
{
  ExpiredManagerObservation observation;
  observation.manager.SetOptions(0ULL, 0ULL);
  observation.manager.StartTiming();

  observation.observed_timeout = observation.manager.OutOfTime();
  for (std::size_t attempt = 0;
       attempt < 1'000'000 && !observation.observed_timeout; ++attempt)
    observation.observed_timeout = observation.manager.OutOfTime();
  return observation;
}

class ScopedCoutCapture
{
public:
  ScopedCoutCapture() : previous_state_(std::cout.rdstate())
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

  [[nodiscard]] std::string str() const { return stream_.str(); }

private:
  std::ostringstream stream_;
  std::streambuf *previous_ = nullptr;
  std::ios::iostate previous_state_;
};

struct SearchObservation
{
  Score result{};
  bool fen_semantics_restored{false};
};

SearchObservation run_search_observation(Search &search, Position &position,
                                         TransposTable &table, const U16 depth,
                                         TimeManager &manager,
                                         const Score alpha = Eval::NEG_INF,
                                         const Score beta = Eval::POS_INF)
{
  const FenSemanticSnapshot before = fen_semantic_snapshot(position);
  const Score result =
      search.GoSearch(&table, &position, depth, &manager, alpha, beta);
  return {result, fen_semantic_snapshot(position) == before};
}

} // namespace

TEST_CASE("Audit search visits depth-zero and terminal colour paths", "[audit]")
{
  Search search;
  TransposTable table;
  table.Resize(1);
  if (table.GetNumElems() == 0)
    SKIP("The current table declined the positive audit allocation");

  for (const std::string_view fen : {kStartFen, kBlackStartFen})
  {
    Position position = validated_position(fen, false, true);
    TimeManager manager = three_second_manager();
    const auto observation =
        run_search_observation(search, position, table, 0, manager);
    CAPTURE(fen, observation.result, observation.fen_semantics_restored);
  }

  struct TerminalFixture
  {
    std::string_view fen;
    bool in_check;
  };
  constexpr std::array terminals{
      TerminalFixture{"7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", true},
      TerminalFixture{"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", false},
      TerminalFixture{"8/8/8/8/8/6k1/6q1/7K w - - 0 1", true},
      TerminalFixture{"8/8/8/8/8/6k1/5q2/7K w - - 0 1", false},
  };

  for (const TerminalFixture &fixture : terminals)
  {
    for (const U16 depth : {U16{0}, U16{1}})
    {
      Position position =
          validated_position(fixture.fen, fixture.in_check, false);
      table.Clear();
      TimeManager manager = three_second_manager();
      const auto observation = run_search_observation(
          search, position, table, depth, manager);
      CAPTURE(fixture.fen, fixture.in_check, depth, observation.result,
              observation.fen_semantics_restored);
    }
  }
}

TEST_CASE("Audit search visits timeout draw and transposition shortcuts",
          "[audit]")
{
  Search search;
  TransposTable table;
  table.Resize(1);
  if (table.GetNumElems() == 0)
    SKIP("The current table declined the positive audit allocation");

  {
    Position position = validated_position(kStartFen, false, true);
    auto manager = expired_manager();
    CAPTURE(manager.observed_timeout);
    if (manager.observed_timeout)
    {
      const auto observation = run_search_observation(
          search, position, table, 1, manager.manager);
      CAPTURE(observation.result, observation.fen_semantics_restored);
    }
  }

  {
    Position position = validated_oracle_position("fifty_move_clock_100");
    REQUIRE(position.HalfMoves() == 100);
    TimeManager manager = three_second_manager();
    const auto observation =
        run_search_observation(search, position, table, 1, manager);
    CAPTURE(observation.result, observation.fen_semantics_restored);
  }

  struct StoredShortcut
  {
    BoundType bound;
    Score stored;
    Score alpha;
    Score beta;
  };
  constexpr std::array shortcuts{
      StoredShortcut{BoundType::EXACT_VAL, 123, -200, 200},
      StoredShortcut{BoundType::UPPER_BOUND, -25, 0, 100},
      StoredShortcut{BoundType::LOWER_BOUND, 25, -100, 0},
  };

  for (const StoredShortcut &shortcut : shortcuts)
  {
    Position position = validated_position(kStartFen, false, true);
    table.Clear();
    table.Store(position.ZKey(), shortcut.stored, Moves::NULL_MOVE, 2,
                shortcut.bound);

    TimeManager manager = three_second_manager();
    const auto observation = run_search_observation(
        search, position, table, 2, manager, shortcut.alpha, shortcut.beta);
    CAPTURE(shortcut.bound, shortcut.stored, shortcut.alpha, shortcut.beta,
            observation.result, observation.fen_semantics_restored);
  }
}

TEST_CASE("Audit shallow search visits capture sorting and window updates",
          "[audit]")
{
  Search search;

  struct SearchFixture
  {
    std::string_view id;
    std::string fen;
    U16 depth;
  };

  const std::array fixtures{
      SearchFixture{"kiwipete", oracle_case("kiwipete").fen, 2},
      SearchFixture{"queen-captures-rook", "7k/8/8/8/3r4/8/3Q4/K7 w - - 0 1",
                    1},
      SearchFixture{"king-captures-checking-pawn",
                    "7k/8/8/8/8/8/1p6/K7 w - - 0 1", 1},
  };

  for (const SearchFixture &fixture : fixtures)
  {
    Position position = fixture.id == "kiwipete"
                            ? validated_oracle_position(fixture.id)
                            : validated_position(fixture.fen);
    TransposTable table;
    table.Resize(1);
    if (table.GetNumElems() == 0)
    {
      WARN("The current table declined the positive audit allocation");
      continue;
    }

    TimeManager manager = three_second_manager();
    const auto observation = run_search_observation(
        search, position, table, fixture.depth, manager);
    CAPTURE(fixture.id, fixture.depth, observation.result,
            observation.fen_semantics_restored);
  }

  Position cutoff_position = validated_position(kStartFen, false, true);
  TransposTable cutoff_table;
  cutoff_table.Resize(1);
  if (cutoff_table.GetNumElems() == 0)
    SKIP("The current table declined the positive audit allocation");
  TimeManager manager = three_second_manager();
  constexpr Score narrow_beta = static_cast<Score>(Eval::NEG_INF + Score{1});
  const auto observation = run_search_observation(
      search, cutoff_position, cutoff_table, 1, manager, Eval::NEG_INF,
      narrow_beta);
  CAPTURE(narrow_beta, observation.result,
          observation.fen_semantics_restored);
}

TEST_CASE("Audit zero-time best-move entry is bounded for both colours",
          "[audit]")
{
  Search search;

  for (const std::string_view fen : {kStartFen, kBlackStartFen})
  {
    Position position = validated_position(fen, false, true);
    const FenSemanticSnapshot before = fen_semantic_snapshot(position);
    TransposTable table;
    table.Resize(1);
    if (table.GetNumElems() == 0)
    {
      WARN("The current table declined the positive audit allocation");
      continue;
    }
    auto manager = expired_manager();
    if (!manager.observed_timeout)
    {
      WARN("The live clock did not expose the zero-time audit path");
      continue;
    }

    const auto *original_buffer = std::cout.rdbuf();
    Move result = Moves::NULL_MOVE;
    std::string output;
    {
      ScopedCoutCapture capture;
      result = search.FindBestMove(&position, &table, &manager.manager);
      output = capture.str();
    }

    CAPTURE(fen, result, output);
    REQUIRE(std::cout.rdbuf() == original_buffer);
    CAPTURE(before == fen_semantic_snapshot(position));
  }
}
