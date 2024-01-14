#ifndef TRANSPOSITIONTABLE_HPP
#define TRANSPOSITIONTABLE_HPP

#include "Types.hpp"
#include "ZobristConstants.hpp"
#include "Evaluate.hpp"

//(num elements)
static constexpr U64 SIZE_OF_TT = 1'000'000;

struct HashEntry
{
    ZobristKey key_;
    U8 depth_;

};


#endif // #ifndef TRANSPOSITIONTABLE_HPP