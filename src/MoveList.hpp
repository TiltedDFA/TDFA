#ifndef MOVELIST_HPP
#define MOVELIST_HPP

#include "Types.hpp"
#include <algorithm>
#include <array>

class MoveList
{
public:
    constexpr MoveList():data_({}), idx_(0ull){}

    constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}

    constexpr Move operator[](const size_t index) const noexcept {return data_[index];}

    [[nodiscard]] constexpr const std::array<Move, MAX_MOVES>& all() const noexcept {return data_;}

    [[nodiscard]] constexpr size_t len()const noexcept {return idx_;}

    [[nodiscard]] constexpr bool contains(const Move m) const {return std::ranges::find(data_, m) != data_.cend();}
private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP