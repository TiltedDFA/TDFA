#ifndef UCI_HPP
#define UCI_HPP

#include "Position.hpp"
#include "Debug.hpp"
#include "Move.hpp"
#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Testing.hpp"
#include "Search.hpp"
#include "TranspositionTable.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

// Stack-allocated arg list — no heap allocation
struct ArgList
{
    std::array<std::string_view, 128> args_;
    U8 count_{0};

    void push(std::string_view sv) noexcept { args_[count_++] = sv; }
    [[nodiscard]] std::string_view operator[](size_t i) const noexcept { return args_[i]; }
    [[nodiscard]] U8 size() const noexcept { return count_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

    // Iterator support for range-based for / std::ranges::find
    const std::string_view* begin() const noexcept { return args_.data(); }
    const std::string_view* end() const noexcept { return args_.data() + count_; }
    const std::string_view* cbegin() const noexcept { return begin(); }
    const std::string_view* cend() const noexcept { return end(); }
};

class Uci
{
public:
    Uci():tt_size_(16), pos_(STARTPOS), tt_(), time_manager_(){tt_.Resize(tt_size_);}
    void Loop();
private:
    void HandleUci();
    void HandleIsReady();
    void HandleGo(const ArgList&);
    void HandlePosition(const ArgList&);

    void HandleStop();
    void WaitForSearch();
    void HandleNewGame();
    void HandleSetOption(const ArgList&);
    static void HandleBench(const ArgList&);
    void HandlePrint(const ArgList&);
private:
    size_t tt_size_;
    Position pos_;
    TransposTable tt_;
    TimeManager time_manager_;
    Search search_;
    std::string input_buf_;
    std::thread search_thread_;
private:
    static constexpr const char* ENGINE_NAME = "TDFA V1.2.1";
    static constexpr const char* ENGINE_AUTHOR = "Malik Tremain";
};
#endif // #ifndef UCI_HPP