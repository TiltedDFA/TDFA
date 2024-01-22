#ifndef BOARDUTILS_HPP
#define BOARDUTILS_HPP
#include "Types.hpp"
#include <array>
#include <charconv>

inline constexpr std::string_view RemoveWhiteSpace(std::string_view str)
{
    int start_index{-1};
    int end_index{static_cast<int>(str.size())};
    while(str.at(++start_index) == ' '){}
    while(str.at(--end_index) == ' '){}
    return std::string_view(str.begin() + start_index, str.begin() + end_index + 1);
}
inline constexpr void SplitFen(std::string_view fen, std::array<std::string_view,6>& fen_sections)
{
    int start = 0;
    int end = -1;
    U8 current_fen_section = 0;
    while(size_t(++end) < fen.size())
    {
        if(fen.at(end) == ' ')
        {
            ++end;
            fen_sections.at(current_fen_section) = std::string_view(fen.begin() + start, fen.begin() + (end - 1));
            ++current_fen_section;
            start = end;
            continue;
        }
    }
    fen_sections.at(current_fen_section) = std::string_view(fen.begin() + start, fen.begin() + (++end - 1));
    ++current_fen_section;
}
inline constexpr bool IsDigit(const char i) {return i <= '9' && i >= '0';}
#endif // #ifndef BOARDUTILS_HPP