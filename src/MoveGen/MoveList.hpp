#ifndef MOVELIST_HPP
#define MOVELIST_HPP

#include "../Core/Types.hpp"
#include "../Core/Move.hpp"
#include <algorithm>
#include <array>

class MoveList
{
public:
    constexpr MoveList():data_({}),idx_(0ull){}

    constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}

    constexpr Move operator[](size_t index) const noexcept {return data_[index];}

    constexpr const std::array<Move,MAX_MOVES>& all() const noexcept {return data_;}

    constexpr size_t len()const noexcept{return idx_;}

    constexpr bool contains(const Move m){return std::find(data_.cbegin(),data_.cend(),m) != data_.cend();}
private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP