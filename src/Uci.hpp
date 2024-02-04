#ifndef UCI_HPP
#define UCI_HPP

#include "BitBoard.hpp"
#include "Debug.hpp"
#include "Move.hpp"
#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Testing.hpp"
#include "Search.hpp"
#include "MoveGen.hpp"
#include "TranspositionTable.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using CmdMap = std::unordered_map<std::string_view, U8>;
using ArgList = std::vector<std::string_view>;
class Uci
{
public:
    Uci():tt_size_(64), pos_(STARTPOS), tt_(), time_manager_(){tt_.Resize(tt_size_);}
    void Loop();
private:
    //helper functions
    void HandleUci();
    void HandleIsReady();
    void HandleGo(const ArgList&);
    void HandlePosition(const ArgList&);
    void HandleStop();
    void HandleNewGame();
    void HandleSetOption(const ArgList&);
    void HandleBench(const ArgList&);
    void HandlePrint(const ArgList&);
private:
    //mutables
    size_t tt_size_;
    Position pos_;    
    TransposTable tt_;
    TimeManager time_manager_;
    Search search_;
private:
    //constants
    static constexpr const char* ENGINE_NAME = "TDFA V1.1.0";
    static constexpr const char* ENGINE_AUTHOR = "Malik Tremain";
    static inline const CmdMap COMMAND_VALUES = 
    {
        {"uci", 1},
        {"isready", 2},
        {"go", 3},
        {"position", 4},
        {"stop", 5},
        {"ucinewgame", 6},
        {"setoption", 7},
        {"bench", 8},
        {"print", 9}
    };
};
#endif // #ifndef UCI_HPP