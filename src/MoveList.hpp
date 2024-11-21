#ifndef MOVELIST_HPP
#define MOVELIST_HPP

#include "Move.hpp"
#include "Types.hpp"
#include <algorithm>
#include <array>
#include <cstring>

class MoveList
{
public:
    constexpr MoveList():data_({}), idx_(0ull){}

    constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}

    constexpr Move operator[](const size_t index) const noexcept {return data_[index];}

    inline constexpr void merge(move_info const* src)
    {
        std::copy(src->encoded_move_.data(), src->encoded_move_.data() + src->count_, data_.data() + idx_);
        idx_ += src->count_;
    }
    inline constexpr void queen_merge(move_info const* src)
    {
        std::transform(src->encoded_move_.data(), src->encoded_move_.data() + src->count_, data_.data() + idx_, Moves::SetPieceType<Queen>);
        idx_ += src->count_;
    }

    [[nodiscard]] constexpr const std::array<Move, MAX_MOVES>& all() const noexcept {return data_;}

    [[nodiscard]] constexpr size_t len()const noexcept {return idx_;}

    [[nodiscard]] constexpr bool contains(const Move m) const {return std::ranges::find(data_, m) != data_.cend();}
private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP