#ifndef MOVELIST_HPP
#define MOVELIST_HPP

#include "Types.hpp"
#include "Move.hpp"
#include "Uci.hpp"
#include <algorithm>
#include <array>
#include <vector>

class MoveList
{
public:
    constexpr MoveList():data_({}),idx_(0ull){}

    constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}

    constexpr Move operator[](size_t index) const noexcept {return data_[index];}

    constexpr const std::array<Move,MAX_MOVES>& all() const noexcept {return data_;}

    constexpr size_t len()const noexcept{return idx_;}

    constexpr bool contains(const Move m){return std::find(data_.cbegin(),data_.cend(),m) != data_.cend();}

    //A debugging utility
    void print()
    {
        std::vector<std::string> moves{};
        moves.reserve(len());
        for(size_t i = 0; i < idx_; ++i)
        {
            moves.push_back(UCI::move(data_[i]));
        }
        std::sort(moves.begin(),moves.end());
        for(size_t i = 0; i < idx_; ++i)
        {
            std::cout << std::to_string(i+1) << '\t' << moves[i] << '\n';
        }
    }
private:
    std::array<Move, MAX_MOVES> data_;
    size_t idx_;
};


#endif // #ifndef MOVELIST_HPP