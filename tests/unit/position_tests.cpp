#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Move.hpp"
#include "Position.hpp"
#include "Types.hpp"
#include "Util.hpp"
#include "support/PrimitivesPositionTestModels.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace
{

using namespace tdfa_test;

struct TransitionRow
{
    const char* label;
    const char* pre_fen;
    const char* uci;
    MoveType type;
    const char* post_fen;
};

inline constexpr std::array<TransitionRow, 9> kSinglePlyRows{
    TransitionRow{
        "quiet knight",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "g1f3", mt_Quiet,
        "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1"},
    TransitionRow{
        "quiet double pawn push",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "e2e4", mt_Quiet,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"},
    TransitionRow{
        "ordinary capture",
        "4k3/8/8/3p4/4P3/8/8/4K3 w - - 7 9",
        "e4d5", mt_Capture,
        "4k3/8/8/3P4/8/8/8/4K3 b - - 0 9"},
    TransitionRow{
        "en-passant capture",
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
        "e5d6", mt_EnPassant,
        "4k3/8/3P4/8/8/8/8/4K3 b - - 0 2"},
    TransitionRow{
        "king-side castling",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "e1g1", mt_Castling,
        "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1"},
    TransitionRow{
        "queen promotion",
        "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
        "a7a8q", mt_QueenPromotion,
        "Q3k3/8/8/8/8/8/7p/4K3 b - - 0 1"},
    TransitionRow{
        "rook promotion",
        "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
        "a7a8r", mt_RookPromotion,
        "R3k3/8/8/8/8/8/7p/4K3 b - - 0 1"},
    TransitionRow{
        "bishop promotion",
        "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
        "a7a8b", mt_BishopPromotion,
        "B3k3/8/8/8/8/8/7p/4K3 b - - 0 1"},
    TransitionRow{
        "knight promotion",
        "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
        "a7a8n", mt_KnightPromotion,
        "N3k3/8/8/8/8/8/7p/4K3 b - - 0 1"},
};

inline constexpr std::array<TransitionRow, 7> kCastlingRows{
    TransitionRow{
        "white king-side castle",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "e1g1", mt_Castling,
        "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1"},
    TransitionRow{
        "white queen-side castle",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "e1c1", mt_Castling,
        "r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1"},
    TransitionRow{
        "black king-side castle",
        "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
        "e8g8", mt_Castling,
        "r4rk1/8/8/8/8/8/8/R3K2R w KQ - 1 2"},
    TransitionRow{
        "black queen-side castle",
        "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
        "e8c8", mt_Castling,
        "2kr3r/8/8/8/8/8/8/R3K2R w KQ - 1 2"},
    TransitionRow{
        "white home-rook move",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "h1h2", mt_Quiet,
        "r3k2r/8/8/8/8/8/7R/R3K3 b Qkq - 1 1"},
    TransitionRow{
        "white king move",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "e1e2", mt_Quiet,
        "r3k2r/8/8/8/8/8/4K3/R6R b kq - 1 1"},
    TransitionRow{
        "capture black queen-side home rook",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "a1a8", mt_Capture,
        "R3k2r/8/8/8/8/8/8/4K2R b Kk - 0 1"},
};

inline constexpr std::array<std::string_view, 5> kPrefixFens{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
    "rnbqkbnr/pp2pppp/3p4/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
};

inline constexpr std::array<std::string_view, 4> kPrefixMoves{
    "e2e4", "c7c5", "g1f3", "d7d6",
};

void require_exact_position(const Position& position,
                            std::string_view expected_fen,
                            std::string_view context)
{
    const ExpectedPosition expected = decode_fen_independently(expected_fen);
    const ObservedPosition actual = observe_position(position);
    std::string reason;
    if (!position_matches(expected, actual, reason))
    {
        INFO("context=" << context);
        INFO("expected_fen=" << expected_fen);
        INFO("reason=" << reason);
        INFO("actual turn=" << static_cast<unsigned>(actual.turn)
                             << " rights=" << static_cast<unsigned>(actual.castling)
                             << " ep_board=" << hex_u64(actual.en_passant_board)
                             << " halfmove=" << static_cast<unsigned>(actual.halfmove)
                             << " fullmove=" << static_cast<unsigned>(actual.fullmove));
        FAIL("Position semantic snapshot disagrees with the independent literal FEN model");
    }
}

void require_placement_and_side(const Position& position,
                                std::string_view expected_fen,
                                std::string_view context)
{
    const ExpectedPosition expected = decode_fen_independently(expected_fen);
    const ObservedPosition actual = observe_position(position);
    std::string reason;
    if (!placement_and_side_match(expected, actual, reason))
    {
        INFO("context=" << context);
        INFO("expected_fen=" << expected_fen);
        INFO("reason=" << reason);
        FAIL("Position placement/side disagrees with the approved unmake projection");
    }
}

Move parse_and_require_type(Position& position,
                            std::string_view uci,
                            MoveType expected_type,
                            std::string_view context)
{
    const Move move = UTIL::UciToMove(uci, position);
    Sq start = 0;
    Sq target = 0;
    MoveType actual_type = mt_Quiet;
    Moves::DecodeMove(move, &start, &target, &actual_type);
    INFO("context=" << context << " uci=" << uci
                    << " move_bits=" << static_cast<unsigned>(move));
    INFO("decoded_start=" << static_cast<unsigned>(start)
                           << " decoded_target=" << static_cast<unsigned>(target)
                           << " expected_type=" << static_cast<unsigned>(expected_type)
                           << " actual_type=" << static_cast<unsigned>(actual_type));
    REQUIRE(actual_type == expected_type);
    return move;
}

} // namespace

TEST_CASE("T-FEN-001 Frozen valid FENs import to exact semantic snapshots",
          "[T-FEN-001][position][deep]")
{
    auto [case_table, cases] = load_oracle_cases();
    auto [perft_table, perft_fens] = load_oracle_perft_fens();
    REQUIRE(case_table.metadata.at("data_sha256") == kCasesDataSha256);
    REQUIRE(perft_table.metadata.at("data_sha256") == kPerftDataSha256);
    REQUIRE(blind::sha256_hex(case_table.scoped_bytes) == kCasesDataSha256);
    REQUIRE(blind::sha256_hex(perft_table.scoped_bytes) == kPerftDataSha256);
    REQUIRE(cases.size() == 27);
    REQUIRE(perft_fens.size() == 18);

    const auto fixtures = fen_001_fixtures();
    REQUIRE_FALSE(fixtures.empty());
    for (std::size_t index = 0; index < fixtures.size(); ++index)
    {
        const auto& fen = fixtures[index];
        const auto& next = fixtures[(index + 1) % fixtures.size()];
        INFO("fixture_index=" << index << " fen=" << fen << " prior_fen=" << next);

        Position direct(fen);
        require_exact_position(direct, fen, "direct construction");

        Position replacement(next);
        replacement.ImportFen(fen);
        require_exact_position(replacement, fen, "replacement import over next byte-sorted fixture");
    }
}

TEST_CASE("T-FEN-003 Constructor and repeated replacement imports are semantically equivalent",
          "[T-FEN-003][position][deep]")
{
    auto [table, cases] = load_oracle_cases();
    REQUIRE(table.metadata.at("data_sha256") == kCasesDataSha256);
    REQUIRE(blind::sha256_hex(table.scoped_bytes) == kCasesDataSha256);
    REQUIRE(cases.size() == 27);

    for (const auto& source : cases)
    {
        for (const auto& target : cases)
        {
            INFO("A_case_id=" << source.case_id << " A_fen=" << source.fen);
            INFO("B_case_id=" << target.case_id << " B_fen=" << target.fen);

            Position fresh(target.fen);
            require_exact_position(fresh, target.fen, "fresh(B)");

            Position replacement(source.fen);
            replacement.ImportFen(target.fen);
            require_exact_position(replacement, target.fen, "construct(A), ImportFen(B)");

            Position repeated(target.fen);
            require_exact_position(repeated, target.fen, "construct(B) before repeated import");
            repeated.ImportFen(target.fen);
            require_exact_position(repeated, target.fen, "construct(B), first ImportFen(B)");
            repeated.ImportFen(target.fen);
            require_exact_position(repeated, target.fen, "construct(B), second ImportFen(B)");
        }
    }
}

TEST_CASE("T-POS-002 Position observers satisfy exact placement set algebra",
          "[T-POS-002][position][deep]")
{
    const auto fixtures = fen_001_fixtures();
    for (std::size_t fixture = 0; fixture < fixtures.size(); ++fixture)
    {
        Position position(fixtures[fixture]);
        require_exact_position(position, fixtures[fixture], "T-POS-002 fixture decode");
        const ObservedPosition before = observe_position(position);
        INFO("fixture_index=" << fixture << " fen=" << fixtures[fixture]);

        for (unsigned repetition = 0; repetition < 2; ++repetition)
        {
            const BitBoard white = position.Pieces(White);
            const BitBoard black = position.Pieces(Black);
            const BitBoard occupied = white | black;
            constexpr std::array<PieceType, 6> types{
                pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
            };
            BitBoard type_union = 0;
            std::array<BitBoard, 6> type_masks{};
            for (std::size_t type = 0; type < types.size(); ++type)
            {
                type_masks[type] = position.Pieces(types[type]);
                for (std::size_t previous = 0; previous < type; ++previous)
                {
                    INFO("repetition=" << repetition << " type=" << type
                                       << " previous_type=" << previous);
                    CHECK((type_masks[type] & type_masks[previous]) == 0);
                }
                type_union |= type_masks[type];
            }

            INFO("repetition=" << repetition << " white=" << hex_u64(white)
                               << " black=" << hex_u64(black)
                               << " occupied=" << hex_u64(occupied));
            CHECK((white & black) == 0);
            CHECK(type_union == occupied);
            CHECK(position.EmptySqs() == ~occupied);

            const BitBoard ep = position.EnPasBB();
            const unsigned ep_count = model_popcount(ep);
            INFO("ep=" << hex_u64(ep) << " ep_count=" << ep_count);
            REQUIRE((ep_count == 0 || ep_count == 1));
            if (ep != 0)
            {
                const Sq ep_square = position.EnPasSq();
                INFO("ep_square=" << static_cast<unsigned>(ep_square));
                CHECK(ep == (U64{1} << static_cast<unsigned>(ep_square)));
            }

            CHECK(position.CastlingRights() == before.castling);
            CHECK(position.ColourToMove() == before.turn);
            CHECK(position.HalfMoves() == before.halfmove);
            CHECK(position.FullMoves() == before.fullmove);
        }
        CHECK(observe_position(position) == before);
    }
}

TEST_CASE("T-POS-003 IsOk accepts every frozen legal corpus position without mutation",
          "[T-POS-003][position][fast]")
{
    const auto fens = corpus_position_fens();
    for (std::size_t index = 0; index < fens.size(); ++index)
    {
        Position position(fens[index]);
        const ObservedPosition before = observe_position(position);
        const bool first = position.IsOk();
        const ObservedPosition between = observe_position(position);
        const bool second = position.IsOk();
        const ObservedPosition after = observe_position(position);
        INFO("fixture_index=" << index << " fen=" << fens[index]);
        INFO("first=" << first << " second=" << second);
        CHECK(first);
        CHECK(second);
        CHECK(between == before);
        CHECK(after == before);
    }
}

TEST_CASE("T-STA-001 Single-ply make applies every named semantic move type exactly",
          "[T-STA-001][position][fast]")
{
    for (std::size_t row_index = 0; row_index < kSinglePlyRows.size(); ++row_index)
    {
        const auto& row = kSinglePlyRows[row_index];
        Position position(row.pre_fen);
        require_exact_position(position, row.pre_fen, "T-STA-001 pre-state");
        const Move move = parse_and_require_type(position, row.uci, row.type, row.label);
        position.MakeMove(move);
        INFO("row_index=" << row_index << " label=" << row.label);
        INFO("pre=" << row.pre_fen << " uci=" << row.uci << " post=" << row.post_fen);
        require_exact_position(position, row.post_fen, "T-STA-001 made post-state");
    }
}

TEST_CASE("T-STA-003 Castling transitions are exact forward and restore placement/side",
          "[T-STA-003][position][fast]")
{
    for (std::size_t row_index = 0; row_index < kCastlingRows.size(); ++row_index)
    {
        const auto& row = kCastlingRows[row_index];
        Position position(row.pre_fen);
        require_exact_position(position, row.pre_fen, "T-STA-003 pre-state");
        const Move move = parse_and_require_type(position, row.uci, row.type, row.label);
        position.MakeMove(move);
        INFO("row_index=" << row_index << " label=" << row.label);
        INFO("pre=" << row.pre_fen << " uci=" << row.uci << " post=" << row.post_fen);
        require_exact_position(position, row.post_fen, "T-STA-003 made post-state");
        position.UnmakeMove(move);
        require_placement_and_side(position, row.pre_fen,
                                   "T-STA-003 restored placement and side");
    }
}

TEST_CASE("T-STA-004 Four-ply LIFO is exact forward and restores placement/side",
          "[T-STA-004][position][fast]")
{
    for (unsigned cycle = 0; cycle < 2; ++cycle)
    {
        Position position(kPrefixFens[0]);
        INFO("cycle=" << cycle);
        require_exact_position(position, kPrefixFens[0], "T-STA-004 cycle root P0");
        std::array<Move, 4> moves{};
        for (std::size_t depth = 0; depth < kPrefixMoves.size(); ++depth)
        {
            INFO("cycle=" << cycle << " direction=forward depth=" << depth
                           << " uci=" << kPrefixMoves[depth]);
            moves[depth] = UTIL::UciToMove(kPrefixMoves[depth], position);
            position.MakeMove(moves[depth]);
            require_exact_position(position, kPrefixFens[depth + 1],
                                   "T-STA-004 forward prefix");
        }
        for (std::size_t depth = kPrefixMoves.size(); depth > 0; --depth)
        {
            INFO("cycle=" << cycle << " direction=reverse depth=" << depth
                           << " uci=" << kPrefixMoves[depth - 1]);
            position.UnmakeMove(moves[depth - 1]);
            require_placement_and_side(position, kPrefixFens[depth - 1],
                                       "T-STA-004 reverse placement and side");
        }
    }
}
