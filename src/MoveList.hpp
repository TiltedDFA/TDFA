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
    constexpr MoveList() noexcept : idx_(0ull) {}

    constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}

    constexpr Move operator[](const size_t index) const noexcept {return data_[index];}

    INLINE void merge(move_info const* __restrict__ src) noexcept
    {
        const U8 count = src->count_;
        std::memcpy(data_.data() + idx_, src->encoded_move_.data(), count * sizeof(Move));
        idx_ += count;
    }

    [[nodiscard]] constexpr std::array<Move, MAX_MOVES>& all() noexcept {return data_;}

    [[nodiscard]] constexpr size_t len()const noexcept {return idx_;}

    constexpr void clear() noexcept {idx_ = 0;}

    [[nodiscard]] constexpr bool contains(const Move m) const {return std::ranges::find(data_.begin(), data_.begin() + idx_, m) != (data_.begin() + idx_);}

private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP
