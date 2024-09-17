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

    constexpr void add(const Move m)   {data_[idx_++] = m;}

    constexpr Move operator[](const size_t index) const  {return data_[index];}

    inline constexpr void merge(move_info const* src)
    {
        std::copy(src->encoded_move_.data(), src->encoded_move_.data() + src->count_, data_.data() + idx_);
        idx_ += src->count_;
    }

    [[nodiscard]] constexpr const std::array<Move, MAX_MOVES>& all() const  {return data_;}

    [[nodiscard]] constexpr size_t len()const  {return idx_;}

    [[nodiscard]]
    constexpr inline Move const* begin()
    {
        return data_.data();
    }
    [[nodiscard]]
    constexpr inline Move const* end()
    {
        return data_.data() + idx_;
    }
private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP