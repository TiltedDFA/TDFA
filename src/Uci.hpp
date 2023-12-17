#ifndef UCI_HPP
#define UCI_HPP

#include "Move.hpp"
#include "Types.hpp"
#include "MagicConstants.hpp"

namespace UCI
{
    std::string square(Sq sq);
    std::string move(Move m);
}
#endif // #ifndef UCI_HPP