#include <catch2/catch_test_macros.hpp>

#include "../support/BlindChessModel.hpp"
#include "../support/BlindFixtureData.hpp"
#include "../support/BlindTdfaAdapter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace tdfa_test::blind;

const SquareBijection& square_bijection() {
    static const SquareBijection value;
    return value;
}

struct FenSemanticSnapshot {
    PublicPlacementSnapshot placement;
    std::set<char> castling_rights;
    BitBoard en_passant{};
    std::optional<Sq> en_passant_square;
    U8 halfmove{};
    U16 fullmove{};

    friend bool operator==(const FenSemanticSnapshot&,
                           const FenSemanticSnapshot&) = default;
};

FenSemanticSnapshot fen_semantic_snapshot(const Position& position) {
    FenSemanticSnapshot result;
    result.placement = public_placement(position);
    const U8 rights = position.CastlingRights();
    if ((rights & Magics::CASTLE_K_W) != 0) result.castling_rights.insert('K');
    if ((rights & Magics::CASTLE_Q_W) != 0) result.castling_rights.insert('Q');
    if ((rights & Magics::CASTLE_K_B) != 0) result.castling_rights.insert('k');
    if ((rights & Magics::CASTLE_Q_B) != 0) result.castling_rights.insert('q');
    result.en_passant = position.EnPasBB();
    if (result.en_passant != 0) result.en_passant_square = position.EnPasSq();
    result.halfmove = position.HalfMoves();
    result.fullmove = position.FullMoves();
    return result;
}

struct NamedFixture {
    std::string id;
    RefPosition reference;
};

const OracleCase& require_oracle_case(const std::vector<OracleCase>& rows,
                                      const std::string_view id) {
    const auto found = std::find_if(rows.begin(), rows.end(), [&](const OracleCase& row) {
        return row.case_id == id;
    });
    if (found == rows.end())
        throw std::runtime_error("required oracle case is absent: " + std::string(id));
    return *found;
}

std::vector<NamedFixture> exact_rule_fixtures(const std::vector<OracleCase>& rows) {
    std::vector<NamedFixture> result;
    const auto append_fen = [&](const std::string& id, const std::string& fen) {
        result.push_back({id, RefPosition::from_fen(fen)});
    };
    const auto append_case = [&](const std::string_view id) {
        const OracleCase& row = require_oracle_case(rows, id);
        append_fen("oracle/" + row.case_id, row.fen);
    };

    append_case("en_passant_legal");
    append_case("en_passant_pinned");
    append_fen("rule002/check-evasion",
               "k7/8/8/3pP3/4K3/8/8/8 w - d6 0 2");
    append_fen("rule002/horizontal-discovery",
               "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 2");

    append_case("castling_minimal");
    append_fen("rule003/white-K",
               "r3k2r/8/8/8/8/8/8/R3K2R w K - 0 1");
    append_fen("rule003/white-Q",
               "r3k2r/8/8/8/8/8/8/R3K2R w Q - 0 1");
    append_fen("rule003/black-full",
               "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    append_fen("rule003/black-k",
               "r3k2r/8/8/8/8/8/8/R3K2R b k - 0 1");
    append_fen("rule003/black-q",
               "r3k2r/8/8/8/8/8/8/R3K2R b q - 0 1");

    for (const std::string rights : {"-", "K", "Q"}) {
        append_fen("rule004/rights-" + rights,
                   "8/3k4/8/8/8/8/8/R3K2R w " + rights + " - 0 1");
    }
    for (const std::string blocker : {"b1", "c1", "d1", "f1", "g1"}) {
        RefPosition position = RefPosition::from_fen(
            "8/3k4/8/8/8/8/8/R3K2R w KQ - 0 1");
        position.board[static_cast<std::size_t>(square_index(blocker))] = 'N';
        result.push_back({"rule004/blocker-" + blocker, position});
    }
    for (const std::string attacker : {"e8", "f8", "g8", "d8", "c8", "b8"}) {
        RefPosition position = RefPosition::from_fen(
            "8/3k4/8/8/8/8/8/R3K2R w KQ - 0 1");
        position.board[static_cast<std::size_t>(square_index(attacker))] = 'r';
        result.push_back({"rule004/attacker-" + attacker, position});
    }
    append_fen("rule004/rook-square-h1",
               "7r/3k4/8/8/8/8/8/R3K2R w KQ - 0 1");
    append_fen("rule004/rook-square-a1",
               "r7/3k4/8/8/8/8/8/R3K2R w KQ - 0 1");

    append_case("promotion_choices");
    append_fen("rule006/white-quiet",
               "4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    append_fen("rule006/white-capture",
               "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    append_fen("rule006/black-quiet",
               "4k3/8/8/8/8/8/p7/4K3 b - - 0 1");
    append_fen("rule006/black-capture",
               "4k3/8/8/8/8/8/p7/1R2K3 b - - 0 1");

    append_case("checkmate");
    append_case("stalemate");
    append_case("perft_promotions");
    append_case("start");
    return result;
}

const std::set<std::string>& exact_rule_fixture_ids() {
    static const std::set<std::string> ids{
        "oracle/en_passant_legal",
        "oracle/en_passant_pinned",
        "rule002/check-evasion",
        "rule002/horizontal-discovery",
        "oracle/castling_minimal",
        "rule003/white-K",
        "rule003/white-Q",
        "rule003/black-full",
        "rule003/black-k",
        "rule003/black-q",
        "rule004/rights--",
        "rule004/rights-K",
        "rule004/rights-Q",
        "rule004/blocker-b1",
        "rule004/blocker-c1",
        "rule004/blocker-d1",
        "rule004/blocker-f1",
        "rule004/blocker-g1",
        "rule004/attacker-e8",
        "rule004/attacker-f8",
        "rule004/attacker-g8",
        "rule004/attacker-d8",
        "rule004/attacker-c8",
        "rule004/attacker-b8",
        "rule004/rook-square-h1",
        "rule004/rook-square-a1",
        "oracle/promotion_choices",
        "rule006/white-quiet",
        "rule006/white-capture",
        "rule006/black-quiet",
        "rule006/black-capture",
        "oracle/checkmate",
        "oracle/stalemate",
        "oracle/perft_promotions",
        "oracle/start",
    };
    return ids;
}

void require_exact_rule_fixture_scope(
    const std::vector<NamedFixture>& fixtures) {
    std::set<std::string> actual;
    for (const auto& fixture : fixtures) {
        INFO("exact-rule-fixture-id=" << fixture.id);
        REQUIRE(actual.insert(fixture.id).second);
    }
    REQUIRE(actual == exact_rule_fixture_ids());
    REQUIRE(fixtures.size() == exact_rule_fixture_ids().size());
}

std::string rank_flip_label(const std::string_view label) {
    const int square = square_index(label);
    return square_name(index_of(file_of(square), 7 - rank_of(square)));
}

std::string rank_flip_uci(const std::string_view uci) {
    if (uci.size() != 4 && uci.size() != 5)
        throw std::runtime_error("rank-flip received a noncanonical logical move");
    std::string result = rank_flip_label(uci.substr(0, 2)) +
                         rank_flip_label(uci.substr(2, 2));
    if (uci.size() == 5) result.push_back(uci[4]);
    return result;
}

std::set<std::string> rank_flip_labels(const std::set<std::string>& labels) {
    std::set<std::string> result;
    for (const auto& label : labels) result.insert(rank_flip_label(label));
    return result;
}

std::set<std::string> rank_flip_moves(const std::set<std::string>& moves) {
    std::set<std::string> result;
    for (const auto& move : moves) result.insert(rank_flip_uci(move));
    return result;
}

std::set<std::string> expected_castling_moves(const RefPosition& reference) {
    const auto with_castling = uci_set(
        reference.pseudo_moves(reference.turn, 'k', true));
    const auto without_castling = uci_set(
        reference.pseudo_moves(reference.turn, 'k', false));
    std::set<std::string> result;
    for (const auto& move : with_castling) {
        if (!without_castling.contains(move)) result.insert(move);
    }
    return result;
}

void require_equal_sets(const std::set<std::string>& actual,
                        const std::set<std::string>& expected,
                        std::string_view context);

struct CheckedGeneration {
    std::set<std::string> logical_set;
    std::vector<ObservedMove> prefix;
    std::size_t capacity{};
};

template<typename Invoke>
std::optional<CheckedGeneration> checked_generator_call(
    const std::string& fen, const std::set<std::string>& expected,
    const std::string_view context, Invoke&& invoke) {
    INFO("generator=" << context << " fen=" << fen);
    MoveList list;
    const std::size_t capacity = list.all().size();
    if (expected.size() > capacity) {
        INFO("unavailable: expected-cardinality=" << expected.size()
             << " observed-capacity=" << capacity);
        return std::nullopt;
    }

    Position position(fen);
    const FenSemanticSnapshot before = fen_semantic_snapshot(position);
    std::invoke(std::forward<Invoke>(invoke), position, list);
    REQUIRE(list.len() <= capacity);
    const std::vector<ObservedMove> prefix = observe_moves(list, square_bijection());
    std::set<std::string> actual;
    for (const auto& move : prefix) actual.insert(move.uci);
    require_equal_sets(actual, expected, context);
    REQUIRE(fen_semantic_snapshot(position) == before);
    return CheckedGeneration{std::move(actual), prefix, capacity};
}

template<typename Query>
auto checked_position_query(Position& position, const std::string_view context,
                            Query&& query) {
    INFO("query=" << context);
    const FenSemanticSnapshot before = fen_semantic_snapshot(position);
    auto result = std::invoke(std::forward<Query>(query), position);
    REQUIRE(fen_semantic_snapshot(position) == before);
    return result;
}

std::set<std::string> leaper_geometry(const std::string_view origin,
                                      const std::vector<std::pair<int, int>>& deltas) {
    const int square = square_index(origin);
    std::set<std::string> result;
    for (const auto [df, dr] : deltas) {
        const int file = file_of(square) + df;
        const int rank = rank_of(square) + dr;
        if (on_board(file, rank)) result.insert(square_name(index_of(file, rank)));
    }
    return result;
}

const std::vector<std::pair<int, int>>& knight_deltas() {
    static const std::vector<std::pair<int, int>> value{
        {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    return value;
}

const std::vector<std::pair<int, int>>& king_deltas() {
    static const std::vector<std::pair<int, int>> value{
        {-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    return value;
}

void require_equal_sets(const std::set<std::string>& actual,
                        const std::set<std::string>& expected,
                        const std::string_view context) {
    INFO("context=" << context);
    INFO("expected={" << print_set(expected) << "}");
    INFO("actual={" << print_set(actual) << "}");
    REQUIRE(actual == expected);
}

RefPosition king_geometry_shell(const Side side, const int origin) {
    RefPosition shell;
    shell.turn = side;
    shell.castling = "-";
    shell.board[static_cast<std::size_t>(origin)] = piece_for(side, 'k');
    for (int enemy = 0; enemy < 64; ++enemy) {
        if (enemy == origin) continue;
        if (std::max(std::abs(file_of(enemy) - file_of(origin)),
                     std::abs(rank_of(enemy) - rank_of(origin))) < 3) continue;
        RefPosition candidate = shell;
        candidate.board[static_cast<std::size_t>(enemy)] = piece_for(opposite(side), 'k');
        if (!candidate.in_check(side) && !candidate.in_check(opposite(side))) return candidate;
    }
    throw std::runtime_error("no deterministic separated king shell");
}

std::vector<std::string> legal_fixture_fens() {
    return {
        "k3r3/8/8/8/8/8/4R3/4K3 w - - 0 1",
        "k3r3/8/8/8/8/8/4N3/4K3 w - - 0 1",
        "7k/8/8/8/1b6/8/3B4/4K3 w - - 0 1",
        "7k/8/8/8/1b6/4n3/3P4/4K3 w - - 0 1",
        "k3r3/8/8/1B6/8/8/8/3QK3 w - - 0 1",
        "k7/8/8/8/8/5n2/8/4K3 w - - 0 1",
        "k7/8/8/8/3p4/4K3/8/8 w - - 0 1",
        "k3r3/8/8/8/8/5n2/1R4P1/4K3 w - - 0 1",
        "k3r3/8/8/8/1b6/8/8/4K3 w - - 0 1",
        "8/8/8/8/8/4k3/8/4K3 w - - 0 1",
        "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1",
        "k7/8/8/1b6/8/8/4r3/4K3 w - - 0 1",
        "k7/8/8/8/8/8/8/4K2r w - - 0 1"
    };
}

} // namespace

TEST_CASE("T-GEO-001 Exhaustive knight geometry on all origins",
          "[movegen][T-GEO-001][geometry][exhaustive]") {
    const auto& squares = square_bijection();
    constexpr auto generated_masks = Magics::KnightAttackingMask();
    for (int raw = 0; raw < 64; ++raw) {
        const auto square = static_cast<Sq>(raw);
        const std::string origin = squares.label(square);
        const auto expected = leaper_geometry(origin, knight_deltas());
        INFO("origin=" << origin << " raw-index=" << raw);
        require_equal_sets(squares.labels(generated_masks[static_cast<std::size_t>(raw)]),
                           expected, "consteval knight mask");
        require_equal_sets(squares.labels(Magics::KNIGHT_ATTACK_MASKS[static_cast<std::size_t>(raw)]),
                           expected, "inline knight mask");
        for (const Side side : {Side::white, Side::black}) {
            RefPosition pieces;
            pieces.turn = side;
            pieces.castling = "-";
            pieces.board[static_cast<std::size_t>(square_index(origin))] = piece_for(side, 'n');
            const auto shell = add_legal_king_shell(pieces);
            REQUIRE(shell.has_value());
            const std::string fen = shell->fen();
            Position position(fen);
            MoveList list;
            generate_piece(position, side, 'n', list);
            require_equal_sets(observed_uci_set(list, squares),
                               uci_set(shell->pseudo_moves(side, 'n', false)),
                               std::string("knight generator ") + (side == Side::white ? "white" : "black"));
        }
    }
}

TEST_CASE("T-GEO-002 Exhaustive king step geometry on all origins",
          "[movegen][T-GEO-002][geometry][exhaustive]") {
    const auto& squares = square_bijection();
    constexpr auto generated_masks = Magics::KingAttackingMask();
    for (int raw = 0; raw < 64; ++raw) {
        const auto square = static_cast<Sq>(raw);
        const std::string origin = squares.label(square);
        const auto expected = leaper_geometry(origin, king_deltas());
        INFO("origin=" << origin << " raw-index=" << raw);
        require_equal_sets(squares.labels(generated_masks[static_cast<std::size_t>(raw)]),
                           expected, "consteval king mask");
        require_equal_sets(squares.labels(Magics::KING_ATTACK_MASKS[static_cast<std::size_t>(raw)]),
                           expected, "inline king mask");
        for (const Side side : {Side::white, Side::black}) {
            const RefPosition shell = king_geometry_shell(side, square_index(origin));
            const std::string fen = shell.fen();
            Position position(fen);
            MoveList list;
            generate_piece(position, side, 'k', list);
            require_equal_sets(observed_uci_set(list, squares),
                               uci_set(shell.pseudo_moves(side, 'k', false)),
                               std::string("king generator ") + (side == Side::white ? "white" : "black"));
        }
    }
}

TEST_CASE("T-GEO-004 Destination-bitboard emission basis and union property",
          "[movegen][T-GEO-004][geometry][exhaustive][metamorphic]") {
    const auto& squares = square_bijection();
    const std::array<MoveType, 8> types{mt_Quiet, mt_EnPassant, mt_Castling, mt_Capture,
        mt_QueenPromotion, mt_RookPromotion, mt_BishopPromotion, mt_KnightPromotion};
    auto expected_text = [&](const Sq from, const Sq to, const MoveType type) {
        std::string text = squares.label(from) + squares.label(to);
        if (const char suffix = promotion_suffix(type); suffix != 0) text.push_back(suffix);
        return text;
    };
    const auto observe_exact = [&](MoveList& list, const std::size_t capacity,
                                   const Sq source, const MoveType type) {
        REQUIRE(list.len() <= capacity);
        std::set<std::string> result;
        for (const auto& move : observe_moves(list, squares)) {
            INFO("requested-source=" << squares.label(source)
                 << " requested-type=" << move_type_name(type)
                 << " actual=" << move.uci
                 << " actual-type=" << move_type_name(move.type));
            REQUIRE(move.from == source);
            REQUIRE(move.to != source);
            REQUIRE(move.type == type);
            result.insert(move.uci);
        }
        return result;
    };
    for (int source_value = 0; source_value < 64; ++source_value) {
        const auto source = static_cast<Sq>(source_value);
        for (const MoveType type : types) {
            INFO("source=" << squares.label(source) << " type=" << move_type_name(type));
            MoveList empty;
            const std::size_t empty_capacity = empty.all().size();
            MoveGen::GenerateMovesFromBB(0, &empty, source, type);
            REQUIRE(observe_exact(empty, empty_capacity, source, type).empty());

            std::set<std::string> all_expected;
            std::vector<Sq> targets;
            for (int target_value = 0; target_value < 64; ++target_value) {
                if (target_value == source_value) continue;
                const auto target = static_cast<Sq>(target_value);
                targets.push_back(target);
                all_expected.insert(expected_text(source, target, type));
            }

            MoveList capacity_probe;
            const std::size_t capacity = capacity_probe.all().size();
            if (capacity == 0) {
                INFO("nonempty emission unavailable because observable capacity is zero");
                continue;
            }

            for (const Sq target : targets) {
                MoveList singleton;
                const std::size_t singleton_capacity = singleton.all().size();
                REQUIRE(singleton_capacity > 0);
                MoveGen::GenerateMovesFromBB(
                    BitBoard{1} << static_cast<unsigned>(target), &singleton, source, type);
                const std::set<std::string> expected{expected_text(source, target, type)};
                require_equal_sets(observe_exact(singleton, singleton_capacity, source, type),
                                   expected, "singleton basis");
            }

            const auto exercise_partition = [&](const std::vector<Sq>& ordered_targets,
                                                const std::string_view context) {
                std::set<std::string> result;
                for (std::size_t begin = 0; begin < ordered_targets.size(); begin += capacity) {
                    const std::size_t end = std::min(ordered_targets.size(), begin + capacity);
                    BitBoard subset = 0;
                    std::set<std::string> expected_subset;
                    for (std::size_t index = begin; index < end; ++index) {
                        const Sq target = ordered_targets[index];
                        subset |= BitBoard{1} << static_cast<unsigned>(target);
                        expected_subset.insert(expected_text(source, target, type));
                    }
                    MoveList list;
                    const std::size_t list_capacity = list.all().size();
                    REQUIRE(expected_subset.size() <= list_capacity);
                    MoveGen::GenerateMovesFromBB(subset, &list, source, type);
                    const auto actual_subset = observe_exact(list, list_capacity, source, type);
                    require_equal_sets(actual_subset, expected_subset, context);
                    result.insert(actual_subset.begin(), actual_subset.end());
                }
                return result;
            };
            const auto forward_union = exercise_partition(targets, "forward fitting partition");
            const std::vector<Sq> reversed(targets.rbegin(), targets.rend());
            const auto reverse_union = exercise_partition(reversed, "reverse fitting partition");
            require_equal_sets(forward_union, all_expected, "complete forward partition union");
            require_equal_sets(reverse_union, all_expected, "complete regrouped partition union");
            REQUIRE(forward_union == reverse_union);
        }
    }
}

TEST_CASE("T-LEGAL-001 Pseudo-legal versus legal moves for absolute pins",
          "[movegen][T-LEGAL-001][legal][pins]") {
    const auto& squares = square_bijection();
    const auto fixtures = legal_fixture_fens();
    const std::vector<std::string> white_fens(fixtures.begin(), fixtures.begin() + 4);
    for (const auto& white_fen : white_fens) {
        for (const RefPosition reference : {RefPosition::from_fen(white_fen),
                                            rank_flip_colour_swap(RefPosition::from_fen(white_fen))}) {
            const std::string fen = reference.fen();
            INFO("fen=" << fen);
            Position pseudo_position(fen);
            Position legal_position(fen);
            MoveList pseudo;
            MoveList legal;
            generate_pseudo(pseudo_position, reference.turn, pseudo);
            generate_legal(legal_position, reference.turn, legal);
            require_equal_sets(observed_uci_set(pseudo, squares), uci_set(reference.pseudo_moves(reference.turn)), "pseudo pin layer");
            require_equal_sets(observed_uci_set(legal, squares), uci_set(reference.legal_moves(reference.turn)), "legal pin layer");
        }
    }
}

TEST_CASE("T-LEGAL-002 Complete single-check evasion set",
          "[movegen][T-LEGAL-002][legal][check]") {
    const auto& squares = square_bijection();
    std::vector<std::string> fens{
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "k3r3/8/8/1B6/8/8/8/3QK3 w - - 0 1",
        "k7/8/8/8/8/5n2/8/4K3 w - - 0 1",
        "k7/8/8/8/3p4/4K3/8/8 w - - 0 1"};
    const std::set<std::string> frozen_perft_promotions{"b4c5","c4c5","d2d4","f1f2","f3d4","g1h1"};
    for (const auto& white_fen : fens) {
        for (const RefPosition reference : {RefPosition::from_fen(white_fen),
                                            rank_flip_colour_swap(RefPosition::from_fen(white_fen))}) {
            const std::string fen = reference.fen();
            Position position(fen);
            MoveList legal;
            generate_legal(position, reference.turn, legal);
            const auto actual = observed_uci_set(legal, squares);
            require_equal_sets(actual, uci_set(reference.legal_moves(reference.turn)), fen);
            if (fen == fens.front()) REQUIRE(actual == frozen_perft_promotions);
        }
    }
}

TEST_CASE("T-LEGAL-003 Double check permits only legal king moves",
          "[movegen][T-LEGAL-003][legal][double-check]") {
    const auto& squares = square_bijection();
    for (const std::string white_fen : {
             "k3r3/8/8/8/8/5n2/1R4P1/4K3 w - - 0 1",
             "k3r3/8/8/8/1b6/8/8/4K3 w - - 0 1"}) {
        for (const RefPosition reference : {RefPosition::from_fen(white_fen),
                                            rank_flip_colour_swap(RefPosition::from_fen(white_fen))}) {
            const std::string fen = reference.fen();
            Position position(fen);
            MoveList legal;
            generate_legal(position, reference.turn, legal);
            require_equal_sets(observed_uci_set(legal, squares), uci_set(reference.legal_moves(reference.turn)), fen);
            for (const auto& move : observe_moves(legal, squares)) {
                REQUIRE(reference.board[static_cast<std::size_t>(square_index(squares.label(move.from)))] == piece_for(reference.turn, 'k'));
            }
        }
    }
}

TEST_CASE("T-LEGAL-004 King destinations account for attacks after captures and vacating origin",
          "[movegen][T-LEGAL-004][legal][king-safety]") {
    const auto& squares = square_bijection();
    const auto fixtures = legal_fixture_fens();
    for (auto iterator = fixtures.begin() + 9; iterator != fixtures.end(); ++iterator) {
        const RefPosition reference = RefPosition::from_fen(*iterator);
        const std::string fen = reference.fen();
        INFO("fen=" << fen);
        Position pseudo_position(fen);
        Position legal_position(fen);
        MoveList pseudo_king;
        MoveList legal;
        generate_piece(pseudo_position, reference.turn, 'k', pseudo_king);
        generate_legal(legal_position, reference.turn, legal);
        require_equal_sets(observed_uci_set(pseudo_king, squares),
                           uci_set(reference.pseudo_moves(reference.turn, 'k', false)), "pseudo king");
        require_equal_sets(observed_uci_set(legal, squares),
                           uci_set(reference.legal_moves(reference.turn)), "legal final occupancy");
    }
}

TEST_CASE("T-STATE-002 Make and unmake every generated legal move restores root state",
          "[movegen][T-STATE-002][state][exhaustive]") {
    const auto cases_fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto perft_fixture = load_tsv_fixture("tests/data/oracle_perft.tsv");
    const auto oracle_rows = oracle_cases(cases_fixture);
    const auto perft_rows = oracle_perft_rows(perft_fixture);
    const auto& squares = square_bijection();
    std::vector<NamedFixture> roots;
    for (const auto& row : oracle_rows) {
        roots.push_back({"oracle/" + row.case_id,
                         RefPosition::from_fen(row.fen)});
    }
    std::set<std::string> perft_ids;
    for (const auto& row : perft_rows) {
        if (perft_ids.insert(row.case_id).second) {
            roots.push_back({"perft/" + row.case_id,
                             RefPosition::from_fen(row.fen)});
        }
    }
    const auto rule_fixtures = exact_rule_fixtures(oracle_rows);
    require_exact_rule_fixture_scope(rule_fixtures);
    for (const auto& fixture : rule_fixtures) {
        roots.push_back({fixture.id, fixture.reference});
        roots.push_back({fixture.id + "/rank-flip-colour-swap",
                         rank_flip_colour_swap(fixture.reference)});
    }

    bool saw_quiet = false;
    bool saw_capture = false;
    bool saw_double_push = false;
    bool saw_en_passant = false;
    bool saw_king_side_castle = false;
    bool saw_queen_side_castle = false;
    std::array<std::array<bool, 4>, 2> promotions{};
    std::size_t fitting_roots = 0;
    std::size_t unavailable_roots = 0;
    std::size_t transition_checks = 0;

    for (const auto& fixture : roots) {
        const RefPosition& root_reference = fixture.reference;
        const std::string fen = root_reference.fen();
        INFO("fixture=" << fixture.id << " fen=" << fen);
        const std::set<std::string> expected =
            uci_set(root_reference.legal_moves(root_reference.turn));
        const auto generated = checked_generator_call(
            fen, expected, "state transition legal generation",
            [&](Position& position, MoveList& list) {
                generate_legal(position, root_reference.turn, list);
            });
        if (!generated) {
            ++unavailable_roots;
            continue;
        }
        ++fitting_roots;

        for (const auto& observed : generated->prefix) {
            INFO("fixture=" << fixture.id << " fen=" << fen
                 << " move=" << observed.uci);
            const LogicalMove logical =
                root_reference.legal_move_from_uci(observed.uci);
            const char moving = root_reference.board[
                static_cast<std::size_t>(logical.from)];
            const char kind = kind_of(moving);
            const bool occupied_target = root_reference.occupied(logical.to);
            const bool is_castle = kind == 'k' &&
                std::abs(file_of(logical.to) - file_of(logical.from)) == 2;
            const bool is_en_passant = kind == 'p' &&
                file_of(logical.from) != file_of(logical.to) &&
                !occupied_target && root_reference.en_passant &&
                logical.to == *root_reference.en_passant;
            const bool is_double_push = kind == 'p' &&
                std::abs(rank_of(logical.to) - rank_of(logical.from)) == 2;

            if (logical.promotion != 0) {
                const std::string_view suffixes{"qrbn"};
                const std::size_t promotion_index = suffixes.find(logical.promotion);
                REQUIRE(promotion_index != std::string_view::npos);
                const std::size_t side_index =
                    root_reference.turn == Side::white ? 0U : 1U;
                promotions[side_index][promotion_index] = true;
            }
            if (occupied_target) saw_capture = true;
            if (is_en_passant) saw_en_passant = true;
            if (is_double_push) saw_double_push = true;
            if (is_castle) {
                if (file_of(logical.to) == 6) saw_king_side_castle = true;
                if (file_of(logical.to) == 2) saw_queen_side_castle = true;
            }
            if (!occupied_target && !is_en_passant && !is_double_push &&
                !is_castle && logical.promotion == 0) {
                saw_quiet = true;
            }

            Position transition(fen);
            const PublicPlacementSnapshot before = public_placement(transition);
            const RefPosition child = root_reference.after(logical);
            transition.MakeMove(observed.raw);
            const PublicPlacementSnapshot expected_child =
                expected_public_placement(child, squares);
            REQUIRE(public_placement(transition) == expected_child);
            transition.UnmakeMove(observed.raw);
            REQUIRE(public_placement(transition) == before);
            ++transition_checks;
        }
    }

    REQUIRE(oracle_rows.size() == 27);
    REQUIRE(perft_ids.size() == 6);
    if (unavailable_roots != 0) {
        SKIP("T-STATE-002 cannot prove its complete frozen root set because "
             "one or more expected legal sets exceed observed MoveList capacity");
    }
    REQUIRE(fitting_roots > 0);
    REQUIRE(transition_checks > 0);
    REQUIRE(saw_quiet);
    REQUIRE(saw_capture);
    REQUIRE(saw_double_push);
    REQUIRE(saw_en_passant);
    REQUIRE(saw_king_side_castle);
    REQUIRE(saw_queen_side_castle);
    for (const auto& side_promotions : promotions) {
        for (const bool seen : side_promotions) REQUIRE(seen);
    }
    INFO("unavailable-roots=" << unavailable_roots);
}

TEST_CASE("T-ORACLE-002 Differential legal sets and check flags for every offline corpus row",
          "[movegen][T-ORACLE-002][oracle][differential]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto rows = oracle_cases(fixture);
    std::size_t checked_rows = 0;
    std::size_t unavailable_rows = 0;
    for (const auto& row : rows) {
        INFO("case_id=" << row.case_id << " fen=" << row.fen);
        const Side side = row.turn == 'w' ? Side::white : Side::black;
        const std::set<std::string> expected(
            row.legal_moves.begin(), row.legal_moves.end());

        Position query_position(row.fen);
        REQUIRE(oracle_side(query_position.ColourToMove()) == side);
        const bool actual_check = checked_position_query(
            query_position, "oracle differential InCheck",
            [&](Position& position) {
                return production_in_check(position, side);
            });
        REQUIRE(actual_check == row.in_check);
        if (row.in_check) ++checked_rows;

        const auto generated = checked_generator_call(
            row.fen, expected, row.case_id,
            [&](Position& position, MoveList& list) {
                generate_legal(position, side, list);
            });
        if (!generated) ++unavailable_rows;
    }
    if (unavailable_rows != 0) {
        SKIP("T-ORACLE-002 cannot prove every frozen differential row because "
             "an expected legal set exceeds observed MoveList capacity");
    }
    REQUIRE(checked_rows == 2);
}

TEST_CASE("T-ORACLE-002 Oracle-only mailbox model reproduces every frozen row",
          "[movegen][T-ORACLE-002][oracle][model][self-check]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto rows = oracle_cases(fixture);
    std::size_t replayed_tokens = 0;
    std::size_t history_rows = 0;

    for (const auto& row : rows) {
        INFO("case_id=" << row.case_id << " fen=" << row.fen);
        const RefPosition imported = RefPosition::from_fen(row.fen);
        const Side expected_turn = row.turn == 'w' ? Side::white : Side::black;
        const std::set<std::string> expected_legal(
            row.legal_moves.begin(), row.legal_moves.end());
        REQUIRE(imported.turn == expected_turn);
        REQUIRE(imported.in_check(imported.turn) == row.in_check);
        require_equal_sets(uci_set(imported.legal_moves(imported.turn)),
                           expected_legal, "oracle-only frozen legal set");

        RefPosition replay = RefPosition::from_fen(row.root_fen);
        if (!row.history.empty()) ++history_rows;
        for (const auto& token : row.history) {
            INFO("history-token=" << token << " prefix=" << replay.fen());
            const auto legal_before = uci_set(replay.legal_moves(replay.turn));
            REQUIRE(legal_before.contains(token));
            replay = replay.after(replay.legal_move_from_uci(token));
            ++replayed_tokens;
        }
        REQUIRE(replay.fen() == row.fen);
        REQUIRE(replay.board == imported.board);
        REQUIRE(replay.turn == imported.turn);
        REQUIRE(replay.castling == imported.castling);
        REQUIRE(replay.en_passant == imported.en_passant);
        REQUIRE(replay.halfmove == imported.halfmove);
        REQUIRE(replay.fullmove == imported.fullmove);
        REQUIRE(replay.in_check(replay.turn) == row.in_check);
        require_equal_sets(uci_set(replay.legal_moves(replay.turn)),
                           expected_legal, "oracle-only replayed legal set");
    }

    REQUIRE(rows.size() == 27);
    REQUIRE(history_rows == 11);
    REQUIRE(replayed_tokens > 0);
}

TEST_CASE("T-ORACLE-003 Committed history and fixed-seed random-walk differential rows",
          "[movegen][T-ORACLE-003][oracle][history][regression]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto& squares = square_bijection();
    const std::set<std::string> selected{
        "threefold_prospective", "threefold_current", "fivefold_repetition",
        "random_000000", "random_000001", "random_000002", "random_000003",
        "random_000004", "random_000005", "random_000006", "random_000007"};
    std::set<std::string> consumed;
    for (const auto& row : oracle_cases(fixture)) {
        if (!selected.contains(row.case_id)) continue;
        consumed.insert(row.case_id);
        INFO("case_id=" << row.case_id << " history-size=" << row.history.size());
        Position replay_position(row.root_fen);
        RefPosition reference = RefPosition::from_fen(row.root_fen);
        for (const auto& token : row.history) {
            Position generation_position(reference.fen());
            MoveList legal;
            generate_legal(generation_position, reference.turn, legal);
            const auto found = find_observed_move(legal, squares, token);
            INFO("history token=" << token << " position=" << reference.fen());
            REQUIRE(found.has_value());
            replay_position.MakeMove(found->raw);
            reference = reference.after(reference.legal_move_from_uci(token));
        }
        const RefPosition frozen = RefPosition::from_fen(row.fen);
        std::string difference;
        REQUIRE(reference.board == frozen.board);
        REQUIRE(reference.turn == frozen.turn);
        REQUIRE(placement_equals(replay_position, frozen, squares, &difference));
        REQUIRE(production_in_check(replay_position, frozen.turn) == row.in_check);
        Position terminal_position(row.fen);
        MoveList legal;
        generate_legal(terminal_position, frozen.turn, legal);
        const std::set<std::string> expected(row.legal_moves.begin(), row.legal_moves.end());
        require_equal_sets(observed_uci_set(legal, squares), expected, row.case_id);
    }
    REQUIRE(consumed == selected);
}

namespace {

template<AttackDirection Direction>
std::set<std::string> sliding_actual(const Sq source, const BitBoard us,
                                     const BitBoard them, const SquareBijection& squares,
                                     const bool expected_nonempty) {
    const move_info* info = MoveGen::GetMovesForSliding<Direction>(source, us, them);
    if (info == nullptr) {
        if (expected_nonempty)
            throw std::runtime_error("nonempty sliding result was represented by null");
        return {};
    }
    if (info->count_ > info->encoded_move_.size())
        throw std::runtime_error("move_info count exceeds storage");
    std::set<std::string> result;
    for (std::size_t index = 0; index < info->count_; ++index) {
        const ObservedMove decoded = observe_move(info->encoded_move_[index], squares);
        if (decoded.from != source)
            throw std::runtime_error("sliding move_info changed the logical source square");
        const BitBoard target = BitBoard{1} << static_cast<unsigned>(decoded.to);
        const MoveType expected_type = (them & target) != 0 ? mt_Capture : mt_Quiet;
        if (decoded.type != expected_type)
            throw std::runtime_error("sliding move_info used the wrong logical move kind");
        result.insert(squares.label(decoded.to));
    }
    return result;
}

std::vector<int> line_squares(const int origin, const int df, const int dr) {
    std::vector<int> result;
    for (const int sign : {-1, 1}) {
        for (int file = file_of(origin) + sign * df, rank = rank_of(origin) + sign * dr;
             on_board(file, rank); file += sign * df, rank += sign * dr) {
            result.push_back(index_of(file, rank));
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::set<std::string> sliding_reference(const int origin, const int df, const int dr,
                                        const std::set<int>& friendly,
                                        const std::set<int>& enemy) {
    std::set<std::string> result;
    for (const int sign : {-1, 1}) {
        for (int file = file_of(origin) + sign * df, rank = rank_of(origin) + sign * dr;
             on_board(file, rank); file += sign * df, rank += sign * dr) {
            const int target = index_of(file, rank);
            if (friendly.contains(target)) break;
            result.insert(square_name(target));
            if (enemy.contains(target)) break;
        }
    }
    return result;
}

template<AttackDirection Direction>
void exercise_sliding_direction(const int df, const int dr, const std::string_view name) {
    const auto& squares = square_bijection();
    std::uint64_t cases = 0;
    for (int origin = 0; origin < 64; ++origin) {
        const auto source = squares.square(square_name(origin));
        const auto line = line_squares(origin, df, dr);
        std::uint64_t assignments = 1;
        for (std::size_t i = 0; i < line.size(); ++i) assignments *= 3U;
        for (std::uint64_t code = 0; code < assignments; ++code) {
            std::uint64_t digits = code;
            std::set<int> friendly{origin};
            std::set<int> enemy;
            BitBoard us = BitBoard{1} << static_cast<unsigned>(source);
            BitBoard them = 0;
            for (const int square : line) {
                const unsigned state = static_cast<unsigned>(digits % 3U);
                digits /= 3U;
                const auto production_square = squares.square(square_name(square));
                if (state == 1U) {
                    friendly.insert(square);
                    us |= BitBoard{1} << static_cast<unsigned>(production_square);
                } else if (state == 2U) {
                    enemy.insert(square);
                    them |= BitBoard{1} << static_cast<unsigned>(production_square);
                }
            }
            INFO("direction=" << name << " origin=" << square_name(origin)
                 << " ternary-code=" << code);
            const auto expected = sliding_reference(origin, df, dr, friendly, enemy);
            require_equal_sets(sliding_actual<Direction>(source, us, them, squares,
                                                         !expected.empty()),
                               expected,
                               "ternary sliding line");
            const std::size_t blocker_count = friendly.size() + enemy.size() - 1;
            if (blocker_count <= 1) {
                const std::set<int> line_set(line.begin(), line.end());
                for (int off_line = 0; off_line < 64; ++off_line) {
                    if (off_line == origin || line_set.contains(off_line)) continue;
                    const Sq production_off_line = squares.square(square_name(off_line));
                    const BitBoard bit = BitBoard{1} << static_cast<unsigned>(production_off_line);
                    require_equal_sets(
                        sliding_actual<Direction>(source, us | bit, them, squares,
                                                  !expected.empty()),
                        expected, "friendly off-line locality toggle");
                    require_equal_sets(
                        sliding_actual<Direction>(source, us, them | bit, squares,
                                                  !expected.empty()),
                        expected, "enemy off-line locality toggle");
                }
            }
            ++cases;
        }
    }
    INFO("direction=" << name << " cases=" << cases);
    if constexpr (Direction == Rank || Direction == File) REQUIRE(cases == 139968);
    else REQUIRE(cases == 31712);
}

std::set<std::string> observed_source_target_set(MoveList& list,
                                                 const SquareBijection& squares) {
    std::set<std::string> result;
    for (const auto& move : observe_moves(list, squares)) {
        result.insert(squares.label(move.from) + squares.label(move.to));
    }
    return result;
}

std::set<std::string> reference_source_target_set(const std::vector<LogicalMove>& moves) {
    std::set<std::string> result;
    for (const auto& move : moves) result.insert(square_name(move.from) + square_name(move.to));
    return result;
}

RefPosition king_shell_avoiding(const RefPosition& pieces, const std::set<int>& forbidden) {
    for (int white = 0; white < 64; ++white) {
        if (forbidden.contains(white) || pieces.occupied(white)) continue;
        for (int black = 0; black < 64; ++black) {
            if (black == white || forbidden.contains(black) || pieces.occupied(black)) continue;
            RefPosition candidate = pieces;
            candidate.board[static_cast<std::size_t>(white)] = 'K';
            candidate.board[static_cast<std::size_t>(black)] = 'k';
            if (!candidate.in_check(Side::white) && !candidate.in_check(Side::black)) return candidate;
        }
    }
    throw std::runtime_error("no legal king shell outside forbidden targets");
}

std::set<int> king_attackers(const RefPosition& position, const Side side) {
    const int king = position.king_square(side);
    std::set<int> result;
    for (int source = 0; source < 64; ++source) {
        if (!position.occupied_by(source, opposite(side))) continue;
        if (position.attacks_from(source).contains(king)) result.insert(source);
    }
    return result;
}

bool removal_exposes_king(const RefPosition& position, const Side side, const int source) {
    if (kind_of(position.board[static_cast<std::size_t>(source)]) == 'k') return false;
    const auto before = king_attackers(position, side);
    RefPosition removed = position;
    removed.board[static_cast<std::size_t>(source)] = '.';
    const auto after = king_attackers(removed, side);
    for (const int attacker : after) {
        if (!before.contains(attacker)) return true;
    }
    return false;
}

struct AttackConsensus {
    std::set<std::string> expected_true;
    std::set<std::string> assertion_mask;
};

AttackConsensus attack_consensus(const RefPosition& position, const Side side,
                                 const std::optional<char> only_kind = std::nullopt) {
    std::set<int> permissive_union;
    std::set<int> restrictive_union;
    std::set<int> legal_king_targets;
    const int king = position.king_square(side);
    for (const auto& move : position.legal_moves(side)) {
        if (move.from == king) legal_king_targets.insert(move.to);
    }

    for (int source = 0; source < 64; ++source) {
        if (!position.occupied_by(source, side)) continue;
        const char kind = kind_of(position.board[static_cast<std::size_t>(source)]);
        if (only_kind && kind != *only_kind) continue;
        const auto attacks = position.attacks_from(source);
        permissive_union.insert(attacks.begin(), attacks.end());
        if (kind != 'k' && removal_exposes_king(position, side, source)) continue;
        for (const int target : attacks) {
            if (position.occupied_by(target, side)) continue;
            if (kind == 'k' && !legal_king_targets.contains(target)) continue;
            restrictive_union.insert(target);
        }
    }

    AttackConsensus result;
    for (const int target : restrictive_union)
        result.expected_true.insert(square_name(target));
    for (int target = 0; target < 64; ++target) {
        const bool disputed = permissive_union.contains(target) &&
                              !restrictive_union.contains(target);
        if (!disputed) result.assertion_mask.insert(square_name(target));
    }
    return result;
}

std::set<std::string> intersection(std::set<std::string> values,
                                   const std::set<std::string>& mask) {
    for (auto iterator = values.begin(); iterator != values.end();) {
        if (!mask.contains(*iterator)) iterator = values.erase(iterator);
        else ++iterator;
    }
    return values;
}

} // namespace

TEST_CASE("T-STATE-001 Move and attack queries preserve complete FEN-semantic state",
          "[movegen][T-STATE-001][state][exhaustive]") {
    const auto cases_fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto perft_fixture = load_tsv_fixture("tests/data/oracle_perft.tsv");
    const auto oracle_rows = oracle_cases(cases_fixture);
    const auto perft_rows = oracle_perft_rows(perft_fixture);

    std::vector<NamedFixture> fixtures;
    for (const auto& row : oracle_rows) {
        fixtures.push_back({"oracle/" + row.case_id,
                            RefPosition::from_fen(row.fen)});
    }
    std::set<std::string> perft_ids;
    for (const auto& row : perft_rows) {
        if (perft_ids.insert(row.case_id).second) {
            fixtures.push_back({"perft/" + row.case_id,
                                RefPosition::from_fen(row.fen)});
        }
    }
    const auto rule_fixtures = exact_rule_fixtures(oracle_rows);
    require_exact_rule_fixture_scope(rule_fixtures);
    for (const auto& fixture : rule_fixtures) {
        fixtures.push_back({fixture.id, fixture.reference});
        fixtures.push_back({fixture.id + "/rank-flip-colour-swap",
                            rank_flip_colour_swap(fixture.reference)});
    }

    REQUIRE(oracle_rows.size() == 27);
    REQUIRE(perft_ids.size() == 6);
    std::size_t generation_calls = 0;
    std::size_t unavailable_generations = 0;
    std::size_t attack_calls = 0;
    std::set<Side> exercised_sides;

    for (const auto& fixture : fixtures) {
        const RefPosition& reference = fixture.reference;
        const std::string fen = reference.fen();
        exercised_sides.insert(reference.turn);
        INFO("fixture=" << fixture.id << " fen=" << fen);
        std::map<std::string, std::set<std::string>> generation_baselines;

        const auto exercise_generator = [&](const std::string_view name,
                                            auto&& expected_factory,
                                            auto&& invoke) {
            std::optional<std::set<std::string>> first_result;
            for (int repetition = 0; repetition < 2; ++repetition) {
                INFO("repetition=" << repetition);
                const std::set<std::string> expected = expected_factory();
                const auto checked = checked_generator_call(
                    fen, expected, name, invoke);
                if (!checked) {
                    ++unavailable_generations;
                    continue;
                }
                ++generation_calls;
                if (first_result) REQUIRE(checked->logical_set == *first_result);
                else first_result = checked->logical_set;
                const auto [baseline, inserted] = generation_baselines.emplace(
                    std::string(name), checked->logical_set);
                if (!inserted)
                    REQUIRE(checked->logical_set == baseline->second);
            }
        };

        const auto exercise_all_generators = [&] {
          for (const char kind : std::string_view{"pbrqnk"}) {
              const std::string name = std::string("piece-") + kind;
              exercise_generator(
                  name,
                  [&] { return uci_set(reference.pseudo_moves(
                      reference.turn, kind, false)); },
                  [&](Position& position, MoveList& list) {
                      generate_piece(position, reference.turn, kind, list);
                  });
          }
            exercise_generator(
                "castling",
                [&] { return expected_castling_moves(reference); },
                [&](Position& position, MoveList& list) {
                    generate_castling(position, reference.turn, list);
                });
            exercise_generator(
                "pseudo-legal",
                [&] { return uci_set(reference.pseudo_moves(reference.turn)); },
                [&](Position& position, MoveList& list) {
                    generate_pseudo(position, reference.turn, list);
                });
            exercise_generator(
                "legal",
                [&] { return uci_set(reference.legal_moves(reference.turn)); },
                [&](Position& position, MoveList& list) {
                    generate_legal(position, reference.turn, list);
                });
        };

        exercise_all_generators();

        Position attack_position(fen);
        for (const char kind : std::string_view{"pbrqnk"}) {
            std::optional<std::set<std::string>> first_result;
            for (int repetition = 0; repetition < 2; ++repetition) {
                INFO("attack-kind=" << kind << " repetition=" << repetition);
                const AttackConsensus consensus =
                    attack_consensus(reference, reference.turn, kind);
                const auto actual = checked_position_query(
                    attack_position, "piece attack",
                    [&](Position& position) {
                        return square_bijection().labels(generate_piece_attacks(
                            position, reference.turn, kind));
                    });
                const auto projection = intersection(actual,
                                                     consensus.assertion_mask);
                require_equal_sets(projection, consensus.expected_true,
                                   "state-test piece attack consensus");
                if (first_result) REQUIRE(projection == *first_result);
                else first_result = projection;
                ++attack_calls;
            }
        }

        std::optional<std::set<std::string>> first_aggregate;
        for (int repetition = 0; repetition < 2; ++repetition) {
            const AttackConsensus consensus =
                attack_consensus(reference, reference.turn);
            const auto actual = checked_position_query(
                attack_position, "aggregate attack",
                [&](Position& position) {
                    return square_bijection().labels(
                        generate_all_attacks(position, reference.turn));
                });
            const auto projection = intersection(actual, consensus.assertion_mask);
            require_equal_sets(projection, consensus.expected_true,
                               "state-test aggregate attack consensus");
            if (first_aggregate) REQUIRE(projection == *first_aggregate);
            else first_aggregate = projection;
            ++attack_calls;
        }

        std::optional<bool> first_check;
        for (int repetition = 0; repetition < 2; ++repetition) {
            const bool expected_check = reference.in_check(reference.turn);
            const bool actual_check = checked_position_query(
                attack_position, "in-check",
                [&](Position& position) {
                    return production_in_check(position, reference.turn);
                });
            REQUIRE(actual_check == expected_check);
            if (first_check) REQUIRE(actual_check == *first_check);
            else first_check = actual_check;
            ++attack_calls;
        }

        // The second fresh-instance pass brackets the attack-query sequence and
        // detects order/cache interactions without sharing mutable Position state.
        exercise_all_generators();
    }

    REQUIRE(exercised_sides == std::set<Side>{Side::white, Side::black});
    if (unavailable_generations != 0) {
        SKIP("T-STATE-001 cannot prove its complete generator state domain "
             "because an expected set exceeds observed MoveList capacity");
    }
    REQUIRE(generation_calls > 0);
    REQUIRE(attack_calls > 0);
    INFO("unavailable-generation-calls=" << unavailable_generations);
}

TEST_CASE("T-GEO-003 Exhaustive ternary blocker geometry for every sliding line",
          "[movegen][T-GEO-003][geometry][exhaustive]") {
    exercise_sliding_direction<File>(0, 1, "File");
    exercise_sliding_direction<Rank>(1, 0, "Rank");
    exercise_sliding_direction<Diagonal>(1, 1, "Diagonal");
    exercise_sliding_direction<AntiDiagonal>(1, -1, "AntiDiagonal");
}

TEST_CASE("T-GEN-001 Piece-specific pseudo-move integration against mailbox geometry",
          "[movegen][T-GEN-001][generator][differential]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto& squares = square_bijection();
    for (const auto& row : oracle_cases(fixture)) {
        const RefPosition reference = RefPosition::from_fen(row.fen);
        for (const char kind : std::string_view{"brqn"}) {
            INFO("case_id=" << row.case_id << " kind=" << kind << " fen=" << row.fen);
            const auto expected = uci_set(reference.pseudo_moves(reference.turn, kind, false));
            Position position(row.fen);
            MoveList list;
            const std::size_t capacity = list.all().size();
            if (expected.size() > capacity) {
                INFO("component unavailable: expected=" << expected.size()
                     << " observable-capacity=" << capacity);
                continue;
            }
            const FenSemanticSnapshot before = fen_semantic_snapshot(position);
            generate_piece(position, reference.turn, kind, list);
            const auto actual = observed_uci_set(list, squares);
            require_equal_sets(actual, expected, "piece-specific pseudo moves");
            REQUIRE(fen_semantic_snapshot(position) == before);
            if (kind == 'q') {
                std::set<int> friendly;
                std::set<int> enemy;
                for (int square = 0; square < 64; ++square) {
                    if (reference.occupied_by(square, reference.turn)) friendly.insert(square);
                    else if (reference.occupied_by(square, opposite(reference.turn))) enemy.insert(square);
                }
                std::set<std::string> decomposed;
                constexpr std::array<std::pair<int, int>, 4> directions{
                    std::pair{1, 0}, std::pair{0, 1},
                    std::pair{1, 1}, std::pair{1, -1},
                };
                for (int source = 0; source < 64; ++source) {
                    if (reference.board[static_cast<std::size_t>(source)] !=
                        piece_for(reference.turn, 'q')) continue;
                    for (const auto [df, dr] : directions) {
                        for (const auto& target :
                             sliding_reference(source, df, dr, friendly, enemy)) {
                            decomposed.insert(square_name(source) + target);
                        }
                    }
                }
                require_equal_sets(actual, decomposed,
                                   "queen output equals mailbox bishop/rook ray union");
            }
        }
    }
}

TEST_CASE("T-GEN-002 White and black pawn pseudo-movement boundaries",
          "[movegen][T-GEN-002][generator][pawn][exhaustive]") {
    const auto& squares = square_bijection();
    std::uint64_t exercised = 0;
    for (const Side side : {Side::white, Side::black}) {
        const int step = side == Side::white ? 1 : -1;
        const int home = side == Side::white ? 1 : 6;
        for (int rank = 1; rank <= 6; ++rank) {
            for (int file = 0; file < 8; ++file) {
                const int origin = index_of(file, rank);
                std::vector<int> relevant;
                if (on_board(file, rank + step)) relevant.push_back(index_of(file, rank + step));
                if (rank == home) relevant.push_back(index_of(file, rank + 2 * step));
                if (on_board(file - 1, rank + step)) relevant.push_back(index_of(file - 1, rank + step));
                if (on_board(file + 1, rank + step)) relevant.push_back(index_of(file + 1, rank + step));
                std::sort(relevant.begin(), relevant.end());
                relevant.erase(std::unique(relevant.begin(), relevant.end()), relevant.end());
                std::uint64_t assignments = 1;
                for (std::size_t i = 0; i < relevant.size(); ++i) assignments *= 3U;
                for (std::uint64_t code = 0; code < assignments; ++code) {
                    RefPosition pieces;
                    pieces.turn = side;
                    pieces.castling = "-";
                    pieces.board[static_cast<std::size_t>(origin)] = piece_for(side, 'p');
                    std::uint64_t digits = code;
                    for (const int target : relevant) {
                        const unsigned state = static_cast<unsigned>(digits % 3U);
                        digits /= 3U;
                        if (state == 1U) pieces.board[static_cast<std::size_t>(target)] = piece_for(side, 'n');
                        if (state == 2U) pieces.board[static_cast<std::size_t>(target)] = piece_for(opposite(side), 'n');
                    }
                    const auto shell = add_legal_king_shell(pieces);
                    INFO("side=" << (side == Side::white ? "white" : "black")
                         << " origin=" << square_name(origin) << " occupancy-code=" << code);
                    REQUIRE(shell.has_value());
                    const std::string fen = shell->fen();
                    Position position(fen);
                    MoveList list;
                    generate_piece(position, side, 'p', list);
                    require_equal_sets(observed_source_target_set(list, squares),
                                       reference_source_target_set(shell->pseudo_moves(side, 'p', false)),
                                       "pawn destination projection");
                    ++exercised;
                }
            }
        }
    }
    REQUIRE(exercised > 0);
}

TEST_CASE("T-GEN-003 Composite pseudo-legal set equals the union of rule components",
          "[movegen][T-GEN-003][generator][pseudo-legal]") {
    const auto cases_fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto perft_fixture = load_tsv_fixture("tests/data/oracle_perft.tsv");
    std::set<std::string> fens;
    for (const auto& row : oracle_cases(cases_fixture)) fens.insert(row.fen);
    for (const auto& row : oracle_perft_rows(perft_fixture)) fens.insert(row.fen);
    for (const auto& fen : legal_fixture_fens()) fens.insert(fen);
    for (const auto& fen : std::vector<std::string>{
             "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
             "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
             "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
             "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1"}) fens.insert(fen);
    const auto& squares = square_bijection();
    for (const auto& fen : fens) {
        const RefPosition reference = RefPosition::from_fen(fen);
        INFO("fen=" << fen);
        const auto expected_composite = uci_set(reference.pseudo_moves(reference.turn));
        Position composite_position(fen);
        MoveList composite_list;
        const std::size_t composite_capacity = composite_list.all().size();
        if (expected_composite.size() > composite_capacity) {
            INFO("composite unavailable: expected=" << expected_composite.size()
                 << " observable-capacity=" << composite_capacity);
            continue;
        }
        const FenSemanticSnapshot composite_before = fen_semantic_snapshot(composite_position);
        generate_pseudo(composite_position, reference.turn, composite_list);
        const auto actual_composite = observed_uci_set(composite_list, squares);
        require_equal_sets(actual_composite, expected_composite, "composite pseudo set");
        REQUIRE(fen_semantic_snapshot(composite_position) == composite_before);

        std::set<std::string> component_union;
        bool all_components_available = true;
        for (const char kind : std::string_view{"pnbrqk"}) {
            const auto expected = uci_set(reference.pseudo_moves(reference.turn, kind, false));
            Position component_position(fen);
            MoveList component_list;
            const std::size_t capacity = component_list.all().size();
            if (expected.size() > capacity) {
                INFO("component unavailable: kind=" << kind << " expected=" << expected.size()
                     << " observable-capacity=" << capacity);
                all_components_available = false;
                break;
            }
            const FenSemanticSnapshot before = fen_semantic_snapshot(component_position);
            generate_piece(component_position, reference.turn, kind, component_list);
            const auto actual = observed_uci_set(component_list, squares);
            require_equal_sets(actual, expected, "direct pseudo component");
            REQUIRE(fen_semantic_snapshot(component_position) == before);
            component_union.insert(actual.begin(), actual.end());
        }

        if (all_components_available) {
            const auto with_castles = uci_set(reference.pseudo_moves(reference.turn, 'k', true));
            const auto without_castles = uci_set(reference.pseudo_moves(reference.turn, 'k', false));
            std::set<std::string> expected_castles;
            for (const auto& move : with_castles)
                if (!without_castles.contains(move)) expected_castles.insert(move);
            Position castle_position(fen);
            MoveList castle_list;
            if (expected_castles.size() <= castle_list.all().size()) {
                const FenSemanticSnapshot before = fen_semantic_snapshot(castle_position);
                generate_castling(castle_position, reference.turn, castle_list);
                const auto actual_castles = observed_uci_set(castle_list, squares);
                require_equal_sets(actual_castles, expected_castles, "direct castling component");
                REQUIRE(fen_semantic_snapshot(castle_position) == before);
                component_union.insert(actual_castles.begin(), actual_castles.end());
                require_equal_sets(actual_composite, component_union,
                                   "composite equals union of separately callable components");
            }
        }
    }
}

TEST_CASE("T-ATK-001 Exhaustive valid-rank pawn attack geometry for both colours",
          "[movegen][T-ATK-001][attack][pawn][exhaustive]") {
    const auto& squares = square_bijection();
    struct Observation {
        std::set<std::string> raw;
        std::set<std::string> projected;
        AttackConsensus consensus;
    };
    const auto observe = [&](const RefPosition& reference) {
        const std::string fen = reference.fen();
        Position position(fen);
        const AttackConsensus consensus =
            attack_consensus(reference, reference.turn, 'p');
        const auto raw = checked_position_query(
            position, "paired pawn attack",
            [&](Position& query_position) {
                return squares.labels(generate_piece_attacks(
                    query_position, reference.turn, 'p'));
            });
        const auto projected = intersection(raw, consensus.assertion_mask);
        require_equal_sets(projected, consensus.expected_true,
                           "pawn consensus attack diagonals");
        REQUIRE(consensus.assertion_mask.size() == 64);
        return Observation{raw, projected, consensus};
    };

    std::size_t origin_colour_cases = 0;
    std::size_t query_calls = 0;
    for (int rank = 1; rank <= 6; ++rank) {
        for (int file = 0; file < 8; ++file) {
            const int origin = index_of(file, rank);
            std::set<int> diagonals;
            for (const int df : {-1, 1}) {
                if (on_board(file + df, rank + 1))
                    diagonals.insert(index_of(file + df, rank + 1));
            }
            std::vector<int> enemy_targets{-1};
            enemy_targets.insert(enemy_targets.end(), diagonals.begin(), diagonals.end());
            origin_colour_cases += 2;

            for (const int enemy_target : enemy_targets) {
                RefPosition pieces;
                pieces.turn = Side::white;
                pieces.board[static_cast<std::size_t>(origin)] = 'P';
                if (enemy_target >= 0)
                    pieces.board[static_cast<std::size_t>(enemy_target)] = 'n';
                const RefPosition white = king_shell_avoiding(pieces, diagonals);
                const RefPosition black = rank_flip_colour_swap(white);
                INFO("white-fen=" << white.fen() << " black-fen=" << black.fen()
                     << " origin=" << square_name(origin));

                const Observation white_observation = observe(white);
                const Observation black_observation = observe(black);
                query_calls += 2;

                require_equal_sets(
                    rank_flip_labels(black_observation.consensus.assertion_mask),
                    white_observation.consensus.assertion_mask,
                    "pawn mirrored consensus mask");
                require_equal_sets(
                    rank_flip_labels(black_observation.consensus.expected_true),
                    white_observation.consensus.expected_true,
                    "pawn mirrored consensus truth");
                require_equal_sets(
                    rank_flip_labels(black_observation.projected),
                    white_observation.projected,
                    "pawn rank-flip/colour-swap projection");
                require_equal_sets(rank_flip_labels(black_observation.raw),
                                   white_observation.raw,
                                   "pawn undisputed raw rank symmetry");
            }
        }
    }
    REQUIRE(origin_colour_cases == 96);
    REQUIRE(query_calls == 264);
}

TEST_CASE("T-ATK-002 Knight king and sliding attack integration",
          "[movegen][T-ATK-002][attack][differential]") {
    struct AttackRow {
        const char* fen;
        char kind;
        const char* origin;
    };
    const std::array<AttackRow, 10> rows{
        AttackRow{"8/7k/8/1R3r2/3N4/8/8/K7 w - - 0 1", 'n', "d4"},
        AttackRow{"7k/8/8/2N1n3/3K4/8/8/8 w - - 0 1", 'k', "d4"},
        AttackRow{"8/8/1n3N1k/8/3B4/8/1r3R2/K7 w - - 0 1", 'b', "d4"},
        AttackRow{"8/7k/3B4/8/1n1R1N2/8/3b4/K7 w - - 0 1", 'r', "d4"},
        AttackRow{"8/7k/3N1B2/8/3Q1b2/8/1r6/K7 w - - 0 1", 'q', "d4"},
        AttackRow{"k7/8/8/3n4/1r3R2/8/7K/8 b - - 0 1", 'n', "d5"},
        AttackRow{"8/8/8/3k4/2n1N3/8/8/7K b - - 0 1", 'k', "d5"},
        AttackRow{"k7/1R3r2/8/3b4/8/1N3n1K/8/8 b - - 0 1", 'b', "d5"},
        AttackRow{"k7/3B4/8/1N1r1n2/8/3b4/7K/8 b - - 0 1", 'r', "d5"},
        AttackRow{"k7/1R6/8/3q1B2/8/3n1b2/7K/8 b - - 0 1", 'q', "d5"},
    };
    const auto& squares = square_bijection();
    std::array<std::set<std::string>, 10> observed;
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        const auto& row = rows[row_index];
        const RefPosition reference = RefPosition::from_fen(row.fen);
        INFO("row_index=" << row_index << " fen=" << row.fen
             << " kind=" << row.kind << " origin=" << row.origin);
        REQUIRE(reference.board[static_cast<std::size_t>(square_index(row.origin))] ==
                piece_for(reference.turn, row.kind));
        unsigned white_kings = 0;
        unsigned black_kings = 0;
        unsigned pawns = 0;
        unsigned queried_pieces = 0;
        for (const char piece : reference.board) {
            white_kings += piece == 'K';
            black_kings += piece == 'k';
            pawns += kind_of(piece) == 'p';
            queried_pieces += piece == piece_for(reference.turn, row.kind);
        }
        REQUIRE(white_kings == 1);
        REQUIRE(black_kings == 1);
        REQUIRE(pawns == 0);
        REQUIRE(queried_pieces == 1);
        const int white_king = reference.king_square(Side::white);
        const int black_king = reference.king_square(Side::black);
        REQUIRE(std::max(std::abs(file_of(white_king) - file_of(black_king)),
                         std::abs(rank_of(white_king) - rank_of(black_king))) > 1);
        REQUIRE_FALSE(reference.in_check(Side::white));
        REQUIRE_FALSE(reference.in_check(Side::black));

        Position position(row.fen);
        const FenSemanticSnapshot before = fen_semantic_snapshot(position);
        const auto consensus = attack_consensus(reference, reference.turn, row.kind);
        observed[row_index] = squares.labels(
            generate_piece_attacks(position, reference.turn, row.kind));
        require_equal_sets(intersection(observed[row_index], consensus.assertion_mask),
                           consensus.expected_true,
                           "literal-fixture consensus attack projection");
        REQUIRE(fen_semantic_snapshot(position) == before);
    }
    for (std::size_t pair = 0; pair < 5; ++pair) {
        std::set<std::string> reflected;
        for (const auto& label : observed[pair]) {
            const int square = square_index(label);
            reflected.insert(square_name(index_of(file_of(square), 7 - rank_of(square))));
        }
        require_equal_sets(observed[pair + 5], reflected,
                           "rank-flip/colour-swap attack metamorphism");
    }
}

TEST_CASE("T-ATK-003 Aggregate attacks and check detection on valid positions",
          "[movegen][T-ATK-003][attack][check][differential]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto& squares = square_bijection();
    const auto exercise = [&](const std::string_view id,
                              const RefPosition& reference,
                              const bool expected_check) {
        const std::string fen = reference.fen();
        INFO("case_id=" << id << " fen=" << fen);
        REQUIRE(reference.in_check(reference.turn) == expected_check);
        Position position(fen);
        const FenSemanticSnapshot before = fen_semantic_snapshot(position);
        const Side attacking = reference.turn;
        const auto consensus = attack_consensus(reference, attacking);
        const auto aggregate = squares.labels(generate_all_attacks(position, attacking));
        require_equal_sets(intersection(aggregate, consensus.assertion_mask),
                           consensus.expected_true,
                           "aggregate consensus projection");
        REQUIRE(fen_semantic_snapshot(position) == before);

        std::set<std::string> component_union;
        for (const char kind : std::string_view{"pnbrqk"}) {
            const auto component = squares.labels(generate_piece_attacks(position, attacking, kind));
            component_union.insert(component.begin(), component.end());
        }
        require_equal_sets(intersection(aggregate, consensus.assertion_mask),
                           intersection(component_union, consensus.assertion_mask),
                           "aggregate equals masked component union");
        REQUIRE(production_in_check(position, reference.turn) == expected_check);
        REQUIRE(fen_semantic_snapshot(position) == before);
    };

    for (const auto& row : oracle_cases(fixture)) {
        exercise(row.case_id, RefPosition::from_fen(row.fen), row.in_check);
    }

    const std::array<std::pair<std::string_view, bool>, 7> exact_checks{
        std::pair{"4k3/8/8/8/8/8/4r3/4K3 w - - 0 1", true},
        std::pair{"4k3/8/8/8/8/4r3/4B3/4K3 w - - 0 1", false},
        std::pair{"4k3/8/8/8/8/5n2/8/4K3 w - - 0 1", true},
        std::pair{"4k3/8/8/8/1b6/8/8/4K3 w - - 0 1", true},
        std::pair{"4k3/8/8/8/8/8/3p4/4K3 w - - 0 1", true},
        std::pair{"4k3/8/8/8/8/8/4q3/4K3 w - - 0 1", true},
        // The black rook is absolutely pinned to e8 by the white rook, but
        // still attacks the active white king horizontally under FIDE 3.1.3.
        std::pair{"4k3/K3r3/8/8/8/8/8/4R3 w - - 0 1", true},
    };
    for (std::size_t index = 0; index < exact_checks.size(); ++index) {
        const auto& [fen, expected_check] = exact_checks[index];
        const RefPosition white = RefPosition::from_fen(fen);
        exercise("exact-check-white-" + std::to_string(index), white, expected_check);
        exercise("exact-check-black-" + std::to_string(index),
                 rank_flip_colour_swap(white), expected_check);
    }
}

namespace {

std::set<std::string> castle_uci_set(const RefPosition& reference) {
    std::set<std::string> result;
    for (const auto& move : reference.legal_moves(reference.turn)) {
        if (kind_of(reference.board[static_cast<std::size_t>(move.from)]) == 'k' &&
            std::abs(file_of(move.to) - file_of(move.from)) == 2) {
            result.insert(move.uci());
        }
    }
    return result;
}

void exercise_special_transition(const std::string& fen, const std::string& uci) {
    const auto& squares = square_bijection();
    const RefPosition reference = RefPosition::from_fen(fen);
    Position generation_position(fen);
    MoveList legal;
    generate_legal(generation_position, reference.turn, legal);
    const auto observed = find_observed_move(legal, squares, uci);
    INFO("fen=" << fen << " move=" << uci);
    REQUIRE(observed.has_value());
    Position transition_position(fen);
    const auto before = public_placement(transition_position);
    const RefPosition child = reference.after(reference.legal_move_from_uci(uci));
    transition_position.MakeMove(observed->raw);
    std::string difference;
    const bool child_matches = placement_equals(transition_position, child, squares, &difference);
    INFO("child placement difference=" << difference);
    REQUIRE(child_matches);
    transition_position.UnmakeMove(observed->raw);
    REQUIRE(public_placement(transition_position) == before);
}

void compare_castling_fixture(const RefPosition& reference) {
    const auto& squares = square_bijection();
    const std::string fen = reference.fen();
    Position direct_position(fen);
    Position legal_position(fen);
    MoveList direct;
    MoveList legal;
    generate_castling(direct_position, reference.turn, direct);
    generate_legal(legal_position, reference.turn, legal);
    const auto expected = castle_uci_set(reference);
    std::set<std::string> legal_castles;
    for (const auto& move : observe_moves(legal, squares)) {
        if (move.type == mt_Castling) legal_castles.insert(move.uci);
    }
    std::set<std::string> direct_castles;
    for (const auto& move : observe_moves(direct, squares)) {
        INFO("direct_castling_move=" << move.uci
             << " type=" << move_type_name(move.type));
        REQUIRE(move.type == mt_Castling);
        REQUIRE(move.from != move.to);
        direct_castles.insert(move.uci);
    }
    INFO("fen=" << fen);
    require_equal_sets(direct_castles, expected, "direct Castling output");
    require_equal_sets(legal_castles, expected, "legal castling subset");
}

bool canonical_uci_token(const std::string_view token) {
    if (token.size() != 4 && token.size() != 5) return false;
    try {
        (void)square_index(token.substr(0, 2));
        (void)square_index(token.substr(2, 2));
    } catch (...) {
        return false;
    }
    return token.size() == 4 || token[4] == 'q' || token[4] == 'r' ||
           token[4] == 'b' || token[4] == 'n';
}

} // namespace

TEST_CASE("T-RULE-001 Ordinary legal en passant generation and capture",
          "[movegen][T-RULE-001][rule][en-passant]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto& squares = square_bijection();
    auto rows = oracle_cases(fixture);
    const auto found = std::find_if(rows.begin(), rows.end(), [](const OracleCase& row) {
        return row.case_id == "en_passant_legal";
    });
    REQUIRE(found != rows.end());
    for (const RefPosition reference : {RefPosition::from_fen(found->fen),
                                        rank_flip_colour_swap(RefPosition::from_fen(found->fen))}) {
        const std::string fen = reference.fen();
        Position pseudo_position(fen);
        Position legal_position(fen);
        MoveList pseudo;
        MoveList legal;
        generate_piece(pseudo_position, reference.turn, 'p', pseudo);
        generate_legal(legal_position, reference.turn, legal);
        require_equal_sets(observed_uci_set(pseudo, squares),
                           uci_set(reference.pseudo_moves(reference.turn, 'p', false)), "EP pawn pseudo set");
        require_equal_sets(observed_uci_set(legal, squares),
                           uci_set(reference.legal_moves(reference.turn)), "EP legal set");
        std::vector<std::string> ep_moves;
        for (const auto& move : reference.legal_moves(reference.turn)) {
            if (kind_of(reference.board[static_cast<std::size_t>(move.from)]) == 'p' &&
                reference.en_passant && move.to == *reference.en_passant &&
                !reference.occupied(move.to) && file_of(move.from) != file_of(move.to)) {
                ep_moves.push_back(move.uci());
            }
        }
        REQUIRE(ep_moves.size() == 1);
        const auto observed = find_observed_move(legal, squares, ep_moves.front());
        REQUIRE(observed.has_value());
        REQUIRE(observed->type == mt_EnPassant);
        exercise_special_transition(fen, ep_moves.front());
    }
}

TEST_CASE("T-RULE-002 En passant king-safety edge cases",
          "[movegen][T-RULE-002][rule][en-passant][king-safety]") {
    const auto& squares = square_bijection();
    const std::vector<std::string> fens{
        "k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
        "k7/8/8/3pP3/4K3/8/8/8 w - d6 0 2",
        "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 2"};
    for (const auto& white_fen : fens) {
        for (const RefPosition reference : {RefPosition::from_fen(white_fen),
                                            rank_flip_colour_swap(RefPosition::from_fen(white_fen))}) {
            const std::string fen = reference.fen();
            Position position(fen);
            MoveList legal;
            generate_legal(position, reference.turn, legal);
            require_equal_sets(observed_uci_set(legal, squares),
                               uci_set(reference.legal_moves(reference.turn)), fen);
        }
    }
    {
        Position pinned(fens[0]);
        MoveList legal;
        generate_legal(pinned, Side::white, legal);
        const auto set = observed_uci_set(legal, squares);
        REQUIRE_FALSE(set.contains("e5d6"));
        REQUIRE(set.contains("e5e6"));
    }
    {
        Position evasion(fens[1]);
        MoveList legal;
        generate_legal(evasion, Side::white, legal);
        REQUIRE(observed_uci_set(legal, squares).contains("e5d6"));
    }
    {
        Position horizontal(fens[2]);
        MoveList legal;
        generate_legal(horizontal, Side::white, legal);
        REQUIRE_FALSE(observed_uci_set(legal, squares).contains("g5f6"));
    }
}

TEST_CASE("T-RULE-003 Both castling wings for both colours",
          "[movegen][T-RULE-003][rule][castling]") {
    for (const Side side : {Side::white, Side::black}) {
        for (const std::string rights : side == Side::white
                 ? std::vector<std::string>{"K", "Q", "KQ"}
                 : std::vector<std::string>{"k", "q", "kq"}) {
            const std::string fen = std::string("r3k2r/8/8/8/8/8/8/R3K2R ") +
                (side == Side::white ? "w " : "b ") + rights + " - 0 1";
            const RefPosition reference = RefPosition::from_fen(fen);
            compare_castling_fixture(reference);
            for (const auto& castle : castle_uci_set(reference)) exercise_special_transition(fen, castle);
        }
    }
}

TEST_CASE("T-RULE-004 Standard castling denial conditions",
          "[movegen][T-RULE-004][rule][castling][negative]") {
    std::vector<RefPosition> fixtures;
    for (const std::string rights : {"-", "K", "Q"}) {
        fixtures.push_back(RefPosition::from_fen(
            "8/3k4/8/8/8/8/8/R3K2R w " + rights + " - 0 1"));
    }
    for (const std::string blocker : {"b1", "c1", "d1", "f1", "g1"}) {
        RefPosition position = RefPosition::from_fen("8/3k4/8/8/8/8/8/R3K2R w KQ - 0 1");
        position.board[static_cast<std::size_t>(square_index(blocker))] = 'N';
        fixtures.push_back(position);
    }
    for (const std::string attacker : {"e8", "f8", "g8", "d8", "c8", "b8"}) {
        RefPosition position = RefPosition::from_fen("8/3k4/8/8/8/8/8/R3K2R w KQ - 0 1");
        position.board[static_cast<std::size_t>(square_index(attacker))] = 'r';
        fixtures.push_back(position);
    }
    fixtures.push_back(RefPosition::from_fen("7r/3k4/8/8/8/8/8/R3K2R w KQ - 0 1"));
    fixtures.push_back(RefPosition::from_fen("r7/3k4/8/8/8/8/8/R3K2R w KQ - 0 1"));
    for (const auto& white : fixtures) {
        compare_castling_fixture(white);
        compare_castling_fixture(rank_flip_colour_swap(white));
    }
    REQUIRE(castle_uci_set(fixtures[fixtures.size() - 2]).contains("e1g1"));
    REQUIRE(castle_uci_set(fixtures.back()).contains("e1c1"));
}

TEST_CASE("T-RULE-006 Quiet and capture promotion choices for both colours",
          "[movegen][T-RULE-006][rule][promotion]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    std::set<std::string> fens{
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
        "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1",
        "4k3/8/8/8/8/8/p7/4K3 b - - 0 1",
        "4k3/8/8/8/8/8/p7/1R2K3 b - - 0 1"};
    for (const auto& row : oracle_cases(fixture)) {
        if (row.case_id == "promotion_choices") fens.insert(row.fen);
    }
    const auto& squares = square_bijection();
    for (const auto& fen : fens) {
        const RefPosition reference = RefPosition::from_fen(fen);
        Position pseudo_position(fen);
        Position legal_position(fen);
        MoveList pawn;
        MoveList legal;
        generate_piece(pseudo_position, reference.turn, 'p', pawn);
        generate_legal(legal_position, reference.turn, legal);
        require_equal_sets(observed_uci_set(pawn, squares),
                           uci_set(reference.pseudo_moves(reference.turn, 'p', false)), "promotion pseudo set");
        require_equal_sets(observed_uci_set(legal, squares),
                           uci_set(reference.legal_moves(reference.turn)), "promotion legal set");
        std::map<std::string, std::set<char>> choices;
        for (const auto& move : observe_moves(legal, squares)) {
            if (move.uci.size() != 5) continue;
            INFO("fen=" << fen << " promotion=" << move.uci << " type=" << move_type_name(move.type));
            REQUIRE(Moves::IsPromotionMove(move.raw));
            REQUIRE((move.type == mt_QueenPromotion || move.type == mt_RookPromotion ||
                     move.type == mt_BishopPromotion || move.type == mt_KnightPromotion));
            choices[move.uci.substr(0, 4)].insert(move.uci[4]);
            exercise_special_transition(fen, move.uci);
        }
        for (const auto& [destination, suffixes] : choices) {
            INFO("destination=" << destination);
            const std::set<char> expected_suffixes{'q','r','b','n'};
            REQUIRE(suffixes == expected_suffixes);
        }
        REQUIRE_FALSE(choices.empty());
    }
}

TEST_CASE("T-RULE-007 Checkmate and stalemate from check plus legal set",
          "[movegen][T-RULE-007][rule][terminal]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto& squares = square_bijection();
    const std::map<std::string, std::pair<bool, bool>> expected_check_and_empty{
        {"checkmate", {true, true}},
        {"stalemate", {false, true}},
        {"perft_promotions", {true, false}},
        {"start", {false, false}},
    };
    std::set<std::string> consumed;
    for (const auto& row : oracle_cases(fixture)) {
        const auto expected = expected_check_and_empty.find(row.case_id);
        if (expected == expected_check_and_empty.end()) continue;
        consumed.insert(row.case_id);
        Position query_position(row.fen);
        Position terminal_position(row.fen);
        const Side side = row.turn == 'w' ? Side::white : Side::black;
        MoveList legal;
        const bool check = production_in_check(query_position, side);
        generate_legal(terminal_position, side, legal);
        const bool empty = observed_uci_set(legal, squares).empty();
        INFO("case_id=" << row.case_id << " check=" << check << " empty=" << empty);
        REQUIRE(check == expected->second.first);
        REQUIRE(empty == expected->second.second);
        REQUIRE((check && empty) == (row.case_id == "checkmate"));
        REQUIRE((!check && empty) == (row.case_id == "stalemate"));
        if (row.case_id == "checkmate" || row.case_id == "stalemate") {
            const RefPosition mirrored = rank_flip_colour_swap(RefPosition::from_fen(row.fen));
            const std::string mirrored_fen = mirrored.fen();
            Position mirrored_query(mirrored_fen);
            Position mirrored_terminal(mirrored_fen);
            MoveList mirrored_legal;
            const bool mirrored_check = production_in_check(mirrored_query, mirrored.turn);
            generate_legal(mirrored_terminal, mirrored.turn, mirrored_legal);
            REQUIRE(mirrored_check == check);
            REQUIRE(observed_uci_set(mirrored_legal, squares).empty());
        }
    }
    std::set<std::string> selected;
    for (const auto& [id, ignored] : expected_check_and_empty) {
        (void)ignored;
        selected.insert(id);
    }
    REQUIRE(consumed == selected);
}

TEST_CASE("T-LEGAL-005 Colour and rank-flip metamorphism",
          "[movegen][T-LEGAL-005][legal][attack][metamorphic]") {
    const auto cases_fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const auto perft_fixture = load_tsv_fixture("tests/data/oracle_perft.tsv");
    const auto oracle_rows = oracle_cases(cases_fixture);
    const auto perft_rows = oracle_perft_rows(perft_fixture);

    std::vector<NamedFixture> fixtures;
    for (const auto& row : oracle_rows) {
        fixtures.push_back({"oracle/" + row.case_id,
                            RefPosition::from_fen(row.fen)});
    }
    const auto legal_fens = legal_fixture_fens();
    for (std::size_t index = 0; index < legal_fens.size(); ++index) {
        fixtures.push_back({"legal-fixture/" + std::to_string(index),
                            RefPosition::from_fen(legal_fens[index])});
    }
    std::set<std::string> perft_ids;
    for (const auto& row : perft_rows) {
        if (perft_ids.insert(row.case_id).second) {
            fixtures.push_back({"perft/" + row.case_id,
                                RefPosition::from_fen(row.fen)});
        }
    }
    const auto rule_fixtures = exact_rule_fixtures(oracle_rows);
    require_exact_rule_fixture_scope(rule_fixtures);
    fixtures.insert(fixtures.end(), rule_fixtures.begin(), rule_fixtures.end());

    REQUIRE(oracle_rows.size() == 27);
    REQUIRE(legal_fens.size() == 13);
    REQUIRE(perft_ids.size() == 6);
    REQUIRE(rule_fixtures.size() == 35);

    std::map<std::string, std::size_t> move_pairs;
    std::map<std::string, std::size_t> attack_pairs;
    std::size_t unavailable_move_pairs = 0;
    bool saw_promotion_pair = false;
    bool saw_en_passant_pair = false;
    bool saw_king_side_castle_pair = false;
    bool saw_queen_side_castle_pair = false;
    bool saw_checked_pair = false;
    bool saw_nonchecked_pair = false;

    for (const auto& fixture : fixtures) {
        const RefPosition& original = fixture.reference;
        const RefPosition mirrored = rank_flip_colour_swap(original);
        INFO("fixture=" << fixture.id << " original=" << original.fen()
             << " mirrored=" << mirrored.fen());

        unsigned white_kings = 0;
        unsigned black_kings = 0;
        for (const char piece : original.board) {
            white_kings += piece == 'K';
            black_kings += piece == 'k';
        }
        REQUIRE(white_kings == 1);
        REQUIRE(black_kings == 1);
        const int white_king = original.king_square(Side::white);
        const int black_king = original.king_square(Side::black);
        REQUIRE(std::max(std::abs(file_of(white_king) - file_of(black_king)),
                         std::abs(rank_of(white_king) - rank_of(black_king))) > 1);
        REQUIRE_FALSE(original.in_check(opposite(original.turn)));
        REQUIRE(RefPosition::from_fen(original.fen()).fen() == original.fen());
        REQUIRE(rank_flip_colour_swap(mirrored).fen() == original.fen());
        REQUIRE(mirrored.turn == opposite(original.turn));

        const auto original_legal = original.legal_moves(original.turn);
        for (const auto& move : original_legal) {
            const char moving = original.board[static_cast<std::size_t>(move.from)];
            if (move.promotion != 0) saw_promotion_pair = true;
            if (kind_of(moving) == 'p' && original.en_passant &&
                move.to == *original.en_passant && !original.occupied(move.to) &&
                file_of(move.from) != file_of(move.to)) {
                saw_en_passant_pair = true;
            }
            if (kind_of(moving) == 'k' &&
                std::abs(file_of(move.to) - file_of(move.from)) == 2) {
                if (file_of(move.to) == 6) saw_king_side_castle_pair = true;
                if (file_of(move.to) == 2) saw_queen_side_castle_pair = true;
            }
        }
        if (original.in_check(original.turn)) saw_checked_pair = true;
        else saw_nonchecked_pair = true;

        const auto exercise_move_pair = [&](const std::string& name,
                                            auto&& expected_factory,
                                            auto&& invoke) {
            const std::set<std::string> expected_original =
                expected_factory(original);
            const std::set<std::string> expected_mirrored =
                expected_factory(mirrored);
            require_equal_sets(rank_flip_moves(expected_mirrored),
                               expected_original,
                               name + " oracle rank symmetry");

            MoveList original_probe;
            MoveList mirrored_probe;
            const std::size_t original_capacity = original_probe.all().size();
            const std::size_t mirrored_capacity = mirrored_probe.all().size();
            if (expected_original.size() > original_capacity ||
                expected_mirrored.size() > mirrored_capacity) {
                INFO("paired generator unavailable: name=" << name
                     << " original-expected=" << expected_original.size()
                     << " original-capacity=" << original_capacity
                     << " mirrored-expected=" << expected_mirrored.size()
                     << " mirrored-capacity=" << mirrored_capacity);
                ++unavailable_move_pairs;
                return;
            }

            const auto actual_original = checked_generator_call(
                original.fen(), expected_original, name + " original",
                [&](Position& position, MoveList& list) {
                    invoke(position, original.turn, list);
                });
            const auto actual_mirrored = checked_generator_call(
                mirrored.fen(), expected_mirrored, name + " mirrored",
                [&](Position& position, MoveList& list) {
                    invoke(position, mirrored.turn, list);
                });
            REQUIRE(actual_original.has_value());
            REQUIRE(actual_mirrored.has_value());
            require_equal_sets(rank_flip_moves(actual_mirrored->logical_set),
                               actual_original->logical_set,
                               name + " production rank symmetry");
            ++move_pairs[name];
        };

        for (const char kind : std::string_view{"pbrqnk"}) {
            const std::string name = std::string("piece-") + kind;
            exercise_move_pair(
                name,
                [&](const RefPosition& reference) {
                    return uci_set(reference.pseudo_moves(
                        reference.turn, kind, false));
                },
                [&](Position& position, const Side side, MoveList& list) {
                    generate_piece(position, side, kind, list);
                });
        }
        exercise_move_pair(
            "castling",
            [&](const RefPosition& reference) {
                return expected_castling_moves(reference);
            },
            [&](Position& position, const Side side, MoveList& list) {
                generate_castling(position, side, list);
            });
        exercise_move_pair(
            "pseudo-legal",
            [&](const RefPosition& reference) {
                return uci_set(reference.pseudo_moves(reference.turn));
            },
            [&](Position& position, const Side side, MoveList& list) {
                generate_pseudo(position, side, list);
            });
        exercise_move_pair(
            "legal",
            [&](const RefPosition& reference) {
                return uci_set(reference.legal_moves(reference.turn));
            },
            [&](Position& position, const Side side, MoveList& list) {
                generate_legal(position, side, list);
            });

        const auto exercise_attack_pair = [&](const std::string& name,
                                              const std::optional<char> kind) {
            const AttackConsensus original_consensus =
                attack_consensus(original, original.turn, kind);
            const AttackConsensus mirrored_consensus =
                attack_consensus(mirrored, mirrored.turn, kind);
            require_equal_sets(
                rank_flip_labels(mirrored_consensus.assertion_mask),
                original_consensus.assertion_mask,
                name + " consensus-mask rank symmetry");
            require_equal_sets(
                rank_flip_labels(mirrored_consensus.expected_true),
                original_consensus.expected_true,
                name + " consensus-truth rank symmetry");

            Position original_position(original.fen());
            Position mirrored_position(mirrored.fen());
            const auto query = [&](Position& position, const Side side) {
                if (kind) {
                    return square_bijection().labels(
                        generate_piece_attacks(position, side, *kind));
                }
                return square_bijection().labels(
                    generate_all_attacks(position, side));
            };
            const auto original_actual = checked_position_query(
                original_position, name + " original",
                [&](Position& position) { return query(position, original.turn); });
            const auto mirrored_actual = checked_position_query(
                mirrored_position, name + " mirrored",
                [&](Position& position) { return query(position, mirrored.turn); });
            const auto original_projection = intersection(
                original_actual, original_consensus.assertion_mask);
            const auto mirrored_projection = intersection(
                mirrored_actual, mirrored_consensus.assertion_mask);
            require_equal_sets(original_projection,
                               original_consensus.expected_true,
                               name + " original consensus");
            require_equal_sets(mirrored_projection,
                               mirrored_consensus.expected_true,
                               name + " mirrored consensus");
            require_equal_sets(rank_flip_labels(mirrored_projection),
                               original_projection,
                               name + " production rank symmetry");
            ++attack_pairs[name];
        };

        for (const char kind : std::string_view{"pbrqnk"}) {
            exercise_attack_pair(std::string("attack-") + kind, kind);
        }
        exercise_attack_pair("aggregate-attack", std::nullopt);

        Position original_check_position(original.fen());
        Position mirrored_check_position(mirrored.fen());
        const bool expected_original_check = original.in_check(original.turn);
        const bool expected_mirrored_check = mirrored.in_check(mirrored.turn);
        REQUIRE(expected_original_check == expected_mirrored_check);
        const bool actual_original_check = checked_position_query(
            original_check_position, "in-check original",
            [&](Position& position) {
                return production_in_check(position, original.turn);
            });
        const bool actual_mirrored_check = checked_position_query(
            mirrored_check_position, "in-check mirrored",
            [&](Position& position) {
                return production_in_check(position, mirrored.turn);
            });
        REQUIRE(actual_original_check == expected_original_check);
        REQUIRE(actual_mirrored_check == expected_mirrored_check);
        REQUIRE(actual_original_check == actual_mirrored_check);
        ++attack_pairs["in-check"];
    }

    const std::array<MoveType, 8> semantic_types{
        mt_Quiet, mt_EnPassant, mt_Castling, mt_Capture,
        mt_QueenPromotion, mt_RookPromotion,
        mt_BishopPromotion, mt_KnightPromotion};
    MoveList primitive_capacity_probe;
    const std::size_t primitive_capacity = primitive_capacity_probe.all().size();
    std::size_t destination_primitive_pairs = 0;
    if (primitive_capacity != 0) {
        for (int source_index = 0; source_index < 64; ++source_index) {
            const std::string source_label = square_name(source_index);
            const std::string mirrored_source_label = rank_flip_label(source_label);
            for (const MoveType type : semantic_types) {
                std::vector<std::string> destinations;
                for (int target_index = 0; target_index < 64; ++target_index) {
                    if (target_index != source_index)
                        destinations.push_back(square_name(target_index));
                }
                for (std::size_t begin = 0; begin < destinations.size();
                     begin += primitive_capacity) {
                    const std::size_t end = std::min(
                        destinations.size(), begin + primitive_capacity);
                    std::set<std::string> target_labels;
                    std::set<std::string> mirrored_target_labels;
                    std::set<std::string> expected_original;
                    std::set<std::string> expected_mirrored;
                    for (std::size_t index = begin; index < end; ++index) {
                        const std::string& target = destinations[index];
                        const std::string mirrored_target = rank_flip_label(target);
                        target_labels.insert(target);
                        mirrored_target_labels.insert(mirrored_target);
                        std::string original_uci = source_label + target;
                        std::string mirrored_uci = mirrored_source_label + mirrored_target;
                        if (const char suffix = promotion_suffix(type); suffix != 0) {
                            original_uci.push_back(suffix);
                            mirrored_uci.push_back(suffix);
                        }
                        expected_original.insert(std::move(original_uci));
                        expected_mirrored.insert(std::move(mirrored_uci));
                    }
                    require_equal_sets(rank_flip_moves(expected_mirrored),
                                       expected_original,
                                       "GenerateMovesFromBB oracle rank symmetry");

                    MoveList original_list;
                    MoveList mirrored_list;
                    const std::size_t original_capacity = original_list.all().size();
                    const std::size_t mirrored_capacity = mirrored_list.all().size();
                    REQUIRE(expected_original.size() <= original_capacity);
                    REQUIRE(expected_mirrored.size() <= mirrored_capacity);
                    MoveGen::GenerateMovesFromBB(
                        square_bijection().bitboard(target_labels), &original_list,
                        square_bijection().square(source_label), type);
                    MoveGen::GenerateMovesFromBB(
                        square_bijection().bitboard(mirrored_target_labels), &mirrored_list,
                        square_bijection().square(mirrored_source_label), type);
                    REQUIRE(original_list.len() <= original_capacity);
                    REQUIRE(mirrored_list.len() <= mirrored_capacity);
                    std::set<std::string> actual_original;
                    for (const auto& move : observe_moves(original_list, square_bijection())) {
                        REQUIRE(move.from == square_bijection().square(source_label));
                        REQUIRE(move.type == type);
                        actual_original.insert(move.uci);
                    }
                    std::set<std::string> actual_mirrored;
                    for (const auto& move : observe_moves(mirrored_list, square_bijection())) {
                        REQUIRE(move.from == square_bijection().square(mirrored_source_label));
                        REQUIRE(move.type == type);
                        actual_mirrored.insert(move.uci);
                    }
                    require_equal_sets(actual_original, expected_original,
                                       "GenerateMovesFromBB original product");
                    require_equal_sets(actual_mirrored, expected_mirrored,
                                       "GenerateMovesFromBB mirrored product");
                    require_equal_sets(rank_flip_moves(actual_mirrored), actual_original,
                                       "GenerateMovesFromBB production rank symmetry");
                    ++destination_primitive_pairs;
                }
            }
        }
    } else {
        INFO("GenerateMovesFromBB metamorphism unavailable: observed capacity is zero");
    }

    const auto occupied_sets = [](const RefPosition& reference) {
        std::pair<std::set<int>, std::set<int>> result;
        for (int square = 0; square < 64; ++square) {
            if (reference.occupied_by(square, reference.turn))
                result.first.insert(square);
            else if (reference.occupied_by(square, opposite(reference.turn)))
                result.second.insert(square);
        }
        return result;
    };
    const auto labelled_occupancy = [&](const std::set<int>& occupancy) {
        std::set<std::string> labels;
        for (const int square : occupancy) labels.insert(square_name(square));
        return square_bijection().bitboard(labels);
    };
    const auto sliding_expected = [](const int direction, const int source,
                                     const std::set<int>& friendly,
                                     const std::set<int>& enemy) {
        if (direction == 0) return sliding_reference(source, 0, 1, friendly, enemy);
        if (direction == 1) return sliding_reference(source, 1, 0, friendly, enemy);
        if (direction == 2) return sliding_reference(source, 1, 1, friendly, enemy);
        return sliding_reference(source, 1, -1, friendly, enemy);
    };
    const auto sliding_query = [&](const int direction, const Sq source,
                                   const BitBoard us, const BitBoard them,
                                   const bool expected_nonempty) {
        if (direction == 0)
            return sliding_actual<File>(source, us, them, square_bijection(), expected_nonempty);
        if (direction == 1)
            return sliding_actual<Rank>(source, us, them, square_bijection(), expected_nonempty);
        if (direction == 2)
            return sliding_actual<Diagonal>(source, us, them, square_bijection(), expected_nonempty);
        return sliding_actual<AntiDiagonal>(source, us, them, square_bijection(), expected_nonempty);
    };
    constexpr std::array<int, 4> mirrored_direction{0, 1, 3, 2};
    std::set<std::string> sliding_fixture_fens;
    std::size_t sliding_primitive_pairs = 0;
    for (const auto& fixture : fixtures) {
        const RefPosition& original = fixture.reference;
        if (!sliding_fixture_fens.insert(original.fen()).second) continue;
        const RefPosition mirrored = rank_flip_colour_swap(original);
        const auto [original_friendly, original_enemy] = occupied_sets(original);
        const auto [mirrored_friendly, mirrored_enemy] = occupied_sets(mirrored);
        const BitBoard original_us = labelled_occupancy(original_friendly);
        const BitBoard original_them = labelled_occupancy(original_enemy);
        const BitBoard mirrored_us = labelled_occupancy(mirrored_friendly);
        const BitBoard mirrored_them = labelled_occupancy(mirrored_enemy);
        for (const int source : original_friendly) {
            const int mirrored_source = index_of(file_of(source), 7 - rank_of(source));
            REQUIRE(mirrored_friendly.contains(mirrored_source));
            for (int direction = 0; direction < 4; ++direction) {
                const int mirror_direction = mirrored_direction[
                    static_cast<std::size_t>(direction)];
                const auto expected_original = sliding_expected(
                    direction, source, original_friendly, original_enemy);
                const auto expected_mirrored = sliding_expected(
                    mirror_direction, mirrored_source,
                    mirrored_friendly, mirrored_enemy);
                require_equal_sets(rank_flip_labels(expected_mirrored),
                                   expected_original,
                                   "GetMovesForSliding oracle rank symmetry");
                const auto actual_original = sliding_query(
                    direction, square_bijection().square(square_name(source)),
                    original_us, original_them, !expected_original.empty());
                const auto actual_mirrored = sliding_query(
                    mirror_direction,
                    square_bijection().square(square_name(mirrored_source)),
                    mirrored_us, mirrored_them, !expected_mirrored.empty());
                require_equal_sets(actual_original, expected_original,
                                   "GetMovesForSliding original geometry");
                require_equal_sets(actual_mirrored, expected_mirrored,
                                   "GetMovesForSliding mirrored geometry");
                require_equal_sets(rank_flip_labels(actual_mirrored),
                                   actual_original,
                                   "GetMovesForSliding production rank symmetry");
                ++sliding_primitive_pairs;
            }
        }
    }

    if (unavailable_move_pairs != 0 || primitive_capacity == 0) {
        SKIP("T-LEGAL-005 cannot prove its complete paired generator domain "
             "because an expected set does not fit observed MoveList capacity");
    }

    for (const std::string name : {
             "piece-p", "piece-b", "piece-r", "piece-q", "piece-n",
             "piece-k", "castling", "pseudo-legal", "legal"}) {
        INFO("move-pair-kind=" << name);
        REQUIRE(move_pairs[name] == fixtures.size());
    }
    for (const std::string name : {
             "attack-p", "attack-b", "attack-r", "attack-q", "attack-n",
             "attack-k", "aggregate-attack", "in-check"}) {
        INFO("attack-pair-kind=" << name);
        REQUIRE(attack_pairs[name] == fixtures.size());
    }
    REQUIRE(saw_promotion_pair);
    REQUIRE(saw_en_passant_pair);
    REQUIRE(saw_king_side_castle_pair);
    REQUIRE(saw_queen_side_castle_pair);
    REQUIRE(saw_checked_pair);
    REQUIRE(saw_nonchecked_pair);
    REQUIRE(destination_primitive_pairs > 0);
    REQUIRE(sliding_primitive_pairs > 0);
    INFO("unavailable-move-pairs=" << unavailable_move_pairs);
}

TEST_CASE("T-ORACLE-001 Offline legal-move corpus format checksum and row integrity",
          "[movegen][T-ORACLE-001][oracle][fixture][regression]") {
    const auto fixture = load_tsv_fixture("tests/data/oracle_cases.tsv");
    const std::vector<std::string> expected_columns{
        "case_id","source","root_fen","history_uci","fen","turn","in_check",
        "is_checkmate","is_stalemate","is_insufficient_material","is_seventyfive_moves",
        "is_threefold_repetition","is_fivefold_repetition","can_claim_fifty_moves",
        "can_claim_threefold_repetition","game_over","result","termination","winner","legal_uci"};
    REQUIRE(fixture.columns == expected_columns);
    REQUIRE(fixture.metadata.at("format") == "tdfa-chess-oracle-v1");
    REQUIRE(fixture.metadata.at("profile") == "committed");
    REQUIRE(fixture.metadata.at("generation_args") == "--profile committed");
    REQUIRE(fixture.metadata.at("python_chess") == "1.11.2");
    REQUIRE(fixture.metadata.at("seed") == "0x0000000054444641");
    REQUIRE(fixture.metadata.at("checksum_scope") == "column-header-and-data-rows-with-lf");
    const std::string expected_hash = "7c6bfab18aecbf89e6da7cdcc29b258edb13bf916804c9f211cb9752c7cb4e77";
    REQUIRE(fixture.metadata.at("data_sha256") == expected_hash);
    REQUIRE(sha256_hex(fixture.scoped_bytes) == expected_hash);
    const auto rows = oracle_cases(fixture);
    REQUIRE(rows.size() == 27);
    std::map<std::string, std::size_t> sources;
    std::set<std::string> ids;
    const auto legal_column = column_index(fixture, "legal_uci");
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& row = rows[index];
        INFO("physical-data-row=" << index + 1 << " case_id=" << row.case_id);
        ++sources[row.source];
        REQUIRE(ids.insert(row.case_id).second);
        REQUIRE(RefPosition::from_fen(row.fen).fen() == row.fen);
        REQUIRE(RefPosition::from_fen(row.root_fen).fen() == row.root_fen);
        REQUIRE(std::is_sorted(row.legal_moves.begin(), row.legal_moves.end()));
        REQUIRE(std::adjacent_find(row.legal_moves.begin(), row.legal_moves.end()) == row.legal_moves.end());
        for (const auto& token : row.legal_moves) REQUIRE(canonical_uci_token(token));
        for (const auto& token : row.history) REQUIRE(canonical_uci_token(token));
        REQUIRE(((row.legal_moves.empty() && fixture.rows[index][legal_column] == "-") ||
                 (!row.legal_moves.empty() && fixture.rows[index][legal_column] != "-")));
    }
    const std::map<std::string, std::size_t> expected_sources{
        {"curated",16},{"history",3},{"random_walk",8}};
    REQUIRE(sources == expected_sources);
    const std::set<std::string> expected_ids{
        "start","kiwipete","perft_endgame","perft_promotions","perft_castling","perft_tactics",
        "castling_minimal","en_passant_legal","en_passant_pinned","promotion_choices","checkmate",
        "stalemate","insufficient_material","seventyfive_move_rule","fifty_move_prospective",
        "fifty_move_clock_100","threefold_prospective","threefold_current","fivefold_repetition",
        "random_000000","random_000001","random_000002","random_000003","random_000004",
        "random_000005","random_000006","random_000007"};
    REQUIRE(ids == expected_ids);
}
