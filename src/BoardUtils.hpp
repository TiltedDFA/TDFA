#ifndef BOARDUTILS_HPP
#define BOARDUTILS_HPP
#include "Types.hpp"
#include <array>
#include <string_view>

constexpr std::string_view RemoveWhiteSpace(std::string_view str)
{
    if(str.empty()) return str;

    std::size_t start = 0;
    while(start < str.size() && str[start] == ' ')
        ++start;
    if(start == str.size())
        return {};

    std::size_t end = str.size();
    while(end > start && str[end - 1] == ' ')
        --end;

    return {str.data() + start, end - start};
}
constexpr void SplitFen(std::string_view fen, std::array<std::string_view,6>& fen_sections)
{
    std::size_t start = 0;
    U8 current_fen_section = 0;

    for(std::size_t i = 0; i < fen.size() && current_fen_section < fen_sections.size(); ++i)
    {
        if(fen[i] == ' ')
        {
            fen_sections.at(current_fen_section) = std::string_view(fen.data() + start, i - start);
            ++current_fen_section;
            start = i + 1;
        }
    }

    if(current_fen_section < fen_sections.size())
    {
        fen_sections.at(current_fen_section) = std::string_view(fen.data() + start, fen.size() - start);
        ++current_fen_section;
    }
}
constexpr bool IsDigit(const char i) {return i <= '9' && i >= '0';}

#endif // #ifndef BOARDUTILS_HPP
