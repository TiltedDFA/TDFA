#ifndef UCI_HPP
#define UCI_HPP

#include "Move.hpp"
#include "Types.hpp"
#include "MagicConstants.hpp"
#include "Debug.hpp"
#include "BitBoard.hpp"
#include "Testing.hpp"
#include "Search.hpp"
#include "MoveGen.hpp"
#include "TranspositionTable.hpp"
#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <string_view>
#include <charconv>

using CmdMap = std::unordered_map<std::string_view, U8>;
using ArgList = std::vector<std::string_view>;
namespace UCI
{
    constexpr const char* ENGINE_NAME = "TDFA V1.0.0";
    constexpr const char* ENGINE_AUTHOR = "Malik Tremain";
    inline const CmdMap INIT_VALUES = 
    {
        {"uci", 1},
        {"isready", 2},
        {"go", 3},
        {"position", 4},
        {"stop", 5},
        {"ucinewgame", 6},
        {"setoption", 7},
        {"bench", 8}
    };
    constexpr size_t TT_SIZE = 64;//mb
    
    inline Position pos(STARTPOS);    
    inline TransposTable transpos_table;
    inline TimeManager time_manager;

    void loop();
    void HandleUci();
    void HandleIsReady();
    void HandleGo(const ArgList&);
    void HandlePosition(const ArgList&);
    void HandleStop();
    void HandleNewGame();
    void HandleSetOption(const ArgList&);
    void HandleBench(const ArgList&);
}
#endif // #ifndef UCI_HPP