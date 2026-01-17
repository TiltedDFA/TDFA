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


/*
    16 Bytes
    ZobristKey key_;    -> 64b  -> 8B
    Score eval_;        -> 16b  -> 2B
    Move best_;         -> 16b  -> 2B
    U8 depth_;          -> 8b   -> 1B
    BoundType bound_;   -> 8b   -> 1B
    U8 padding_[2];     -> 16b  -> 2B
*/
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

constexpr size_t TT_BUCKET_SIZE = 4;
struct alignas(64) HashBucket
{
    HashEntry entries[TT_BUCKET_SIZE];
};
static_assert(sizeof(HashBucket) == 64);
class TransposTable
{
public:
    TransposTable():num_buckets_(0),table_ptr_(nullptr){}
    ~TransposTable(){delete[] table_ptr_;};
    void Resize(size_t size_in_mB);
    void Store(ZobristKey, Score, Move, U8, BoundType) const;
    [[nodiscard]] HashEntry const* Probe(ZobristKey)const;
    void Clear() const;
    [[nodiscard]] size_t GetNumElems()const{return num_buckets_ * TT_BUCKET_SIZE;}
private:
    size_t num_buckets_;
    HashBucket* table_ptr_;
};

#endif // #ifndef TRANSPOSITIONTABLE_HPP
