#include "MagicConstants.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace
{
[[noreturn]] void fail() { std::abort(); }
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    BitBoard board = 0;
    for (std::size_t index = 0; index < size && index < sizeof(board); ++index)
        board |= static_cast<BitBoard>(data[index]) << (index * 8U);
    if (board == 0)
        return 0;

    unsigned expected_count = 0;
    unsigned expected_first = 0;
    unsigned expected_last = 0;
    bool found_first = false;
    for (unsigned bit = 0; bit < 64; ++bit)
    {
        if ((board & (BitBoard{1} << bit)) == 0)
            continue;
        if (!found_first)
        {
            expected_first = bit;
            found_first = true;
        }
        expected_last = bit;
        ++expected_count;
    }

    if (Magics::PopCnt(board) != expected_count ||
        static_cast<unsigned>(Magics::FindLS1B(board)) != expected_first ||
        static_cast<unsigned>(Magics::FindMS1B(board)) != expected_last ||
        Magics::GetLS1B(board) != (BitBoard{1} << expected_first))
        fail();

    const BitBoard least_removed = Magics::PopLS1B(board);
    if (least_removed != (board & ~(BitBoard{1} << expected_first)))
        fail();

    const BitBoard most_removed = Magics::PopMS1B(board);
    if (most_removed != (board & ~(BitBoard{1} << expected_last)))
        fail();
    return 0;
}
