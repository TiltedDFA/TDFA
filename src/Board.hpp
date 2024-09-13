//
// Created by Malik T on 14/09/2024.
//

#ifndef TDFA_BOARD_HPP
#define TDFA_BOARD_HPP

#include "Types.hpp"

namespace Internals
{
    enum Colour
    {
        White,
        Black
    };
    enum LocPieces
    {
        King,
        Queen,
        Bishop,
        Knight,
        Rook,
        Pawn,
        All
    };
}
class Board
{
public:
    Board();
private:

};


#endif //TDFA_BOARD_HPP
