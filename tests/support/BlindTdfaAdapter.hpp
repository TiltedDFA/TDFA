#pragma once

#include "BlindChessModel.hpp"

#include "Board.hpp"
#include "MagicConstants.hpp"
#include "Move.hpp"
#include "MoveGen.hpp"
#include "MoveList.hpp"
#include "Position.hpp"
#include "Types.hpp"
#include "Util.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tdfa_test::blind {

class SquareBijection {
public:
    SquareBijection() {
        std::set<std::string> seen;
        for (int value = 0; value < 64; ++value) {
            const auto square = static_cast<Sq>(value);
            const std::string label = UTIL::Square(square);
            if (label.size() != 2 || label[0] < 'a' || label[0] > 'h' ||
                label[1] < '1' || label[1] > '8' || !seen.insert(label).second) {
                throw std::runtime_error("UTIL::Square is not the contracted 64-label bijection");
            }
            labels_[static_cast<std::size_t>(value)] = label;
            by_label_.emplace(label, square);
        }
        if (seen.size() != 64) throw std::runtime_error("incomplete square-label bijection");
    }

    [[nodiscard]] const std::string& label(const Sq square) const {
        const auto value = static_cast<unsigned>(square);
        if (value >= 64) throw std::runtime_error("production returned out-of-domain Sq");
        return labels_[value];
    }

    [[nodiscard]] Sq square(const std::string_view label) const {
        const auto found = by_label_.find(std::string(label));
        if (found == by_label_.end()) throw std::runtime_error("unknown canonical square label");
        return found->second;
    }

    [[nodiscard]] BitBoard bitboard(const std::set<std::string>& labels) const {
        BitBoard result = 0;
        for (const auto& label_text : labels) {
            const auto index = static_cast<unsigned>(square(label_text));
            result |= (BitBoard{1} << index);
        }
        return result;
    }

    [[nodiscard]] std::set<std::string> labels(const BitBoard board) const {
        std::set<std::string> result;
        for (unsigned index = 0; index < 64; ++index) {
            if ((board & (BitBoard{1} << index)) != 0) {
                result.insert(labels_[index]);
            }
        }
        return result;
    }

private:
    std::array<std::string, 64> labels_{};
    std::map<std::string, Sq> by_label_;
};

inline Colour production_colour(const Side side) {
    return side == Side::white ? White : Black;
}

inline Side oracle_side(const Colour colour) {
    if (colour == White) return Side::white;
    if (colour == Black) return Side::black;
    throw std::runtime_error("production returned invalid Colour");
}

inline Piece named_piece(const char piece) {
    switch (piece) {
        case '.': return p_None;
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
        default: throw std::runtime_error("invalid test-oracle piece character");
    }
}

inline char promotion_suffix(const MoveType type) {
    if (type == mt_QueenPromotion) return 'q';
    if (type == mt_RookPromotion) return 'r';
    if (type == mt_BishopPromotion) return 'b';
    if (type == mt_KnightPromotion) return 'n';
    return 0;
}

inline bool semantic_move_type(const MoveType type) {
    return type == mt_Quiet || type == mt_EnPassant || type == mt_Castling ||
           type == mt_Capture || type == mt_QueenPromotion ||
           type == mt_RookPromotion || type == mt_BishopPromotion ||
           type == mt_KnightPromotion;
}

inline std::string move_type_name(const MoveType type) {
    if (type == mt_Quiet) return "mt_Quiet";
    if (type == mt_EnPassant) return "mt_EnPassant";
    if (type == mt_Castling) return "mt_Castling";
    if (type == mt_Capture) return "mt_Capture";
    if (type == mt_QueenPromotion) return "mt_QueenPromotion";
    if (type == mt_RookPromotion) return "mt_RookPromotion";
    if (type == mt_BishopPromotion) return "mt_BishopPromotion";
    if (type == mt_KnightPromotion) return "mt_KnightPromotion";
    return "nonsemantic MoveType";
}

struct ObservedMove {
    Move raw{};
    Sq from{};
    Sq to{};
    MoveType type{};
    std::string uci;
};

inline ObservedMove observe_move(const Move move, const SquareBijection& squares) {
    Sq from{};
    Sq to{};
    MoveType type{};
    Moves::DecodeMove(move, &from, &to, &type);
    if (!semantic_move_type(type)) throw std::runtime_error("generator emitted nonsemantic MoveType");
    std::string uci = squares.label(from) + squares.label(to);
    if (const char suffix = promotion_suffix(type); suffix != 0) uci.push_back(suffix);
    return {move, from, to, type, std::move(uci)};
}

inline std::vector<ObservedMove> observe_moves(MoveList& list,
                                               const SquareBijection& squares) {
    const std::size_t capacity = list.all().size();
    if (list.len() > capacity)
        throw std::runtime_error("MoveList length exceeds observable backing capacity");
    std::vector<ObservedMove> result;
    result.reserve(list.len());
    for (std::size_t index = 0; index < list.len(); ++index) {
        result.push_back(observe_move(list[index], squares));
    }
    return result;
}

inline std::set<std::string> observed_uci_set(MoveList& list,
                                              const SquareBijection& squares) {
    std::set<std::string> result;
    for (const auto& move : observe_moves(list, squares)) result.insert(move.uci);
    return result;
}

inline std::string print_set(const std::set<std::string>& values) {
    std::ostringstream out;
    bool first = true;
    for (const auto& value : values) {
        if (!first) out << ' ';
        first = false;
        out << value;
    }
    return out.str();
}

inline void generate_legal(Position& position, const Side side, MoveList& list) {
    if (side == Side::white) MoveGen::GenerateLegalMoves<White>(&position, &list);
    else MoveGen::GenerateLegalMoves<Black>(&position, &list);
}

inline void generate_pseudo(const Position& position, const Side side, MoveList& list) {
    if (side == Side::white) MoveGen::GeneratePseudoLegalMoves<White>(&position, &list);
    else MoveGen::GeneratePseudoLegalMoves<Black>(&position, &list);
}

inline void generate_castling(const Position& position, const Side side, MoveList& list) {
    if (side == Side::white) MoveGen::Castling<White>(&position, &list);
    else MoveGen::Castling<Black>(&position, &list);
}

inline void generate_piece(const Position& position, const Side side, const char kind,
                           MoveList& list) {
    if (side == Side::white) {
        if (kind == 'p') MoveGen::WhitePawnMoves(&position, &list);
        else if (kind == 'b') MoveGen::BishopMoves<White>(&position, &list);
        else if (kind == 'r') MoveGen::RookMoves<White>(&position, &list);
        else if (kind == 'q') MoveGen::QueenMoves<White>(&position, &list);
        else if (kind == 'n') MoveGen::KnightMoves<White>(&position, &list);
        else if (kind == 'k') MoveGen::KingMoves<White>(&position, &list);
        else throw std::runtime_error("unsupported piece generator kind");
    } else {
        if (kind == 'p') MoveGen::BlackPawnMoves(&position, &list);
        else if (kind == 'b') MoveGen::BishopMoves<Black>(&position, &list);
        else if (kind == 'r') MoveGen::RookMoves<Black>(&position, &list);
        else if (kind == 'q') MoveGen::QueenMoves<Black>(&position, &list);
        else if (kind == 'n') MoveGen::KnightMoves<Black>(&position, &list);
        else if (kind == 'k') MoveGen::KingMoves<Black>(&position, &list);
        else throw std::runtime_error("unsupported piece generator kind");
    }
}

inline BitBoard generate_piece_attacks(const Position& position, const Side side,
                                       const char kind) {
    if (side == Side::white) {
        if (kind == 'p') return MoveGen::PawnAttacks<White>(&position);
        if (kind == 'b') return MoveGen::BishopAttacks<White>(&position);
        if (kind == 'r') return MoveGen::RookAttacks<White>(&position);
        if (kind == 'q') return MoveGen::QueenAttacks<White>(&position);
        if (kind == 'n') return MoveGen::KnightAttacks<White>(&position);
        if (kind == 'k') return MoveGen::KingAttacks<White>(&position);
    } else {
        if (kind == 'p') return MoveGen::PawnAttacks<Black>(&position);
        if (kind == 'b') return MoveGen::BishopAttacks<Black>(&position);
        if (kind == 'r') return MoveGen::RookAttacks<Black>(&position);
        if (kind == 'q') return MoveGen::QueenAttacks<Black>(&position);
        if (kind == 'n') return MoveGen::KnightAttacks<Black>(&position);
        if (kind == 'k') return MoveGen::KingAttacks<Black>(&position);
    }
    throw std::runtime_error("unsupported attack generator kind");
}

inline BitBoard generate_all_attacks(const Position& position, const Side side) {
    return side == Side::white ? MoveGen::GenerateAllAttacks<White>(&position)
                               : MoveGen::GenerateAllAttacks<Black>(&position);
}

inline bool production_in_check(const Position& position, const Side side) {
    return side == Side::white ? MoveGen::InCheck<White>(&position)
                               : MoveGen::InCheck<Black>(&position);
}

struct PublicPlacementSnapshot {
    std::array<Piece, 64> pieces{};
    BitBoard white{};
    BitBoard black{};
    BitBoard occupied{};
    std::array<BitBoard, 6> by_type{};
    Colour turn{};

    friend bool operator==(const PublicPlacementSnapshot&, const PublicPlacementSnapshot&) = default;
};

inline PublicPlacementSnapshot public_placement(const Position& position) {
    PublicPlacementSnapshot snapshot;
    for (int square = 0; square < 64; ++square) {
        snapshot.pieces[static_cast<std::size_t>(square)] =
            position.PieceOn(static_cast<Sq>(square));
    }
    snapshot.white = position.Pieces(White);
    snapshot.black = position.Pieces(Black);
    snapshot.occupied = position.Pieces(pt_All);
    snapshot.by_type = {position.Pieces(pt_King), position.Pieces(pt_Queen),
                        position.Pieces(pt_Bishop), position.Pieces(pt_Knight),
                        position.Pieces(pt_Rook), position.Pieces(pt_Pawn)};
    snapshot.turn = position.ColourToMove();
    return snapshot;
}

inline PublicPlacementSnapshot expected_public_placement(
    const RefPosition& expected, const SquareBijection& squares) {
    PublicPlacementSnapshot snapshot;
    snapshot.pieces.fill(p_None);
    snapshot.turn = production_colour(expected.turn);
    for (int oracle_square = 0; oracle_square < 64; ++oracle_square) {
        const char piece = expected.board[static_cast<std::size_t>(oracle_square)];
        const Sq production_square = squares.square(square_name(oracle_square));
        const std::size_t production_index = static_cast<std::size_t>(production_square);
        snapshot.pieces[production_index] = named_piece(piece);
        if (piece == '.') continue;

        const BitBoard bit = BitBoard{1} << static_cast<unsigned>(production_square);
        snapshot.occupied |= bit;
        if (side_of(piece) == Side::white) snapshot.white |= bit;
        else snapshot.black |= bit;

        std::size_t type_index = 0;
        switch (kind_of(piece)) {
            case 'k': type_index = 0; break;
            case 'q': type_index = 1; break;
            case 'b': type_index = 2; break;
            case 'n': type_index = 3; break;
            case 'r': type_index = 4; break;
            case 'p': type_index = 5; break;
            default: throw std::runtime_error("invalid test-oracle piece kind");
        }
        snapshot.by_type[type_index] |= bit;
    }
    return snapshot;
}

inline bool placement_equals(const Position& position, const RefPosition& expected,
                             const SquareBijection& squares, std::string* difference = nullptr) {
    const PublicPlacementSnapshot actual_snapshot = public_placement(position);
    const PublicPlacementSnapshot expected_snapshot =
        expected_public_placement(expected, squares);
    for (int oracle_square = 0; oracle_square < 64; ++oracle_square) {
        const std::string label = square_name(oracle_square);
        const std::size_t production_index =
            static_cast<std::size_t>(squares.square(label));
        const Piece actual = actual_snapshot.pieces[production_index];
        const Piece wanted = expected_snapshot.pieces[production_index];
        if (actual != wanted) {
            if (difference) *difference = label;
            return false;
        }
    }
    if (actual_snapshot.white != expected_snapshot.white) {
        if (difference) *difference = "white occupancy";
        return false;
    }
    if (actual_snapshot.black != expected_snapshot.black) {
        if (difference) *difference = "black occupancy";
        return false;
    }
    if (actual_snapshot.occupied != expected_snapshot.occupied) {
        if (difference) *difference = "aggregate occupancy";
        return false;
    }
    if (actual_snapshot.by_type != expected_snapshot.by_type) {
        if (difference) *difference = "piece-type occupancy";
        return false;
    }
    if (actual_snapshot.turn != expected_snapshot.turn) {
        if (difference) *difference = "side-to-move";
        return false;
    }
    return true;
}

inline std::optional<ObservedMove> find_observed_move(MoveList& list,
                                                      const SquareBijection& squares,
                                                      const std::string_view uci) {
    for (const auto& move : observe_moves(list, squares)) {
        if (move.uci == uci) return move;
    }
    return std::nullopt;
}

} // namespace tdfa_test::blind
