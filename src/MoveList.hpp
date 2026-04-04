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
    MoveList(): idx_(0){}

    constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}

    constexpr Move operator[](const size_t index) const noexcept {return data_[index];}

    constexpr void merge(move_info const* src)
    {
        std::copy_n(src->encoded_move_.data(), src->count_, data_.data() + idx_);
        idx_ += src->count_;
    }

    [[nodiscard]] constexpr std::array<Move, MAX_MOVES>& all() noexcept {return data_;}

    [[nodiscard]] constexpr size_t len()const noexcept {return idx_;}

    [[nodiscard]] constexpr bool contains(const Move m) const {return std::find(data_.begin(), data_.begin() + idx_, m) != data_.begin() + idx_;}

private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP