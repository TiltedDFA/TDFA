#ifndef TRANSPOSITIONTABLE_HPP
#define TRANSPOSITIONTABLE_HPP

#include "Types.hpp"
#include "ZobristConstants.hpp"
#include "Evaluate.hpp"
#include "Debug.hpp"
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <format>

struct HashEntry
{
    ZobristKey key_;
    Score eval_;
    Move best_;
    U8 depth_;
    BoundType bound_;
    U8 padding_[2];
};
static_assert(sizeof(HashEntry) == 16);
class TransposTable
{
public:
    ~TransposTable(){delete[] table_ptr_;};
    void Resize(size_t size_in_mB);
    void Store(ZobristKey, Score, Move, U8, BoundType);
    HashEntry const* Probe(ZobristKey)const;
    void Clear();
private:
    size_t num_elements_;
    HashEntry* table_ptr_{nullptr};
};
#endif // #ifndef TRANSPOSITIONTABLE_HPP