#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Board.hpp"
#include "BoardUtils.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "Position.hpp"
#include "Types.hpp"
#include "Util.hpp"
#include "support/PrimitivesPositionTestModels.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{

using namespace tdfa_test;

constexpr std::array<MoveType, 8> kSemanticMoveTypes{
    mt_Quiet,
    mt_EnPassant,
    mt_Castling,
    mt_Capture,
    mt_QueenPromotion,
    mt_BishopPromotion,
    mt_KnightPromotion,
    mt_RookPromotion,
};

constexpr std::array<PieceType, 6> kOrdinaryPieceTypes{
    pt_King, pt_Queen, pt_Bishop, pt_Knight, pt_Rook, pt_Pawn,
};

constexpr std::array<Piece, 12> kPieces{
    p_WhiteKing, p_WhiteQueen, p_WhiteBishop, p_WhiteKnight, p_WhiteRook, p_WhitePawn,
    p_BlackKing, p_BlackQueen, p_BlackBishop, p_BlackKnight, p_BlackRook, p_BlackPawn,
};

constexpr std::array<Colour, 2> kColours{White, Black};

template <std::size_t... I>
consteval std::array<BitBoard, sizeof...(I)> make_consteval_square_bits(
    std::index_sequence<I...>)
{
    return {Magics::SqToBB<static_cast<Sq>(I)>()...};
}

inline constexpr auto kConstevalSquareBits =
    make_consteval_square_bits(std::make_index_sequence<64>{});

using ShiftFunction = BitBoard (*)(BitBoard);

template <MD Direction>
constexpr BitBoard invoke_shift(BitBoard board)
{
    return Magics::Shift<Direction>(board);
}

struct DirectionSpec
{
    const char* name;
    ShiftFunction shift;
    std::size_t opposite;
    unsigned discarded_singletons;
};

constexpr std::array<DirectionSpec, 10> kDirections{
    DirectionSpec{"NORTH", invoke_shift<NORTH>, 4, 8},
    DirectionSpec{"NORTH_EAST", invoke_shift<NORTH_EAST>, 5, 15},
    DirectionSpec{"EAST", invoke_shift<EAST>, 6, 8},
    DirectionSpec{"SOUTH_EAST", invoke_shift<SOUTH_EAST>, 7, 15},
    DirectionSpec{"SOUTH", invoke_shift<SOUTH>, 0, 8},
    DirectionSpec{"SOUTH_WEST", invoke_shift<SOUTH_WEST>, 1, 15},
    DirectionSpec{"WEST", invoke_shift<WEST>, 2, 8},
    DirectionSpec{"NORTH_WEST", invoke_shift<NORTH_WEST>, 3, 15},
    DirectionSpec{"NORTHNORTH", invoke_shift<NORTHNORTH>, 9, 16},
    DirectionSpec{"SOUTHSOUTH", invoke_shift<SOUTHSOUTH>, 8, 16},
};

void require_popcount_match(BitBoard board, std::string_view corpus, std::size_t index)
{
    const unsigned expected = model_popcount(board);
    const unsigned actual = Magics::PopCnt(board);
    if (actual != expected || actual > 64)
    {
        INFO("corpus=" << corpus << " index=" << index);
        INFO("board=" << hex_u64(board) << " expected=" << expected << " actual=" << actual);
        FAIL("PopCnt disagrees with the independent 64-coefficient model");
    }

    const unsigned complement = Magics::PopCnt(~board);
    if (complement != 64u - expected)
    {
        INFO("corpus=" << corpus << " index=" << index << " board=" << hex_u64(board));
        INFO("expected_complement=" << 64u - expected << " actual_complement=" << complement);
        FAIL("PopCnt violates the fixed-width complement identity");
    }
}

void require_scan_match(BitBoard board, std::string_view corpus, std::size_t index)
{
    const auto expected = model_set_indices(board);
    REQUIRE_FALSE(expected.empty());
    const BitBoard expected_low_bit = U64{1} << expected.front();
    const BitBoard expected_without_low = model_board_from_indices(expected, 1, expected.size());
    const BitBoard expected_without_high = model_board_from_indices(expected, 0, expected.size() - 1);

    const BitBoard low_bit = Magics::GetLS1B(board);
    const Sq low_index = Magics::FindLS1B(board);
    const Sq high_index = Magics::FindMS1B(board);
    const BitBoard without_low = Magics::PopLS1B(board);
    const BitBoard without_high = Magics::PopMS1B(board);
    if (low_bit != expected_low_bit || low_index != static_cast<Sq>(expected.front()) ||
        high_index != static_cast<Sq>(expected.back()) || without_low != expected_without_low ||
        without_high != expected_without_high)
    {
        INFO("corpus=" << corpus << " index=" << index << " board=" << hex_u64(board));
        INFO("expected_first=" << expected.front() << " expected_last=" << expected.back());
        INFO("GetLS1B=" << hex_u64(low_bit)
                          << " FindLS1B=" << static_cast<unsigned>(low_index)
                          << " FindMS1B=" << static_cast<unsigned>(high_index));
        INFO("PopLS1B=" << hex_u64(without_low)
                          << " PopMS1B=" << hex_u64(without_high));
        FAIL("a nonzero bit query/removal disagrees with the ordered-set model");
    }

    BitBoard mutable_board = board;
    std::vector<unsigned> returned;
    returned.reserve(expected.size());
    unsigned previous_count = static_cast<unsigned>(expected.size());
    while (mutable_board != 0)
    {
        const BitBoard before = mutable_board;
        const Sq returned_index = Magics::PopNRetLS1B(mutable_board);
        returned.push_back(static_cast<unsigned>(returned_index));
        const unsigned next_count = model_popcount(mutable_board);
        const BitBoard expected_after =
            model_board_from_indices(expected, returned.size(), expected.size());
        if (returned_index != static_cast<Sq>(expected[returned.size() - 1]) ||
            mutable_board != expected_after || next_count + 1 != previous_count)
        {
            INFO("corpus=" << corpus << " index=" << index << " step=" << returned.size() - 1);
            INFO("before=" << hex_u64(before) << " after=" << hex_u64(mutable_board));
            INFO("expected_index=" << expected[returned.size() - 1]
                                     << " actual_index=" << static_cast<unsigned>(returned_index));
            FAIL("PopNRetLS1B did not return and remove exactly the next low bit");
        }
        CHECK(mutable_board == Magics::PopLS1B(before));
        previous_count = next_count;
    }
    CHECK(returned == expected);
}

constexpr BitBoard model_attack_mask(unsigned square, bool knight)
{
    BitBoard result = 0;
    const int file = static_cast<int>(square % 8u);
    const int rank = static_cast<int>(square / 8u);
    constexpr std::array<std::pair<int, int>, 8> king_deltas{
        std::pair{-1, -1}, std::pair{0, -1}, std::pair{1, -1}, std::pair{-1, 0},
        std::pair{1, 0}, std::pair{-1, 1}, std::pair{0, 1}, std::pair{1, 1},
    };
    constexpr std::array<std::pair<int, int>, 8> knight_deltas{
        std::pair{-2, -1}, std::pair{-2, 1}, std::pair{-1, -2}, std::pair{-1, 2},
        std::pair{1, -2}, std::pair{1, 2}, std::pair{2, -1}, std::pair{2, 1},
    };
    const auto& deltas = knight ? knight_deltas : king_deltas;
    for (const auto [file_delta, rank_delta] : deltas)
    {
        const int target_file = file + file_delta;
        const int target_rank = rank + rank_delta;
        if (target_file >= 0 && target_file < 8 && target_rank >= 0 && target_rank < 8)
            result |= U64{1} << static_cast<unsigned>(target_file + 8 * target_rank);
    }
    return result;
}

BoardModel aggregate_fixture_model()
{
    BoardModel model = empty_board_model();
    constexpr std::array<std::pair<Piece, unsigned>, 24> entries{
        std::pair{p_WhiteKing, 0u}, std::pair{p_WhiteKing, 15u},
        std::pair{p_WhiteQueen, 1u}, std::pair{p_WhiteQueen, 14u},
        std::pair{p_WhiteBishop, 2u}, std::pair{p_WhiteBishop, 13u},
        std::pair{p_WhiteKnight, 3u}, std::pair{p_WhiteKnight, 12u},
        std::pair{p_WhiteRook, 4u}, std::pair{p_WhiteRook, 11u},
        std::pair{p_WhitePawn, 5u}, std::pair{p_WhitePawn, 10u},
        std::pair{p_BlackKing, 63u}, std::pair{p_BlackKing, 48u},
        std::pair{p_BlackQueen, 62u}, std::pair{p_BlackQueen, 49u},
        std::pair{p_BlackBishop, 61u}, std::pair{p_BlackBishop, 50u},
        std::pair{p_BlackKnight, 60u}, std::pair{p_BlackKnight, 51u},
        std::pair{p_BlackRook, 59u}, std::pair{p_BlackRook, 52u},
        std::pair{p_BlackPawn, 58u}, std::pair{p_BlackPawn, 53u},
    };
    for (const auto [piece, square] : entries)
        model[square] = piece;
    return model;
}

void populate_board(Board& board, const BoardModel& model)
{
    for (unsigned square = 0; square < 64; ++square)
        if (model[square] != p_None)
            board.AddPiece(model[square], static_cast<Sq>(square));
}

template <unsigned Mask>
consteval PieceType first_selected_type()
{
    static_assert(Mask > 0 && Mask < 64);
    for (unsigned index = 0; index < 6; ++index)
        if ((Mask & (1u << index)) != 0)
            return kOrdinaryPieceTypes[index];
    return pt_King;
}

template <unsigned Mask, std::size_t I = 0, PieceType... Selected>
BitBoard query_types_declaration_order(const Board& board)
{
    if constexpr (I == kOrdinaryPieceTypes.size())
        return board.Pieces(Selected...);
    else if constexpr ((Mask & (1u << I)) != 0)
        return query_types_declaration_order<Mask, I + 1, Selected..., kOrdinaryPieceTypes[I]>(board);
    else
        return query_types_declaration_order<Mask, I + 1, Selected...>(board);
}

template <unsigned Mask, int I = 5, PieceType... Selected>
BitBoard query_types_reverse_order(const Board& board)
{
    if constexpr (I < 0)
        return board.Pieces(Selected...);
    else if constexpr ((Mask & (1u << I)) != 0)
        return query_types_reverse_order<Mask, I - 1, Selected..., kOrdinaryPieceTypes[I]>(board);
    else
        return query_types_reverse_order<Mask, I - 1, Selected...>(board);
}

template <unsigned Mask, std::size_t I = 0, PieceType... Selected>
BitBoard query_types_with_duplicate(const Board& board)
{
    if constexpr (I == kOrdinaryPieceTypes.size())
        return board.Pieces(Selected..., first_selected_type<Mask>());
    else if constexpr ((Mask & (1u << I)) != 0)
        return query_types_with_duplicate<Mask, I + 1, Selected..., kOrdinaryPieceTypes[I]>(board);
    else
        return query_types_with_duplicate<Mask, I + 1, Selected...>(board);
}

template <unsigned Mask, std::size_t I = 0, PieceType... Selected>
BitBoard query_colour_types_declaration_order(const Board& board, Colour colour)
{
    if constexpr (I == kOrdinaryPieceTypes.size())
        return board.Pieces(colour, Selected...);
    else if constexpr ((Mask & (1u << I)) != 0)
        return query_colour_types_declaration_order<Mask, I + 1, Selected..., kOrdinaryPieceTypes[I]>(board, colour);
    else
        return query_colour_types_declaration_order<Mask, I + 1, Selected...>(board, colour);
}

template <unsigned Mask, int I = 5, PieceType... Selected>
BitBoard query_colour_types_reverse_order(const Board& board, Colour colour)
{
    if constexpr (I < 0)
        return board.Pieces(colour, Selected...);
    else if constexpr ((Mask & (1u << I)) != 0)
        return query_colour_types_reverse_order<Mask, I - 1, Selected..., kOrdinaryPieceTypes[I]>(board, colour);
    else
        return query_colour_types_reverse_order<Mask, I - 1, Selected...>(board, colour);
}

template <unsigned Mask, std::size_t I = 0, PieceType... Selected>
BitBoard query_colour_types_with_duplicate(const Board& board, Colour colour)
{
    if constexpr (I == kOrdinaryPieceTypes.size())
        return board.Pieces(colour, Selected..., first_selected_type<Mask>());
    else if constexpr ((Mask & (1u << I)) != 0)
        return query_colour_types_with_duplicate<Mask, I + 1, Selected..., kOrdinaryPieceTypes[I]>(board, colour);
    else
        return query_colour_types_with_duplicate<Mask, I + 1, Selected...>(board, colour);
}

template <unsigned Mask>
void check_aggregate_subset(const Board& board, const BoardModel& model)
{
    static_assert(Mask > 0 && Mask < 64);
    BitBoard expected_types = 0;
    for (unsigned square = 0; square < 64; ++square)
    {
        const int type = model_type_index(model[square]);
        if (type >= 0 && (Mask & (1u << static_cast<unsigned>(type))) != 0)
            expected_types |= U64{1} << square;
    }

    const auto declaration = query_types_declaration_order<Mask>(board);
    const auto reverse = query_types_reverse_order<Mask>(board);
    const auto duplicate = query_types_with_duplicate<Mask>(board);
    INFO("subset_mask=" << Mask << " expected=" << hex_u64(expected_types));
    CHECK(declaration == expected_types);
    CHECK(reverse == expected_types);
    CHECK(duplicate == expected_types);

    for (std::size_t colour_index = 0; colour_index < kColours.size(); ++colour_index)
    {
        const BitBoard expected_colour = expected_types & model_board_mask(model,
                                                                           static_cast<int>(colour_index),
                                                                           -1);
        const auto colour = kColours[colour_index];
        INFO("colour_index=" << colour_index << " expected_colour=" << hex_u64(expected_colour));
        CHECK(query_colour_types_declaration_order<Mask>(board, colour) == expected_colour);
        CHECK(query_colour_types_reverse_order<Mask>(board, colour) == expected_colour);
        CHECK(query_colour_types_with_duplicate<Mask>(board, colour) == expected_colour);
    }
}

template <std::size_t... I>
void check_all_aggregate_subsets(const Board& board,
                                 const BoardModel& model,
                                 std::index_sequence<I...>)
{
    (check_aggregate_subset<static_cast<unsigned>(I + 1)>(board, model), ...);
}

constexpr bool is_promotion_kind(MoveType type)
{
    return type == mt_QueenPromotion || type == mt_RookPromotion ||
           type == mt_BishopPromotion || type == mt_KnightPromotion;
}

const char* move_type_name(MoveType type)
{
    switch (type)
    {
    case mt_Quiet: return "mt_Quiet";
    case mt_EnPassant: return "mt_EnPassant";
    case mt_Castling: return "mt_Castling";
    case mt_Capture: return "mt_Capture";
    case mt_QueenPromotion: return "mt_QueenPromotion";
    case mt_BishopPromotion: return "mt_BishopPromotion";
    case mt_KnightPromotion: return "mt_KnightPromotion";
    case mt_RookPromotion: return "mt_RookPromotion";
    default: return "excluded MoveType";
    }
}

template <typename T>
void check_float_div_type(std::string_view type_name, bool include_negative)
{
    struct Case
    {
        int numerator;
        int denominator;
        float expected;
    };
    constexpr std::array<Case, 5> ordinary{
        Case{0, 1, 0.0F},
        Case{1, 1, 1.0F},
        Case{1, 2, 0.5F},
        Case{3, 2, 1.5F},
        Case{8, 8, 1.0F},
    };
    constexpr std::array<Case, 3> negative{
        Case{-1, 1, -1.0F},
        Case{1, -2, -0.5F},
        Case{-3, -2, 1.5F},
    };

    const auto check = [&](const Case& test_case) {
        const T numerator = static_cast<T>(test_case.numerator);
        const T denominator = static_cast<T>(test_case.denominator);
        const float actual = FloatDiv<T>(numerator, denominator);
        if (actual != test_case.expected)
        {
            INFO("T=" << type_name);
            INFO("numerator=" << test_case.numerator << " denominator=" << test_case.denominator);
            INFO("expected_bits=" << std::bit_cast<std::uint32_t>(test_case.expected)
                                  << " actual_bits=" << std::bit_cast<std::uint32_t>(actual));
            FAIL("FloatDiv returned the wrong exact binary32 quotient");
        }

        const T scaled_numerator = static_cast<T>(test_case.numerator * 2);
        const T scaled_denominator = static_cast<T>(test_case.denominator * 2);
        const float scaled = FloatDiv<T>(scaled_numerator, scaled_denominator);
        if (scaled != test_case.expected)
        {
            INFO("T=" << type_name << " scaled tuple");
            INFO("numerator=" << test_case.numerator * 2
                               << " denominator=" << test_case.denominator * 2);
            INFO("expected_bits=" << std::bit_cast<std::uint32_t>(test_case.expected)
                                  << " actual_bits=" << std::bit_cast<std::uint32_t>(scaled));
            FAIL("FloatDiv changed after exact common scaling");
        }
    };

    for (const auto& test_case : ordinary)
        check(test_case);
    if (include_negative)
        for (const auto& test_case : negative)
            check(test_case);
}

} // namespace

TEST_CASE("T-TYP-001 Colour negation is the exhaustive two-colour involution",
          "[T-TYP-001][primitives][fast]")
{
    struct Row
    {
        Colour input;
        Colour expected;
        const char* label;
    };
    constexpr std::array<Row, 2> rows{
        Row{White, Black, "White"},
        Row{Black, White, "Black"},
    };

    for (const auto& row : rows)
    {
        const Colour once = !row.input;
        const Colour twice = !once;
        INFO("input=" << row.label << " input_value=" << static_cast<unsigned>(row.input));
        INFO("once_value=" << static_cast<unsigned>(once)
                            << " twice_value=" << static_cast<unsigned>(twice));
        CHECK(once == row.expected);
        CHECK(once != row.input);
        CHECK(twice == row.input);
    }
}

TEST_CASE("T-TYP-002 FloatDiv returns exact representable ordinary quotients",
          "[T-TYP-002][primitives][fast]")
{
    check_float_div_type<U8>("U8", false);
    check_float_div_type<U16>("U16", false);
    check_float_div_type<U32>("U32", false);
    check_float_div_type<U64>("U64", false);
    check_float_div_type<I16>("I16", true);
    check_float_div_type<int>("int", true);
    check_float_div_type<float>("float", true);
}

TEST_CASE("T-MOV-001 Move encoding round-trips the complete valid component product",
          "[T-MOV-001][primitives][deep]")
{
    std::set<Move> encodings;
    std::size_t ordinal = 0;
    for (unsigned start = 0; start < 64; ++start)
    {
        for (unsigned target = 0; target < 64; ++target)
        {
            if (start == target)
                continue;
            for (const MoveType type : kSemanticMoveTypes)
            {
                ++ordinal;
                const Sq input_start = static_cast<Sq>(start);
                const Sq input_target = static_cast<Sq>(target);
                const Move encoded = Moves::EncodeMove(input_start, input_target, type);
                Sq decoded_start = 0;
                Sq decoded_target = 0;
                MoveType decoded_type = mt_Quiet;
                Moves::DecodeMove(encoded, &decoded_start, &decoded_target, &decoded_type);
                const bool decoded_start_valid = static_cast<unsigned>(decoded_start) < 64;
                const bool decoded_target_valid = static_cast<unsigned>(decoded_target) < 64;
                const bool decoded_type_valid =
                    std::find(kSemanticMoveTypes.begin(), kSemanticMoveTypes.end(), decoded_type) !=
                    kSemanticMoveTypes.end();
                INFO("ordinal=" << ordinal << " start=" << start << " target=" << target);
                INFO("encoded=" << static_cast<unsigned>(encoded)
                                  << " decoded_start=" << static_cast<unsigned>(decoded_start)
                                  << " decoded_target=" << static_cast<unsigned>(decoded_target)
                                  << " decoded_type=" << static_cast<unsigned>(decoded_type));
                REQUIRE(decoded_start_valid);
                REQUIRE(decoded_target_valid);
                REQUIRE(decoded_type_valid);
                const Move reencoded = Moves::EncodeMove(decoded_start, decoded_target, decoded_type);
                const bool unique = encodings.insert(encoded).second;

                if (decoded_start != input_start || decoded_target != input_target ||
                    decoded_type != type || Moves::StartSq(encoded) != input_start ||
                    Moves::TargetSq(encoded) != input_target || reencoded != encoded || !unique)
                {
                    INFO("ordinal=" << ordinal << " start=" << start << " target=" << target);
                    INFO("type=" << move_type_name(type)
                                  << " type_value=" << static_cast<unsigned>(type));
                    INFO("encoded=" << static_cast<unsigned>(encoded)
                                    << " decoded_start=" << static_cast<unsigned>(decoded_start)
                                    << " decoded_target=" << static_cast<unsigned>(decoded_target)
                                    << " decoded_type=" << static_cast<unsigned>(decoded_type));
                    INFO("StartSq=" << static_cast<unsigned>(Moves::StartSq(encoded))
                                     << " TargetSq=" << static_cast<unsigned>(Moves::TargetSq(encoded))
                                     << " reencoded=" << static_cast<unsigned>(reencoded)
                                     << " globally_unique=" << unique);
                    FAIL("valid move component product is not encoded injectively and reversibly");
                }
            }
        }
    }
    CHECK(ordinal == 32256);
    CHECK(encodings.size() == 32256);
}

TEST_CASE("T-MOV-002 Promotion predicate classifies every valid semantic move kind",
          "[T-MOV-002][primitives][deep]")
{
    for (unsigned start = 0; start < 64; ++start)
    {
        for (unsigned target = 0; target < 64; ++target)
        {
            if (start == target)
                continue;
            for (const MoveType type : kSemanticMoveTypes)
            {
                const Move move = Moves::EncodeMove(static_cast<Sq>(start),
                                                    static_cast<Sq>(target), type);
                const bool expected = is_promotion_kind(type);
                const bool actual = Moves::IsPromotionMove(move);
                if (actual != expected)
                {
                    INFO("start=" << start << " target=" << target);
                    INFO("type=" << move_type_name(type)
                                  << " type_value=" << static_cast<unsigned>(type)
                                  << " encoded=" << static_cast<unsigned>(move));
                    INFO("expected=" << expected << " actual=" << actual);
                    FAIL("IsPromotionMove disagrees with the four named promotion kinds");
                }
            }
        }
    }
}

TEST_CASE("T-MOV-004 Valid corpus UCI moves parse and render canonically",
          "[T-MOV-004][primitives][deep]")
{
    auto [table, cases] = load_oracle_cases();
    REQUIRE(table.metadata.at("data_sha256") == kCasesDataSha256);
    REQUIRE(blind::sha256_hex(table.scoped_bytes) == kCasesDataSha256);
    REQUIRE(cases.size() == 27);

    const std::map<std::string, MoveType> exact_types{
        {"start/g1f3", mt_Quiet},
        {"kiwipete/e2a6", mt_Capture},
        {"en_passant_legal/e5d6", mt_EnPassant},
        {"castling_minimal/e1c1", mt_Castling},
        {"castling_minimal/e1g1", mt_Castling},
        {"promotion_choices/a7a8q", mt_QueenPromotion},
        {"promotion_choices/a7a8r", mt_RookPromotion},
        {"promotion_choices/a7a8b", mt_BishopPromotion},
        {"promotion_choices/a7a8n", mt_KnightPromotion},
        {"perft_castling/d7c8q", mt_QueenPromotion},
        {"perft_castling/d7c8r", mt_RookPromotion},
        {"perft_castling/d7c8b", mt_BishopPromotion},
        {"perft_castling/d7c8n", mt_KnightPromotion},
    };
    std::set<std::string> observed_sentinels;

    for (const auto& entry : cases)
    {
        for (std::size_t token_index = 0; token_index < entry.legal_uci.size(); ++token_index)
        {
            const auto& token = entry.legal_uci[token_index];
            Position position(entry.fen);
            const Move move = UTIL::UciToMove(token, position);
            const std::string rendered = UTIL::MoveToStr(move);
            Sq start = 0;
            Sq target = 0;
            MoveType type = mt_Quiet;
            Moves::DecodeMove(move, &start, &target, &type);

            INFO("case_id=" << entry.case_id << " token_index=" << token_index);
            INFO("fen=" << entry.fen << " token=" << token
                         << " rendered=" << rendered
                         << " move_bits=" << static_cast<unsigned>(move));
            INFO("decoded_start=" << static_cast<unsigned>(start)
                                   << " decoded_target=" << static_cast<unsigned>(target)
                                   << " decoded_type=" << static_cast<unsigned>(type));
            REQUIRE((token.size() == 4 || token.size() == 5));
            CHECK(rendered == token);
            CHECK(Moves::IsPromotionMove(move) == (token.size() == 5));
            if (token.size() == 5)
                CHECK(rendered.back() == token.back());

            const std::string key = entry.case_id + "/" + token;
            if (const auto sentinel = exact_types.find(key); sentinel != exact_types.end())
            {
                observed_sentinels.insert(key);
                INFO("expected_type=" << move_type_name(sentinel->second)
                                      << " actual_type=" << move_type_name(type));
                CHECK(type == sentinel->second);
            }
        }
    }
    CHECK(observed_sentinels.size() == exact_types.size());
}

TEST_CASE("T-UTL-001 UCI square vocabulary and ordinary promotion characters are exact",
          "[T-UTL-001][primitives][fast]")
{
    std::set<std::string> expected_squares;
    for (char rank = '1'; rank <= '8'; ++rank)
        for (char file = 'a'; file <= 'h'; ++file)
            expected_squares.insert(std::string{file, rank});

    std::set<std::string> actual_squares;
    for (unsigned index = 0; index < 64; ++index)
    {
        const std::string square = UTIL::Square(static_cast<Sq>(index));
        INFO("Sq=" << index << " output=" << square);
        CHECK(expected_squares.contains(square));
        CHECK(actual_squares.insert(square).second);
    }
    CHECK(actual_squares == expected_squares);

    struct PromotionRow
    {
        PieceType type;
        char expected;
    };
    constexpr std::array<PromotionRow, 4> rows{
        PromotionRow{pt_Queen, 'q'},
        PromotionRow{pt_Rook, 'r'},
        PromotionRow{pt_Bishop, 'b'},
        PromotionRow{pt_Knight, 'n'},
    };
    for (const auto& row : rows)
    {
        const char actual = UTIL::PromotionChar(row.type);
        INFO("PieceType=" << static_cast<unsigned>(row.type)
                           << " expected=" << row.expected << " actual=" << actual);
        CHECK(actual == row.expected);
    }
}

TEST_CASE("T-BB-001 One-hot conversion exhausts the valid 64-square domain",
          "[T-BB-001][primitives][fast]")
{
    BitBoard union_of_bits = 0;
    for (int index = 0; index < 64; ++index)
    {
        const BitBoard expected = U64{1} << static_cast<unsigned>(index);
        const BitBoard runtime = Magics::SqToBB(static_cast<Sq>(index));
        const BitBoard compile_time = kConstevalSquareBits[static_cast<std::size_t>(index)];
        INFO("index=" << index << " expected=" << hex_u64(expected));
        INFO("runtime=" << hex_u64(runtime) << " consteval=" << hex_u64(compile_time));
        CHECK(runtime == expected);
        CHECK(compile_time == expected);
        CHECK(runtime == compile_time);
        CHECK(model_popcount(runtime) == 1);
        CHECK((union_of_bits & runtime) == 0);
        union_of_bits |= runtime;
    }
    CHECK(union_of_bits == std::numeric_limits<BitBoard>::max());

}

TEST_CASE("T-BB-002 Population count matches independent exhaustive and fixed-random models",
          "[T-BB-002][primitives][deep]")
{
    std::size_t ordinal = 0;
    for (unsigned offset : {0u, 16u, 32u, 48u})
    {
        for (std::uint64_t value = 0; value <= UINT64_C(65535); ++value)
        {
            require_popcount_match(value << offset, "16-bit slice", ordinal++);
        }
    }

    require_popcount_match(0, "boundary", 0);
    require_popcount_match(std::numeric_limits<BitBoard>::max(), "boundary", 1);
    for (unsigned first = 0; first < 64; ++first)
    {
        require_popcount_match(U64{1} << first, "singleton", first);
        for (unsigned second = first + 1; second < 64; ++second)
        {
            const BitBoard board = (U64{1} << first) | (U64{1} << second);
            require_popcount_match(board, "two-bit", first * 64u + second);
        }
    }

    SplitMix64 generator(UINT64_C(0x0000000054444641));
    for (std::size_t draw = 0; draw < 4096; ++draw)
        require_popcount_match(generator.next(), "SplitMix64", draw);

    SplitMix64 pairs(UINT64_C(0x0000000054444641));
    for (std::size_t draw = 0; draw < 4096; ++draw)
    {
        const BitBoard left = pairs.next();
        const BitBoard right = pairs.next() & ~left;
        const unsigned left_count = model_popcount(left);
        const unsigned right_count = model_popcount(right);
        const unsigned union_count = Magics::PopCnt(left | right);
        if (union_count != left_count + right_count)
        {
            INFO("draw=" << draw << " seed=0x0000000054444641");
            INFO("left=" << hex_u64(left) << " right=" << hex_u64(right));
            INFO("left_count=" << left_count << " right_count=" << right_count
                                << " union_count=" << union_count);
            FAIL("PopCnt violates additivity on disjoint operands");
        }
    }
}

TEST_CASE("T-BB-003 Nonzero bit scans and removals match an independent ordered set",
          "[T-BB-003][primitives][deep]")
{
    for (unsigned first = 0; first < 64; ++first)
    {
        require_scan_match(U64{1} << first, "singleton", first);
        for (unsigned second = first + 1; second < 64; ++second)
            require_scan_match((U64{1} << first) | (U64{1} << second),
                               "two-bit", first * 64u + second);
    }

    std::size_t ordinal = 0;
    for (unsigned offset : {0u, 16u, 32u, 48u})
        for (std::uint64_t value = 1; value <= UINT64_C(65535); ++value)
            require_scan_match(value << offset, "nonzero 16-bit slice", ordinal++);

    SplitMix64 generator(UINT64_C(0x0000000054444641));
    for (std::size_t draw = 0; draw < 4096; ++draw)
    {
        BitBoard board = generator.next();
        if (board == 0)
            board = U64{1};
        require_scan_match(board, "nonzero SplitMix64", draw);
    }
}

TEST_CASE("T-BB-005 Directional shifts satisfy orientation-free grid invariants",
          "[T-BB-005][primitives][deep]")
{
    for (std::size_t direction = 0; direction < kDirections.size(); ++direction)
    {
        const auto& spec = kDirections[direction];
        INFO("direction=" << spec.name);
        CHECK(spec.shift(0) == 0);

        unsigned discarded = 0;
        std::set<BitBoard> images;
        for (unsigned square = 0; square < 64; ++square)
        {
            const BitBoard input = U64{1} << square;
            const BitBoard output = spec.shift(input);
            const unsigned output_count = model_popcount(output);
            INFO("square=" << square << " input=" << hex_u64(input)
                            << " output=" << hex_u64(output));
            REQUIRE((output_count == 0 || output_count == 1));
            if (output == 0)
            {
                ++discarded;
                continue;
            }
            CHECK(images.insert(output).second);
            if (direction < 8)
            {
                const auto& opposite = kDirections[spec.opposite];
                INFO("opposite=" << opposite.name
                                  << " restored=" << hex_u64(opposite.shift(output)));
                CHECK(opposite.shift(output) == input);
            }
        }
        CHECK(discarded == spec.discarded_singletons);
        CHECK(images.size() == 64u - spec.discarded_singletons);
    }

    const auto check_distributive = [](BitBoard left, BitBoard right,
                                       std::string_view corpus, std::size_t ordinal) {
        for (const auto& spec : kDirections)
        {
            const BitBoard combined = spec.shift(left | right);
            const BitBoard separate = spec.shift(left) | spec.shift(right);
            if (combined != separate)
            {
                INFO("corpus=" << corpus << " ordinal=" << ordinal
                                << " direction=" << spec.name);
                INFO("left=" << hex_u64(left) << " right=" << hex_u64(right));
                INFO("combined=" << hex_u64(combined) << " separate=" << hex_u64(separate));
                FAIL("Shift does not distribute over bitwise union");
            }
        }
    };

    std::size_t pair_ordinal = 0;
    for (unsigned first = 0; first < 64; ++first)
        for (unsigned second = first + 1; second < 64; ++second)
            check_distributive(U64{1} << first, U64{1} << second,
                               "all two-bit boards", pair_ordinal++);

    SplitMix64 generator(UINT64_C(0x0000000054444641));
    for (std::size_t draw = 0; draw < 4096; ++draw)
        check_distributive(generator.next(), generator.next(), "SplitMix64 pairs", draw);

    std::vector<BitBoard> composition_corpus{0};
    composition_corpus.reserve(1 + 64 + 2016 + 4096);
    for (unsigned square = 0; square < 64; ++square)
        composition_corpus.push_back(U64{1} << square);
    for (unsigned first = 0; first < 64; ++first)
        for (unsigned second = first + 1; second < 64; ++second)
            composition_corpus.push_back((U64{1} << first) | (U64{1} << second));
    SplitMix64 composition_generator(UINT64_C(0x0000000054444641));
    for (std::size_t draw = 0; draw < 4096; ++draw)
        composition_corpus.push_back(composition_generator.next());

    for (std::size_t ordinal = 0; ordinal < composition_corpus.size(); ++ordinal)
    {
        const BitBoard board = composition_corpus[ordinal];
        INFO("composition_ordinal=" << ordinal << " board=" << hex_u64(board));
        CHECK(Magics::Shift<NORTHNORTH>(board) ==
              Magics::Shift<NORTH>(Magics::Shift<NORTH>(board)));
        CHECK(Magics::Shift<SOUTHSOUTH>(board) ==
              Magics::Shift<SOUTH>(Magics::Shift<SOUTH>(board)));
    }

    struct DiagonalComposition
    {
        const char* name;
        ShiftFunction first_a;
        ShiftFunction second_a;
        ShiftFunction first_b;
        ShiftFunction second_b;
        ShiftFunction diagonal;
    };
    constexpr std::array<DiagonalComposition, 4> compositions{
        DiagonalComposition{"NORTH_EAST", invoke_shift<EAST>, invoke_shift<NORTH>,
                            invoke_shift<NORTH>, invoke_shift<EAST>, invoke_shift<NORTH_EAST>},
        DiagonalComposition{"SOUTH_EAST", invoke_shift<EAST>, invoke_shift<SOUTH>,
                            invoke_shift<SOUTH>, invoke_shift<EAST>, invoke_shift<SOUTH_EAST>},
        DiagonalComposition{"SOUTH_WEST", invoke_shift<WEST>, invoke_shift<SOUTH>,
                            invoke_shift<SOUTH>, invoke_shift<WEST>, invoke_shift<SOUTH_WEST>},
        DiagonalComposition{"NORTH_WEST", invoke_shift<WEST>, invoke_shift<NORTH>,
                            invoke_shift<NORTH>, invoke_shift<WEST>, invoke_shift<NORTH_WEST>},
    };
    for (const auto& composition : compositions)
    {
        for (unsigned square = 0; square < 64; ++square)
        {
            const BitBoard input = U64{1} << square;
            const BitBoard intermediate_a = composition.first_a(input);
            const BitBoard intermediate_b = composition.first_b(input);
            const BitBoard path_a = composition.second_a(intermediate_a);
            const BitBoard path_b = composition.second_b(intermediate_b);
            if (intermediate_a != 0 && intermediate_b != 0 && path_a != 0 && path_b != 0)
            {
                const BitBoard diagonal = composition.diagonal(input);
                INFO("diagonal=" << composition.name << " square=" << square);
                INFO("path_a=" << hex_u64(path_a) << " path_b=" << hex_u64(path_b)
                                << " direct=" << hex_u64(diagonal));
                CHECK(path_a == path_b);
                CHECK(path_a == diagonal);
            }
        }
    }
}

TEST_CASE("T-BB-006 File and rank classifiers form orthogonal opaque partitions",
          "[T-BB-006][primitives][fast]")
{
    std::array<unsigned, 64> file{};
    std::array<unsigned, 64> rank{};
    std::array<unsigned, 64> bit_file{};
    std::array<unsigned, 64> bit_rank{};
    std::map<unsigned, unsigned> file_histogram;
    std::map<unsigned, unsigned> rank_histogram;
    std::map<unsigned, unsigned> bit_file_histogram;
    std::map<unsigned, unsigned> bit_rank_histogram;
    std::set<std::pair<unsigned, unsigned>> coordinate_pairs;

    for (unsigned square = 0; square < 64; ++square)
    {
        file[square] = static_cast<unsigned>(Magics::FileOf(static_cast<U8>(square)));
        rank[square] = static_cast<unsigned>(Magics::RankOf(static_cast<Sq>(square)));
        bit_file[square] = static_cast<unsigned>(Magics::BBFileOf(static_cast<Sq>(square)));
        bit_rank[square] = static_cast<unsigned>(Magics::BBRankOf(static_cast<Sq>(square)));
        ++file_histogram[file[square]];
        ++rank_histogram[rank[square]];
        ++bit_file_histogram[bit_file[square]];
        ++bit_rank_histogram[bit_rank[square]];
        coordinate_pairs.emplace(file[square], rank[square]);
        INFO("square=" << square << " file_label=" << file[square]
                        << " rank_label=" << rank[square]
                        << " bit_file_label=" << bit_file[square]
                        << " bit_rank_label=" << bit_rank[square]);
        CHECK(bit_file[square] != 0);
        CHECK(bit_rank[square] != 0);
        CHECK(model_popcount(bit_file[square]) == 1);
        CHECK(model_popcount(bit_rank[square]) == 1);
    }

    REQUIRE(file_histogram.size() == 8);
    REQUIRE(rank_histogram.size() == 8);
    REQUIRE(bit_file_histogram.size() == 8);
    REQUIRE(bit_rank_histogram.size() == 8);
    for (const auto& [label, count] : file_histogram)
    {
        INFO("opaque file label=" << label);
        CHECK(count == 8);
    }
    for (const auto& [label, count] : rank_histogram)
    {
        INFO("opaque rank label=" << label);
        CHECK(count == 8);
    }
    for (const auto& [label, count] : bit_file_histogram)
    {
        INFO("opaque bit-file label=" << label);
        CHECK(count == 8);
    }
    for (const auto& [label, count] : bit_rank_histogram)
    {
        INFO("opaque bit-rank label=" << label);
        CHECK(count == 8);
    }
    CHECK(coordinate_pairs.size() == 64);

    for (unsigned left = 0; left < 64; ++left)
    {
        for (unsigned right = 0; right < 64; ++right)
        {
            INFO("left=" << left << " right=" << right);
            CHECK((file[left] == file[right]) == (bit_file[left] == bit_file[right]));
            CHECK((rank[left] == rank[right]) == (bit_rank[left] == bit_rank[right]));
        }
    }
}

TEST_CASE("T-BB-009 Integer-exponent power matches a frozen exact binary64 table",
          "[T-BB-009][primitives][fast]")
{
    struct PowerRow
    {
        double base;
        std::array<double, 13> powers;
        bool exponent_zero_is_defined;
    };
    constexpr std::array<PowerRow, 9> rows{
        PowerRow{-4.0, {1.0, -4.0, 16.0, -64.0, 256.0, -1024.0, 4096.0,
                        -16384.0, 65536.0, -262144.0, 1048576.0, -4194304.0,
                        16777216.0}, true},
        PowerRow{-2.0, {1.0, -2.0, 4.0, -8.0, 16.0, -32.0, 64.0,
                        -128.0, 256.0, -512.0, 1024.0, -2048.0, 4096.0}, true},
        PowerRow{-1.0, {1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0,
                        -1.0, 1.0, -1.0, 1.0, -1.0, 1.0}, true},
        PowerRow{-0.5, {1.0, -0.5, 0.25, -0.125, 0.0625, -0.03125, 0.015625,
                        -0.0078125, 0.00390625, -0.001953125, 0.0009765625,
                        -0.00048828125, 0.000244140625}, true},
        PowerRow{0.0, {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                       0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, false},
        PowerRow{0.5, {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625,
                       0.0078125, 0.00390625, 0.001953125, 0.0009765625,
                       0.00048828125, 0.000244140625}, true},
        PowerRow{1.0, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                       1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, true},
        PowerRow{2.0, {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0,
                       128.0, 256.0, 512.0, 1024.0, 2048.0, 4096.0}, true},
        PowerRow{4.0, {1.0, 4.0, 16.0, 64.0, 256.0, 1024.0, 4096.0,
                       16384.0, 65536.0, 262144.0, 1048576.0, 4194304.0,
                       16777216.0}, true},
    };

    for (const auto& row : rows)
    {
        const unsigned first_exponent = row.exponent_zero_is_defined ? 0u : 1u;
        for (unsigned exponent = first_exponent; exponent <= 12; ++exponent)
        {
            const double actual = Magics::pow(row.base, exponent);
            const double expected = row.powers[exponent];
            INFO("base=" << row.base << " base_bits=" << std::bit_cast<std::uint64_t>(row.base));
            INFO("exponent=" << exponent << " expected=" << expected << " actual=" << actual);
            INFO("expected_bits=" << std::bit_cast<std::uint64_t>(expected)
                                   << " actual_bits=" << std::bit_cast<std::uint64_t>(actual));
            CHECK(std::bit_cast<std::uint64_t>(actual) == std::bit_cast<std::uint64_t>(expected));
            if (row.base < 0.0)
                CHECK(std::signbit(actual) == ((exponent % 2u) != 0));
        }

        if (row.base == 0.0)
            continue;
        CHECK(Magics::pow(row.base, 0) == 1.0);
        CHECK(Magics::pow(row.base, 1) == row.base);
        for (unsigned left_exponent = 0; left_exponent <= 12; ++left_exponent)
        {
            for (unsigned right_exponent = 0;
                 left_exponent + right_exponent <= 12; ++right_exponent)
            {
                const double combined = Magics::pow(row.base, left_exponent + right_exponent);
                const double split = Magics::pow(row.base, left_exponent) *
                                     Magics::pow(row.base, right_exponent);
                INFO("base=" << row.base << " exponent_split=" << left_exponent
                              << "+" << right_exponent);
                CHECK(std::bit_cast<std::uint64_t>(combined) ==
                      std::bit_cast<std::uint64_t>(split));
            }
        }
    }
}

TEST_CASE("T-LEX-001 IsDigit recognizes exactly the ASCII decimal alphabet",
          "[T-LEX-001][primitives][fast]")
{
    for (unsigned byte = 0; byte <= 0x7fu; ++byte)
    {
        const bool expected = byte >= 0x30u && byte <= 0x39u;
        const bool actual = IsDigit(static_cast<char>(byte));
        INFO("byte=0x" << std::hex << byte << std::dec
                       << " expected=" << expected << " actual=" << actual);
        INFO("host_char_is_signed=" << std::numeric_limits<char>::is_signed);
        CHECK(actual == expected);
    }
}

TEST_CASE("T-LEX-002 SplitFen returns six exact fields for every canonical corpus FEN",
          "[T-LEX-002][primitives][fast]")
{
    auto [case_table, cases] = load_oracle_cases();
    auto [perft_table, perft_fens] = load_oracle_perft_fens();
    REQUIRE(case_table.metadata.at("data_sha256") == kCasesDataSha256);
    REQUIRE(perft_table.metadata.at("data_sha256") == kPerftDataSha256);
    REQUIRE(blind::sha256_hex(case_table.scoped_bytes) == kCasesDataSha256);
    REQUIRE(blind::sha256_hex(perft_table.scoped_bytes) == kPerftDataSha256);
    REQUIRE(cases.size() == 27);
    REQUIRE(perft_fens.size() == 18);

    std::set<std::string> unique_fens;
    for (const auto& entry : cases)
    {
        unique_fens.insert(entry.fen);
        unique_fens.insert(entry.root_fen);
    }
    unique_fens.insert(perft_fens.begin(), perft_fens.end());

    constexpr std::string_view sentinel = "SENTINEL-NOT-A-FEN-FIELD";
    for (const auto& source : unique_fens)
    {
        const std::string source_before = source;
        const auto expected = split_fen_independently(source);
        std::array<std::string_view, 6> actual;
        actual.fill(sentinel);
        SplitFen(source, actual);

        INFO("source=" << source);
        std::size_t expected_offset = 0;
        for (std::size_t field = 0; field < actual.size(); ++field)
        {
            INFO("field=" << field << " expected=" << expected[field]
                           << " actual=" << actual[field]);
            CHECK(actual[field] == expected[field]);
            CHECK(actual[field] != sentinel);
            CHECK(actual[field].data() == source.data() + expected_offset);
            expected_offset += expected[field].size() + (field + 1 < actual.size() ? 1u : 0u);
        }
        CHECK(source == source_before);
        for (std::size_t field = 0; field < actual.size(); ++field)
            CHECK(actual[field] == expected[field]);
    }
}

TEST_CASE("T-BRD-001 Board construction and reset produce the exhaustive empty placement",
          "[T-BRD-001][primitives][fast]")
{
    const BoardModel empty = empty_board_model();
    const auto check_empty = [&](const Board& board, std::string_view phase) {
        std::string reason;
        INFO("phase=" << phase);
        INFO("white_mask=" << hex_u64(board.Pieces(White))
                            << " black_mask=" << hex_u64(board.Pieces(Black)));
        if (!board_matches(board, empty, reason))
        {
            INFO("reason=" << reason);
            FAIL("Board does not have the exhaustive empty placement");
        }
    };

    Board fresh;
    check_empty(fresh, "fresh construction");
    fresh.ResetBoard();
    check_empty(fresh, "fresh first reset");
    fresh.ResetBoard();
    check_empty(fresh, "fresh second reset");

    Board populated;
    constexpr std::array<std::pair<Piece, unsigned>, 12> entries{
        std::pair{p_WhiteKing, 0u}, std::pair{p_WhiteQueen, 1u},
        std::pair{p_WhiteBishop, 2u}, std::pair{p_WhiteKnight, 3u},
        std::pair{p_WhiteRook, 4u}, std::pair{p_WhitePawn, 5u},
        std::pair{p_BlackKing, 58u}, std::pair{p_BlackQueen, 59u},
        std::pair{p_BlackBishop, 60u}, std::pair{p_BlackKnight, 61u},
        std::pair{p_BlackRook, 62u}, std::pair{p_BlackPawn, 63u},
    };
    for (const auto [piece, square] : entries)
        populated.AddPiece(piece, static_cast<Sq>(square));
    populated.ResetBoard();
    check_empty(populated, "populated first reset");
    populated.ResetBoard();
    check_empty(populated, "populated second reset");
}

TEST_CASE("T-BRD-002 Piece construction and decoding are exhaustive inverses",
          "[T-BRD-002][primitives][fast]")
{
    std::set<Piece> constructed_pieces;
    for (const Colour colour : kColours)
    {
        for (const PieceType type : kOrdinaryPieceTypes)
        {
            const Piece piece = Board::MakePiece(colour, type);
            const PieceType decoded_type = Magics::TypeOf(piece);
            const Colour decoded_colour = Magics::ColourOf(piece);
            const Piece reconstructed = Board::MakePiece(decoded_colour, decoded_type);
            INFO("colour=" << static_cast<unsigned>(colour)
                            << " type=" << static_cast<unsigned>(type));
            INFO("piece=" << static_cast<unsigned>(piece)
                            << " decoded_colour=" << static_cast<unsigned>(decoded_colour)
                            << " decoded_type=" << static_cast<unsigned>(decoded_type));
            CHECK(decoded_type == type);
            CHECK(decoded_colour == colour);
            CHECK(reconstructed == piece);
            CHECK(constructed_pieces.insert(piece).second);
        }
    }
    CHECK(constructed_pieces.size() == 12);
}

TEST_CASE("T-BRD-003 Single-piece add and queries exhaust all pieces and squares",
          "[T-BRD-003][primitives][deep]")
{
    for (const Piece piece : kPieces)
    {
        const int selected_colour = model_colour_index(piece);
        const int selected_type = model_type_index(piece);
        REQUIRE(selected_colour >= 0);
        REQUIRE(selected_type >= 0);
        for (unsigned square = 0; square < 64; ++square)
        {
            Board board;
            BoardModel expected = empty_board_model();
            board.AddPiece(piece, static_cast<Sq>(square));
            expected[square] = piece;
            std::string reason;
            INFO("piece=" << static_cast<unsigned>(piece) << " square=" << square);
            if (!board_matches(board, expected, reason))
            {
                INFO("reason=" << reason);
                FAIL("single-piece Board state disagrees with the literal model");
            }

            const BitBoard singleton = U64{1} << square;
            for (std::size_t colour = 0; colour < kColours.size(); ++colour)
            {
                for (std::size_t type = 0; type < kOrdinaryPieceTypes.size(); ++type)
                {
                    const BitBoard expected_pair =
                        static_cast<int>(colour) == selected_colour &&
                                static_cast<int>(type) == selected_type
                            ? singleton
                            : 0;
                    INFO("query_colour_index=" << colour << " query_type_index=" << type
                                                << " expected_pair=" << hex_u64(expected_pair));
                    CHECK(board.Pieces(kColours[colour], kOrdinaryPieceTypes[type]) == expected_pair);
                }
            }
        }
    }
}

TEST_CASE("T-BRD-004 Aggregate Board queries match exhaustive set algebra",
          "[T-BRD-004][primitives][deep]")
{
    const BoardModel model = aggregate_fixture_model();
    Board board;
    populate_board(board, model);
    std::string reason;
    REQUIRE(board_matches(board, model, reason));

    check_all_aggregate_subsets(board, model, std::make_index_sequence<63>{});

    const BitBoard all_occupied = model_board_mask(model, -1, -1);
    INFO("all_occupied=" << hex_u64(all_occupied));
    CHECK(board.Pieces(White, Black) == all_occupied);
    CHECK(board.Pieces(Black, White) == all_occupied);
}

TEST_CASE("T-BRD-005 Primitive Board transitions exhaust valid sources and destinations",
          "[T-BRD-005][primitives][deep]")
{
    const auto require_state = [](const Board& board, const BoardModel& expected,
                                  std::string_view operation, Piece piece,
                                  unsigned from, unsigned to, unsigned replay) {
        std::string reason;
        if (!board_matches(board, expected, reason))
        {
            INFO("operation=" << operation << " replay=" << replay);
            INFO("piece=" << static_cast<unsigned>(piece)
                           << " from=" << from << " to=" << to);
            INFO("reason=" << reason);
            FAIL("primitive Board transition disagrees with the finite-map model");
        }
    };

    for (const Piece piece : kPieces)
    {
        for (unsigned square = 0; square < 64; ++square)
        {
            Board remove_board;
            for (unsigned replay = 0; replay < 2; ++replay)
            {
                if (replay != 0)
                    remove_board.ResetBoard();
                BoardModel model = empty_board_model();
                remove_board.AddPiece(piece, static_cast<Sq>(square));
                model[square] = piece;
                require_state(remove_board, model, "remove pre-state", piece, square, square, replay);
                remove_board.RemovePiece(static_cast<Sq>(square));
                model[square] = p_None;
                require_state(remove_board, model, "remove post-state", piece, square, square, replay);
            }

            Board pop_board;
            for (unsigned replay = 0; replay < 2; ++replay)
            {
                if (replay != 0)
                    pop_board.ResetBoard();
                BoardModel model = empty_board_model();
                pop_board.AddPiece(piece, static_cast<Sq>(square));
                model[square] = piece;
                require_state(pop_board, model, "pop pre-state", piece, square, square, replay);
                const Piece returned = pop_board.PopPiece(static_cast<Sq>(square));
                INFO("operation=pop replay=" << replay << " piece=" << static_cast<unsigned>(piece)
                                              << " square=" << square
                                              << " returned=" << static_cast<unsigned>(returned));
                CHECK(returned == piece);
                model[square] = p_None;
                require_state(pop_board, model, "pop post-state", piece, square, square, replay);
            }

            for (unsigned target = 0; target < 64; ++target)
            {
                if (target == square)
                    continue;
                Board move_board;
                for (unsigned replay = 0; replay < 2; ++replay)
                {
                    if (replay != 0)
                        move_board.ResetBoard();
                    BoardModel model = empty_board_model();
                    move_board.AddPiece(piece, static_cast<Sq>(square));
                    model[square] = piece;
                    require_state(move_board, model, "move pre-state", piece, square, target, replay);
                    move_board.MovePiece(static_cast<Sq>(square), static_cast<Sq>(target));
                    model[square] = p_None;
                    model[target] = piece;
                    require_state(move_board, model, "move post-state", piece, square, target, replay);
                }
            }
        }
    }
}

TEST_CASE("T-BRD-007 Mixed multi-piece Board trace preserves unrelated state",
          "[T-BRD-007][primitives][fast]")
{
    Board board;
    const auto check_state = [&](const BoardModel& model, unsigned replay,
                                 std::string_view step) {
        std::string reason;
        if (!board_matches(board, model, reason))
        {
            INFO("replay=" << replay << " step=" << step << " reason=" << reason);
            FAIL("mixed Board trace diverged from the literal finite-map model");
        }
    };

    constexpr std::array<std::pair<Piece, unsigned>, 8> fixture{
        std::pair{p_WhiteKing, 0u}, std::pair{p_WhiteQueen, 9u},
        std::pair{p_WhiteRook, 18u}, std::pair{p_WhitePawn, 27u},
        std::pair{p_BlackKing, 63u}, std::pair{p_BlackQueen, 54u},
        std::pair{p_BlackRook, 45u}, std::pair{p_BlackPawn, 36u},
    };

    for (unsigned replay = 0; replay < 2; ++replay)
    {
        BoardModel model = empty_board_model();
        check_state(model, replay, "empty start");
        for (std::size_t index = 0; index < fixture.size(); ++index)
        {
            const auto [piece, square] = fixture[index];
            board.AddPiece(piece, static_cast<Sq>(square));
            model[square] = piece;
            check_state(model, replay, "fixture add " + std::to_string(index));
        }

        board.MovePiece(static_cast<Sq>(9), static_cast<Sq>(10));
        model[10] = model[9];
        model[9] = p_None;
        check_state(model, replay, "MovePiece(9,10)");

        const Piece popped = board.PopPiece(static_cast<Sq>(36));
        INFO("replay=" << replay << " PopPiece(36) returned=" << static_cast<unsigned>(popped));
        CHECK(popped == p_BlackPawn);
        model[36] = p_None;
        check_state(model, replay, "PopPiece(36)");

        board.AddPiece(p_WhiteKnight, static_cast<Sq>(36));
        model[36] = p_WhiteKnight;
        check_state(model, replay, "AddPiece(p_WhiteKnight,36)");

        board.RemovePiece(static_cast<Sq>(45));
        model[45] = p_None;
        check_state(model, replay, "RemovePiece(45)");

        board.MovePiece(static_cast<Sq>(54), static_cast<Sq>(45));
        model[45] = model[54];
        model[54] = p_None;
        check_state(model, replay, "MovePiece(54,45)");

        board.ResetBoard();
        model = empty_board_model();
        check_state(model, replay, "ResetBoard");
    }
}
