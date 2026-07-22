#ifndef TDFA_TESTS_POSITION_TEST_PROBE_HPP
#define TDFA_TESTS_POSITION_TEST_PROBE_HPP

#if !defined(TDFA_TESTING)
#error "PositionTestProbe is available only in TDFA_TESTING builds"
#endif

#include "Position.hpp"

#include <array>
#include <cstddef>
#include <vector>

struct PositionSnapshot
{
    std::array<Piece, 64> squares{};
    std::array<BitBoard, 2> colours{};
    std::array<BitBoard, 7> types{};
    StateInfo state{};
    Colour turn{White};
    U16 full_moves{0};
    std::vector<StateInfo> history{};
};

struct PositionTestProbe
{
    [[nodiscard]] static StateInfo& State(Position& position) noexcept
    {
        return position.info_;
    }

    [[nodiscard]] static StateInfo const& State(Position const& position) noexcept
    {
        return position.info_;
    }

    [[nodiscard]] static Colour& Turn(Position& position) noexcept
    {
        return position.turn_;
    }

    [[nodiscard]] static U16& FullMoves(Position& position) noexcept
    {
        return position.full_moves_;
    }

    [[nodiscard]] static std::vector<StateInfo>& History(Position& position) noexcept
    {
        return position.previous_state_info;
    }

    [[nodiscard]] static std::vector<StateInfo> const& History(Position const& position) noexcept
    {
        return position.previous_state_info;
    }

    [[nodiscard]] static bool Equal(StateInfo const& lhs, StateInfo const& rhs) noexcept
    {
        return lhs.castling_rights_ == rhs.castling_rights_
            && lhs.half_moves_ == rhs.half_moves_
            && lhs.en_passant_sq_ == rhs.en_passant_sq_
            && lhs.captured_type_ == rhs.captured_type_
            && lhs.zobrist_key_ == rhs.zobrist_key_;
    }

    [[nodiscard]] static PositionSnapshot Snapshot(Position const& position)
    {
        PositionSnapshot snapshot;
        for (std::size_t square = 0; square < snapshot.squares.size(); ++square)
            snapshot.squares[square] = position.PieceOn(static_cast<Sq>(square));

        snapshot.colours[White] = position.Pieces(White);
        snapshot.colours[Black] = position.Pieces(Black);
        for (std::size_t piece_type = 0; piece_type < snapshot.types.size(); ++piece_type)
            snapshot.types[piece_type] = position.Pieces(static_cast<PieceType>(piece_type));

        snapshot.state = position.info_;
        snapshot.turn = position.turn_;
        snapshot.full_moves = position.full_moves_;
        snapshot.history = position.previous_state_info;
        return snapshot;
    }

    [[nodiscard]] static bool Equal(PositionSnapshot const& lhs, PositionSnapshot const& rhs)
    {
        if (lhs.squares != rhs.squares
            || lhs.colours != rhs.colours
            || lhs.types != rhs.types
            || !Equal(lhs.state, rhs.state)
            || lhs.turn != rhs.turn
            || lhs.full_moves != rhs.full_moves
            || lhs.history.size() != rhs.history.size())
            return false;

        for (std::size_t index = 0; index < lhs.history.size(); ++index)
            if (!Equal(lhs.history[index], rhs.history[index]))
                return false;

        return true;
    }

    [[nodiscard]] static ZobristKey IndependentlyComputedHash(Position const& position)
    {
        ZobristKey key = position.ColourToMove() == White ? Zobrist::SIDE_TO_MOVE : 0;
        for (std::size_t square = 0; square < 64; ++square)
        {
            const Piece piece = position.PieceOn(static_cast<Sq>(square));
            if (piece == p_None)
                continue;

            key ^= Zobrist::PIECES[Magics::ColourOf(piece)][Magics::TypeOf(piece)][square];
        }

        if (position.EnPasSq() != Magics::EP_NULL)
            key ^= Zobrist::EN_PASSANT[position.EnPasSq()];
        key ^= Zobrist::CASTLING[position.CastlingRights()];
        return key;
    }

    [[nodiscard]] static bool PublicBoardInvariantsHold(Position const& position)
    {
        const BitBoard white = position.Pieces(White);
        const BitBoard black = position.Pieces(Black);
        if ((white & black) != 0 || (white | black) != position.Pieces(White, Black))
            return false;

        std::array<BitBoard, 2> observed_colours{};
        std::array<BitBoard, 6> observed_types{};
        for (std::size_t square = 0; square < 64; ++square)
        {
            const Piece piece = position.PieceOn(static_cast<Sq>(square));
            if (piece == p_None)
                continue;

            const BitBoard square_bb = Magics::SqToBB(static_cast<Sq>(square));
            const Colour colour = Magics::ColourOf(piece);
            const PieceType type = Magics::TypeOf(piece);
            if (type > pt_Pawn)
                return false;
            observed_colours[colour] |= square_bb;
            observed_types[type] |= square_bb;
        }

        if (observed_colours[White] != white || observed_colours[Black] != black)
            return false;
        for (std::size_t type = 0; type < observed_types.size(); ++type)
            if (observed_types[type] != position.Pieces(static_cast<PieceType>(type)))
                return false;

        return position.Pieces(pt_All) == (white | black);
    }
};

#endif
