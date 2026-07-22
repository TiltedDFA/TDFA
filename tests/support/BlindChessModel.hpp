#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace tdfa_test::blind {

enum class Side { white, black };

inline Side opposite(const Side side) {
    return side == Side::white ? Side::black : Side::white;
}

inline bool on_board(const int file, const int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

inline int index_of(const int file, const int rank) {
    return rank * 8 + file;
}

inline int file_of(const int square) { return square % 8; }
inline int rank_of(const int square) { return square / 8; }

inline std::string square_name(const int square) {
    if (square < 0 || square >= 64) {
        throw std::runtime_error("test oracle square outside 0..63");
    }
    return {static_cast<char>('a' + file_of(square)),
            static_cast<char>('1' + rank_of(square))};
}

inline int square_index(const std::string_view name) {
    if (name.size() != 2 || name[0] < 'a' || name[0] > 'h' ||
        name[1] < '1' || name[1] > '8') {
        throw std::runtime_error("noncanonical square in test oracle");
    }
    return index_of(name[0] - 'a', name[1] - '1');
}

inline bool is_piece(const char piece) {
    const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(piece)));
    return lower == 'k' || lower == 'q' || lower == 'b' || lower == 'n' ||
           lower == 'r' || lower == 'p';
}

inline Side side_of(const char piece) {
    if (!is_piece(piece)) {
        throw std::runtime_error("empty or invalid oracle piece");
    }
    return std::isupper(static_cast<unsigned char>(piece)) ? Side::white : Side::black;
}

inline char kind_of(const char piece) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(piece)));
}

inline char piece_for(const Side side, const char lower_kind) {
    return side == Side::white
        ? static_cast<char>(std::toupper(static_cast<unsigned char>(lower_kind)))
        : lower_kind;
}

struct LogicalMove {
    int from{};
    int to{};
    char promotion{};

    [[nodiscard]] std::string uci() const {
        std::string text = square_name(from) + square_name(to);
        if (promotion != 0) {
            text.push_back(promotion);
        }
        return text;
    }

    friend bool operator==(const LogicalMove&, const LogicalMove&) = default;
    friend bool operator<(const LogicalMove& lhs, const LogicalMove& rhs) {
        return std::tie(lhs.from, lhs.to, lhs.promotion) <
               std::tie(rhs.from, rhs.to, rhs.promotion);
    }
};

inline std::vector<std::string> split_exact(const std::string_view text, const char delimiter) {
    std::vector<std::string> fields;
    std::size_t begin = 0;
    for (;;) {
        const std::size_t end = text.find(delimiter, begin);
        fields.emplace_back(text.substr(begin, end == std::string_view::npos
            ? text.size() - begin : end - begin));
        if (end == std::string_view::npos) {
            return fields;
        }
        begin = end + 1;
    }
}

struct RefPosition {
    std::array<char, 64> board{};
    Side turn{Side::white};
    std::string castling{"-"};
    std::optional<int> en_passant;
    unsigned halfmove{};
    unsigned fullmove{1};

    RefPosition() { board.fill('.'); }

    static RefPosition from_fen(const std::string_view fen) {
        const auto fields = split_exact(fen, ' ');
        if (fields.size() != 6) {
            throw std::runtime_error("oracle requires canonical six-field FEN");
        }
        RefPosition position;
        const auto ranks = split_exact(fields[0], '/');
        if (ranks.size() != 8) {
            throw std::runtime_error("oracle FEN rank count");
        }
        for (int fen_rank = 0; fen_rank < 8; ++fen_rank) {
            int file = 0;
            for (const char token : ranks[static_cast<std::size_t>(fen_rank)]) {
                if (token >= '1' && token <= '8') {
                    file += token - '0';
                } else if (is_piece(token)) {
                    if (file >= 8) throw std::runtime_error("oracle FEN rank overflow");
                    position.board[static_cast<std::size_t>(index_of(file, 7 - fen_rank))] = token;
                    ++file;
                } else {
                    throw std::runtime_error("oracle FEN piece token");
                }
            }
            if (file != 8) throw std::runtime_error("oracle FEN rank width");
        }
        if (fields[1] == "w") position.turn = Side::white;
        else if (fields[1] == "b") position.turn = Side::black;
        else throw std::runtime_error("oracle FEN active colour");
        position.castling = fields[2];
        position.en_passant = fields[3] == "-"
            ? std::nullopt : std::optional<int>(square_index(fields[3]));
        position.halfmove = static_cast<unsigned>(std::stoul(fields[4]));
        position.fullmove = static_cast<unsigned>(std::stoul(fields[5]));
        return position;
    }

    [[nodiscard]] std::string fen() const {
        std::ostringstream out;
        for (int rank = 7; rank >= 0; --rank) {
            int empty = 0;
            for (int file = 0; file < 8; ++file) {
                const char piece = board[static_cast<std::size_t>(index_of(file, rank))];
                if (piece == '.') {
                    ++empty;
                } else {
                    if (empty != 0) out << empty;
                    empty = 0;
                    out << piece;
                }
            }
            if (empty != 0) out << empty;
            if (rank != 0) out << '/';
        }
        out << ' ' << (turn == Side::white ? 'w' : 'b') << ' '
            << (castling.empty() ? "-" : castling) << ' '
            << (en_passant ? square_name(*en_passant) : "-") << ' '
            << halfmove << ' ' << fullmove;
        return out.str();
    }

    [[nodiscard]] bool occupied(const int square) const {
        return board[static_cast<std::size_t>(square)] != '.';
    }

    [[nodiscard]] bool occupied_by(const int square, const Side side) const {
        return occupied(square) && side_of(board[static_cast<std::size_t>(square)]) == side;
    }

    [[nodiscard]] int king_square(const Side side) const {
        const char king = piece_for(side, 'k');
        for (int square = 0; square < 64; ++square) {
            if (board[static_cast<std::size_t>(square)] == king) return square;
        }
        return -1;
    }

    [[nodiscard]] std::set<int> attacks_from(const int from) const {
        std::set<int> result;
        const char piece = board[static_cast<std::size_t>(from)];
        if (!is_piece(piece)) return result;
        const Side side = side_of(piece);
        const char kind = kind_of(piece);
        const int file = file_of(from);
        const int rank = rank_of(from);
        auto add = [&](const int target_file, const int target_rank) {
            if (on_board(target_file, target_rank)) {
                result.insert(index_of(target_file, target_rank));
            }
        };
        if (kind == 'p') {
            const int step = side == Side::white ? 1 : -1;
            add(file - 1, rank + step);
            add(file + 1, rank + step);
        } else if (kind == 'n') {
            constexpr std::array<std::pair<int, int>, 8> deltas{{
                {1, 2}, {2, 1}, {2, -1}, {1, -2},
                {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};
            for (const auto [df, dr] : deltas) add(file + df, rank + dr);
        } else if (kind == 'k') {
            for (int df = -1; df <= 1; ++df) {
                for (int dr = -1; dr <= 1; ++dr) {
                    if (df != 0 || dr != 0) add(file + df, rank + dr);
                }
            }
        } else {
            std::vector<std::pair<int, int>> directions;
            if (kind == 'b' || kind == 'q') {
                directions.insert(directions.end(), {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}});
            }
            if (kind == 'r' || kind == 'q') {
                directions.insert(directions.end(), {{1, 0}, {-1, 0}, {0, 1}, {0, -1}});
            }
            for (const auto [df, dr] : directions) {
                for (int f = file + df, r = rank + dr; on_board(f, r); f += df, r += dr) {
                    const int target = index_of(f, r);
                    result.insert(target);
                    if (occupied(target)) break;
                }
            }
        }
        return result;
    }

    [[nodiscard]] std::set<int> attacks(const Side by_side) const {
        std::set<int> result;
        for (int square = 0; square < 64; ++square) {
            if (!occupied_by(square, by_side)) continue;
            const auto piece_attacks = attacks_from(square);
            result.insert(piece_attacks.begin(), piece_attacks.end());
        }
        return result;
    }

    [[nodiscard]] bool is_attacked(const int square, const Side by_side) const {
        const auto set = attacks(by_side);
        return set.contains(square);
    }

    [[nodiscard]] bool in_check(const Side side) const {
        const int king = king_square(side);
        if (king < 0) throw std::runtime_error("oracle position has no king");
        return is_attacked(king, opposite(side));
    }

    void add_step_moves(std::vector<LogicalMove>& moves, const int from,
                        const std::vector<std::pair<int, int>>& deltas) const {
        const Side side = side_of(board[static_cast<std::size_t>(from)]);
        const int file = file_of(from);
        const int rank = rank_of(from);
        for (const auto [df, dr] : deltas) {
            const int f = file + df;
            const int r = rank + dr;
            if (!on_board(f, r)) continue;
            const int to = index_of(f, r);
            if (!occupied_by(to, side)) moves.push_back({from, to, 0});
        }
    }

    void add_slider_moves(std::vector<LogicalMove>& moves, const int from,
                          const std::vector<std::pair<int, int>>& directions) const {
        const Side side = side_of(board[static_cast<std::size_t>(from)]);
        for (const auto [df, dr] : directions) {
            for (int f = file_of(from) + df, r = rank_of(from) + dr;
                 on_board(f, r); f += df, r += dr) {
                const int to = index_of(f, r);
                if (occupied_by(to, side)) break;
                moves.push_back({from, to, 0});
                if (occupied(to)) break;
            }
        }
    }

    void add_pawn_moves(std::vector<LogicalMove>& moves, const int from) const {
        const Side side = side_of(board[static_cast<std::size_t>(from)]);
        const int step = side == Side::white ? 1 : -1;
        const int home = side == Side::white ? 1 : 6;
        const int last = side == Side::white ? 7 : 0;
        const int file = file_of(from);
        const int rank = rank_of(from);
        auto append = [&](const int to) {
            if (rank_of(to) == last) {
                for (const char promotion : std::string_view{"qrbn"}) {
                    moves.push_back({from, to, promotion});
                }
            } else {
                moves.push_back({from, to, 0});
            }
        };
        if (on_board(file, rank + step)) {
            const int one = index_of(file, rank + step);
            if (!occupied(one)) {
                append(one);
                if (rank == home) {
                    const int two = index_of(file, rank + 2 * step);
                    if (!occupied(two)) moves.push_back({from, two, 0});
                }
            }
        }
        for (const int df : {-1, 1}) {
            if (!on_board(file + df, rank + step)) continue;
            const int to = index_of(file + df, rank + step);
            const bool enemy = occupied(to) && !occupied_by(to, side);
            if (enemy || (en_passant && *en_passant == to)) append(to);
        }
    }

    [[nodiscard]] bool has_right(const char right) const {
        return castling != "-" && castling.find(right) != std::string::npos;
    }

    void add_castles(std::vector<LogicalMove>& moves, const Side side) const {
        const int rank = side == Side::white ? 0 : 7;
        const int king = index_of(4, rank);
        if (board[static_cast<std::size_t>(king)] != piece_for(side, 'k')) return;
        const Side enemy = opposite(side);
        auto can_castle = [&](const bool king_side) {
            const char right = side == Side::white
                ? (king_side ? 'K' : 'Q') : (king_side ? 'k' : 'q');
            const int rook_file = king_side ? 7 : 0;
            if (!has_right(right) ||
                board[static_cast<std::size_t>(index_of(rook_file, rank))] != piece_for(side, 'r')) {
                return false;
            }
            if (king_side) {
                if (occupied(index_of(5, rank)) || occupied(index_of(6, rank))) return false;
            } else {
                if (occupied(index_of(1, rank)) || occupied(index_of(2, rank)) ||
                    occupied(index_of(3, rank))) return false;
            }
            const int transit = index_of(king_side ? 5 : 3, rank);
            const int target = index_of(king_side ? 6 : 2, rank);
            return !is_attacked(king, enemy) && !is_attacked(transit, enemy) &&
                   !is_attacked(target, enemy);
        };
        if (can_castle(true)) moves.push_back({king, index_of(6, rank), 0});
        if (can_castle(false)) moves.push_back({king, index_of(2, rank), 0});
    }

    [[nodiscard]] std::vector<LogicalMove> pseudo_moves(
        const Side side, const std::optional<char> only_kind = std::nullopt,
        const bool include_castling = true) const {
        std::vector<LogicalMove> moves;
        constexpr std::array<std::pair<int, int>, 8> knight{{
            {1, 2}, {2, 1}, {2, -1}, {1, -2},
            {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};
        std::vector<std::pair<int, int>> king;
        for (int df = -1; df <= 1; ++df)
            for (int dr = -1; dr <= 1; ++dr)
                if (df != 0 || dr != 0) king.emplace_back(df, dr);
        const std::vector<std::pair<int, int>> bishop{{1,1},{1,-1},{-1,1},{-1,-1}};
        const std::vector<std::pair<int, int>> rook{{1,0},{-1,0},{0,1},{0,-1}};
        auto queen = bishop;
        queen.insert(queen.end(), rook.begin(), rook.end());
        for (int from = 0; from < 64; ++from) {
            if (!occupied_by(from, side)) continue;
            const char kind = kind_of(board[static_cast<std::size_t>(from)]);
            if (only_kind && kind != *only_kind) continue;
            if (kind == 'p') add_pawn_moves(moves, from);
            else if (kind == 'n') add_step_moves(
                moves, from, std::vector<std::pair<int, int>>(knight.begin(), knight.end()));
            else if (kind == 'b') add_slider_moves(moves, from, bishop);
            else if (kind == 'r') add_slider_moves(moves, from, rook);
            else if (kind == 'q') add_slider_moves(moves, from, queen);
            else if (kind == 'k') add_step_moves(moves, from, king);
        }
        if (include_castling && (!only_kind || *only_kind == 'k')) add_castles(moves, side);
        return moves;
    }

    void erase_right(const char right) {
        if (castling == "-") return;
        castling.erase(std::remove(castling.begin(), castling.end(), right), castling.end());
        if (castling.empty()) castling = "-";
    }

    [[nodiscard]] RefPosition after(const LogicalMove& move) const {
        RefPosition next = *this;
        const char moving = board[static_cast<std::size_t>(move.from)];
        if (!is_piece(moving) || side_of(moving) != turn) {
            throw std::runtime_error("oracle move source does not contain side-to-move piece");
        }
        const Side moving_side = turn;
        const char kind = kind_of(moving);
        char captured = board[static_cast<std::size_t>(move.to)];
        int captured_square = move.to;
        next.board[static_cast<std::size_t>(move.from)] = '.';
        if (kind == 'p' && file_of(move.from) != file_of(move.to) && captured == '.' &&
            en_passant && *en_passant == move.to) {
            captured_square = move.to + (moving_side == Side::white ? -8 : 8);
            captured = next.board[static_cast<std::size_t>(captured_square)];
            next.board[static_cast<std::size_t>(captured_square)] = '.';
        }
        char placed = moving;
        if (kind == 'p' && move.promotion != 0) placed = piece_for(moving_side, move.promotion);
        next.board[static_cast<std::size_t>(move.to)] = placed;
        if (kind == 'k' && std::abs(file_of(move.to) - file_of(move.from)) == 2) {
            const int rank = rank_of(move.from);
            const bool king_side = file_of(move.to) == 6;
            const int rook_from = index_of(king_side ? 7 : 0, rank);
            const int rook_to = index_of(king_side ? 5 : 3, rank);
            next.board[static_cast<std::size_t>(rook_to)] = next.board[static_cast<std::size_t>(rook_from)];
            next.board[static_cast<std::size_t>(rook_from)] = '.';
        }
        if (kind == 'k') {
            if (moving_side == Side::white) { next.erase_right('K'); next.erase_right('Q'); }
            else { next.erase_right('k'); next.erase_right('q'); }
        }
        if (kind == 'r') {
            if (move.from == square_index("a1")) next.erase_right('Q');
            if (move.from == square_index("h1")) next.erase_right('K');
            if (move.from == square_index("a8")) next.erase_right('q');
            if (move.from == square_index("h8")) next.erase_right('k');
        }
        if (captured != '.' && kind_of(captured) == 'r') {
            if (captured_square == square_index("a1")) next.erase_right('Q');
            if (captured_square == square_index("h1")) next.erase_right('K');
            if (captured_square == square_index("a8")) next.erase_right('q');
            if (captured_square == square_index("h8")) next.erase_right('k');
        }
        next.en_passant.reset();
        if (kind == 'p' && std::abs(rank_of(move.to) - rank_of(move.from)) == 2) {
            next.en_passant = (move.from + move.to) / 2;
        }
        next.halfmove = (kind == 'p' || captured != '.') ? 0U : halfmove + 1U;
        next.fullmove = fullmove + (moving_side == Side::black ? 1U : 0U);
        next.turn = opposite(turn);
        return next;
    }

    [[nodiscard]] std::vector<LogicalMove> legal_moves(const Side side) const {
        std::vector<LogicalMove> legal;
        for (const LogicalMove& move : pseudo_moves(side)) {
            if (occupied(move.to) && kind_of(board[static_cast<std::size_t>(move.to)]) == 'k') continue;
            RefPosition base = *this;
            base.turn = side;
            const RefPosition child = base.after(move);
            const int king = child.king_square(side);
            if (king >= 0 && !child.is_attacked(king, opposite(side))) legal.push_back(move);
        }
        return legal;
    }

    [[nodiscard]] LogicalMove legal_move_from_uci(const std::string_view token) const {
        for (const LogicalMove& move : legal_moves(turn)) {
            if (move.uci() == token) return move;
        }
        throw std::runtime_error("history token is not legal in oracle position");
    }
};

inline std::set<std::string> uci_set(const std::vector<LogicalMove>& moves) {
    std::set<std::string> result;
    for (const auto& move : moves) result.insert(move.uci());
    return result;
}

inline std::set<std::string> destination_set(const std::vector<LogicalMove>& moves) {
    std::set<std::string> result;
    for (const auto& move : moves) result.insert(square_name(move.to));
    return result;
}

inline RefPosition rank_flip_colour_swap(const RefPosition& source) {
    RefPosition result;
    result.board.fill('.');
    for (int square = 0; square < 64; ++square) {
        const char piece = source.board[static_cast<std::size_t>(square)];
        if (piece == '.') continue;
        const int target = index_of(file_of(square), 7 - rank_of(square));
        result.board[static_cast<std::size_t>(target)] = std::isupper(static_cast<unsigned char>(piece))
            ? static_cast<char>(std::tolower(static_cast<unsigned char>(piece)))
            : static_cast<char>(std::toupper(static_cast<unsigned char>(piece)));
    }
    result.turn = opposite(source.turn);
    result.castling.clear();
    if (source.castling.find('k') != std::string::npos) result.castling.push_back('K');
    if (source.castling.find('q') != std::string::npos) result.castling.push_back('Q');
    if (source.castling.find('K') != std::string::npos) result.castling.push_back('k');
    if (source.castling.find('Q') != std::string::npos) result.castling.push_back('q');
    if (result.castling.empty()) result.castling = "-";
    if (source.en_passant) {
        result.en_passant = index_of(file_of(*source.en_passant), 7 - rank_of(*source.en_passant));
    }
    result.halfmove = source.halfmove;
    result.fullmove = source.fullmove;
    return result;
}

inline std::optional<RefPosition> add_legal_king_shell(
    const RefPosition& pieces, const std::optional<int> fixed_white_king = std::nullopt,
    const std::optional<int> fixed_black_king = std::nullopt) {
    for (int white = 0; white < 64; ++white) {
        if (fixed_white_king && white != *fixed_white_king) continue;
        for (int black = 0; black < 64; ++black) {
            if (fixed_black_king && black != *fixed_black_king) continue;
            if (white == black) continue;
            RefPosition candidate = pieces;
            if ((candidate.occupied(white) && candidate.board[static_cast<std::size_t>(white)] != 'K') ||
                (candidate.occupied(black) && candidate.board[static_cast<std::size_t>(black)] != 'k')) continue;
            candidate.board[static_cast<std::size_t>(white)] = 'K';
            candidate.board[static_cast<std::size_t>(black)] = 'k';
            if (candidate.is_attacked(white, Side::black) || candidate.is_attacked(black, Side::white)) continue;
            return candidate;
        }
    }
    return std::nullopt;
}

} // namespace tdfa_test::blind
