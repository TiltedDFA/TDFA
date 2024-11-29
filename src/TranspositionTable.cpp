#include "TranspositionTable.hpp"

static inline constexpr uint32_t index(const ZobristKey z, const size_t s) noexcept
{
    //maybe uncessary assertion but on some uncommon architectures could cause issue
    static_assert(sizeof(ZobristKey) == 8 && sizeof(size_t) == 8);
    // return (z * s ) >> 32;
    return z % s;
}
void TransposTable::Resize(const size_t size_in_mB)
{
    const size_t num_bytes = size_in_mB * 1024 * 1024;
    num_elements_ = num_bytes / sizeof(HashEntry);

    delete[] table_ptr_;
    
    table_ptr_ = new HashEntry[num_elements_];
    
    // if(!table_ptr_)
    // {
    //     std::cout << "Failed to allocate requested table" << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    std::memset(table_ptr_, 0, num_bytes);
}
void TransposTable::Store(
                            ZobristKey  key,
                            Score       eval,
                            Move        best,
                            U8          depth,
                            BoundType   bound
                          ) const {
    assert(num_elements_ != 0);

    // HashEntry* entry = &table_ptr_[key % num_elements_];
    HashEntry* entry = &table_ptr_[index(key, num_elements_)];

    #if DEBUG_TRANPOSITION_TABLE == 1
    PRINTNL(std::format("key: {}, num_elms: {}, index accessed: {}", key, num_elements_, key % num_elements_));
    #endif

    entry->key_   = key;
    entry->eval_  = eval;
    entry->best_  = best;
    entry->depth_ = depth;
    entry->bound_ = bound;
}
HashEntry const* TransposTable::Probe(ZobristKey key)const
{
    assert(num_elements_ != 0);
    // const HashEntry* entry = &table_ptr_[key % num_elements_];
    const HashEntry* entry = &table_ptr_[index(key, num_elements_)];
    return (entry->key_ == key) ? entry : nullptr;
}
void TransposTable::Clear() const {
    std::memset(table_ptr_, 0, sizeof(HashEntry) * num_elements_);
}
