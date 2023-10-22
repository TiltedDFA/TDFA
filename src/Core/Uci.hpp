#ifndef UCI_HPP
#define UCI_HPP

#include "../Core/Move.hpp"
#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"

namespace UCI
{
    std::string square(Sq sq);
    std::string move(Move m);
}
#endif // #ifndef UCI_HPP