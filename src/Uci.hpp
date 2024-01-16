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

using CmdMap = std::unordered_map<std::string_view, U8>;
using ArgList = std::vector<std::string_view>;
namespace UCI
{
    inline constexpr const char* ENGINE_NAME = "TDFA V1";
    inline constexpr const char* ENGINE_AUTHOR = "Malik Tremain";
    inline const CmdMap INIT_VALUES = 
    {
        {"uci", 1},
        {"isready", 2},
        {"go", 3},
        {"position", 4},
        {"stop", 5},
        {"ucinewgame", 6},
        {"setoption", 7}
    };
    inline size_t constexpr TT_SIZE = 128;//mb
    
    inline BB::Position pos(STARTPOS);    
    inline TransposTable transpos_table;
    void loop();

    void HandleUci();
    void HandleIsReady();
    void HandleGo();
    void HandlePosition(const ArgList&);
    void HandleStop();
    void HandleNewGame();
    void HandleSetOption(const ArgList&);
}
#endif // #ifndef UCI_HPP