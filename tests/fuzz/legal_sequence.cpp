#include "../support/BlindTdfaAdapter.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <string>

namespace
{
using namespace tdfa_test::blind;

[[noreturn]] void fail() { std::abort(); }

constexpr std::array<const char*, 6> roots{
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/2pP4/1p2P3/2N2N2/PPQBBPPP/R3K2R w KQkq - 0 1",
    "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
};
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size == 0)
        return 0;
    try
    {
        const SquareBijection squares;
        RefPosition reference = RefPosition::from_fen(roots[data[0] % roots.size()]);
        const std::size_t plies = std::min<std::size_t>(size - 1, 32);
        for (std::size_t ply = 0; ply < plies; ++ply)
        {
            const std::string fen = reference.fen();
            Position generation(fen);
            MoveList list;
            generate_legal(generation, reference.turn, list);
            const auto observed = observe_moves(list, squares);
            const auto expected = uci_set(reference.legal_moves(reference.turn));
            std::set<std::string> actual;
            for (const auto& move : observed)
                actual.insert(move.uci);
            if (actual != expected)
                fail();
            if (observed.empty())
                break;

            const ObservedMove chosen = observed[data[ply + 1] % observed.size()];
            const LogicalMove logical = reference.legal_move_from_uci(chosen.uci);
            const RefPosition child = reference.after(logical);
            Position transition(fen);
            const PublicPlacementSnapshot before = public_placement(transition);
            transition.MakeMove(chosen.raw);
            if (!placement_equals(transition, child, squares))
                fail();
            transition.UnmakeMove(chosen.raw);
            if (!(public_placement(transition) == before))
                fail();
            reference = child;
        }
    }
    catch (...)
    {
        fail();
    }
    return 0;
}
