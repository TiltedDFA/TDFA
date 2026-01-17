#include "TranspositionTable.hpp"

static inline constexpr uint64_t index(const ZobristKey z, const size_t s) noexcept
{
    //maybe uncessary assertion but on some uncommon architectures could cause issue
    static_assert(sizeof(ZobristKey) == 8 && sizeof(size_t) == 8);
    return static_cast<uint64_t>((static_cast<unsigned __int128>(z) * static_cast<unsigned __int128>(s)) >> 64);;
}
void TransposTable::Resize(const size_t size_in_mB)
{
    const size_t num_bytes = size_in_mB * 1024 * 1024;
    num_buckets_ = num_bytes / sizeof(HashBucket);
    if(num_buckets_ == 0)
        num_buckets_ = 1;

    delete[] table_ptr_;
    
    table_ptr_ = new HashBucket[num_buckets_];
    
    std::memset(table_ptr_, 0, sizeof(HashBucket) * num_buckets_);
}
void TransposTable::Store(
                            ZobristKey  key,
                            Score       eval,
                            Move        best,
                            U8          depth,
                            BoundType   bound
                          ) const {
    assert(num_buckets_ != 0);

    HashBucket* bucket = &table_ptr_[index(key, num_buckets_)];
    HashEntry* entry = &bucket->entries[0];
    U8 lowest_depth = entry->depth_;

    for(size_t i = 0; i < TT_BUCKET_SIZE; ++i)
    {
        HashEntry* cur = &bucket->entries[i];
        if(cur->key_ == key || cur->key_ == 0)
        {
            entry = cur;
            break;
        }
        if(cur->depth_ < lowest_depth)
        {
            lowest_depth = cur->depth_;
            entry = cur;
        }
    }

    #if DEBUG_TRANPOSITION_TABLE == 1
    PRINTNL(std::format("key: {}, num_buckets: {}", key, num_buckets_));
    #endif

    entry->key_   = key;
    entry->eval_  = eval;
    entry->best_  = best;
    entry->depth_ = depth;
    entry->bound_ = bound;
}
HashEntry const* TransposTable::Probe(ZobristKey key)const
{
    assert(num_buckets_ != 0);
    const HashBucket* bucket = &table_ptr_[index(key, num_buckets_)];
    __builtin_prefetch(bucket, 0, 1);
    for(size_t i = 0; i < TT_BUCKET_SIZE; ++i)
    {
        const HashEntry* entry = &bucket->entries[i];
        if(entry->key_ == key)
            return entry;
    }
    return nullptr;
}
void TransposTable::Clear() const {
    std::memset(table_ptr_, 0, sizeof(HashBucket) * num_buckets_);
}
