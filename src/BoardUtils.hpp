#ifndef BOARDUTILS_HPP
#define BOARDUTILS_HPP
#include "Types.hpp"
#include <array>
#include <string_view>

constexpr std::string_view RemoveWhiteSpace(std::string_view str) noexcept
{
    size_t start = 0;
    size_t end = str.size();
    while(start < end && str[start] == ' ') ++start;
    while(end > start && str[end - 1] == ' ') --end;
    return str.substr(start, end - start);
}
constexpr void SplitFen(std::string_view fen, std::array<std::string_view,6>& fen_sections) noexcept
{
    U8 section = 0;
    size_t start = 0;
    for(size_t i = 0; i < fen.size(); ++i)
    {
        if(fen[i] == ' ')
        {
            fen_sections[section++] = fen.substr(start, i - start);
            start = i + 1;
        }
    }
    if(start <= fen.size())
        fen_sections[section] = fen.substr(start);
}
constexpr bool IsDigit(const char i) noexcept {return U8(i - '0') <= 9;}

#endif // #ifndef BOARDUTILS_HPP