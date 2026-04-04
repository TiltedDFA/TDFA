#include <iostream>
#include <vector>

#include "MagicConstants.hpp"
#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Testing.hpp"
#include "Debug.hpp"
#include "Search.hpp"
#include "Timer.hpp"
#include "Uci.hpp"
#include "TranspositionTable.hpp"
#include "ZobristConstants.hpp"

static const char* BENCH_FENS[] = {
    // Startpos & well-known test positions
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        // Rook endgames & rook-heavy
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1",
        "7k/RR6/8/8/8/8/rr6/7K w - - 0 1",
        "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
        "R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1",
        "7k/RR6/8/8/8/8/rr6/7K b - - 0 1",
        // Bishop pairs
        "B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1",
        "8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1",
        "B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1",
        "8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1",
        // Queen + rook middlegames
        "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
        "r1bqkb1r/pppppppp/2n2n2/8/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1",
        "r1bqkbnr/pppppppp/2n5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkb1r/pp1p1ppp/2p2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
        "r1bqk2r/ppp2ppp/2n2n2/1B1pp3/1b2P3/2NP1N2/PPP2PPP/R1BQK2R w KQkq - 0 1",
        "r2qkb1r/ppp2ppp/2np1n2/4p1B1/2B1P3/2N2N2/PPP2PPP/R2QK2R w KQkq - 0 1",
        // Open positions with active sliders
        "r1b1k2r/ppppqppp/2n2n2/4p3/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - 0 1",
        "r2q1rk1/ppp2ppp/2np1n2/2b1p1B1/2B1P3/2NP1N2/PPP2PPP/R2Q1RK1 w - - 0 1",
        "r1bq1rk1/pppn1ppp/4pn2/3p4/1bPP4/2NBPN2/PP3PPP/R1BQ1RK1 w - - 0 1",
        "r2qr1k1/ppp2ppp/2n1bn2/3p4/3P1B2/2PB1N2/PP3PPP/R2QR1K1 w - - 0 1",
        "r3r1k1/ppq2ppp/2p1bn2/3p4/3P1B2/2PB1N2/PP2QPPP/R4RK1 w - - 0 1",
        // Rich middlegames with multiple sliders
        "r1bq1rk1/pp2ppbp/2np1np1/8/3NP3/2N1BP2/PPPQ2PP/R3KB1R w KQ - 0 1",
        "r2q1rk1/1pp1bppp/p1np1n2/4p1B1/4P3/2NP1N1P/PPP2PP1/R2QR1K1 w - - 0 1",
        "r1bqk2r/pp1pppbp/2n2np1/2p5/4P3/2N2NP1/PPPP1PBP/R1BQK2R w KQkq - 0 1",
        "r1bq1rk1/ppp1npbp/3p1np1/3Pp3/2P1P3/2N2NP1/PP3PBP/R1BQ1RK1 w - - 0 1",
        "r1bqr1k1/pp1n1ppp/2pb1n2/3p4/3P1B2/2PB1N2/PP1N1PPP/R2QR1K1 w - - 0 1",
        // Queen vs queen with support
        "r2q1rk1/pppb1ppp/2n1pn2/3p4/3P4/2NBPN2/PPP2PPP/R1BQ1RK1 w - - 0 1",
        "r2qr1k1/pp1bbppp/2n1pn2/3p4/3P4/2NBPN2/PP1B1PPP/R2QR1K1 w - - 0 1",
        "r3k2r/pp1b1ppp/1qn1pn2/2pp4/3P4/2PBPN2/PP1N1PPP/R1BQK2R w KQkq - 0 1",
        "r1b1k2r/pp1n1ppp/2p1pn2/q2p4/1bPP4/2NBPN2/PP3PPP/R1BQK2R w KQkq - 0 1",
        "r2qk2r/pp1b1ppp/2n1pn2/2pp4/1bPP4/2NBPN2/PP3PPP/R1BQK2R w KQkq - 0 1",
        // Fianchetto positions
        "rnbq1rk1/pp2ppbp/2pp1np1/8/2PPP3/2N2NP1/PP3PBP/R1BQK2R w KQ - 0 1",
        "r1bq1rk1/pppnnpbp/3p2p1/3Pp3/2P1P3/2NN2P1/PP3PBP/R1BQ1RK1 w - - 0 1",
        "rnbq1rk1/ppp1ppbp/3p1np1/8/2PPP3/2N5/PP2BPPP/R1BQK1NR w KQ - 0 1",
        "r1bq1rk1/ppp2pbp/2np1np1/4p3/2PPP3/2N2NP1/PP3PBP/R1BQ1RK1 w - - 0 1",
        "rnbqk2r/pppp1ppp/5n2/2b1p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 0 1",
        // Double rook endgames
        "3r1rk1/pp3ppp/2p5/8/3P4/2P2N2/PP3PPP/3R1RK1 w - - 0 1",
        "3r1rk1/ppp2ppp/8/3p4/3P4/5N2/PPP2PPP/3R1RK1 w - - 0 1",
        "r4rk1/ppp2ppp/8/3p4/3P4/8/PPP2PPP/R4RK1 w - - 0 1",
        "1r3rk1/ppp2ppp/8/8/8/8/PPP2PPP/1R3RK1 w - - 0 1",
        "2r2rk1/pp3ppp/8/3p4/3P4/2P5/PP3PPP/2R2RK1 w - - 0 1",
        // Rook + bishop combos
        "r1b2rk1/pp3ppp/2p2n2/3p4/3P4/2P2N2/PP2BPPP/R1B2RK1 w - - 0 1",
        "r4rk1/pp1b1ppp/2p2n2/3p4/3P1B2/2P2N2/PP3PPP/R4RK1 w - - 0 1",
        "r1b2rk1/ppp2ppp/2n5/3pN3/3P4/8/PPP2PPP/R1B2RK1 w - - 0 1",
        "r4rk1/ppb2ppp/2p2n2/3p4/3P1B2/5N2/PPP2PPP/R4RK1 w - - 0 1",
        "r1b2rk1/pp3ppp/2n1p3/3p4/3P4/2PB1N2/PP3PPP/R4RK1 w - - 0 1",
        // Queen + rook power positions
        "r2q1rk1/pp2ppbp/2p2np1/8/3PP3/2N2N2/PPP1BPPP/R2QR1K1 w - - 0 1",
        "r3r1k1/ppq2ppp/2pb1n2/8/3P4/2PB1N2/PP2QPPP/R4RK1 w - - 0 1",
        "r2qr1k1/pp3ppp/2p1bn2/3pN3/3P4/2P5/PP1QBPPP/R4RK1 w - - 0 1",
        "r4rk1/pp1qppbp/2np1np1/8/3PP3/2N1BN2/PPPQ1PPP/R3R1K1 w - - 0 1",
        "r2q1rk1/ppp1bppp/2n1p3/3pN3/3P4/2P5/PP1QBPPP/R4RK1 w - - 0 1",
        // Opposite side castling
        "r3kb1r/pp1n1ppp/2p1pn2/q2p4/3P1B2/2PBPN2/PP1N1PPP/R2QK2R w KQkq - 0 1",
        "2kr3r/ppp2ppp/2n1bn2/1B2p3/4P3/2NP1N2/PPP2PPP/R3K2R w KQ - 0 1",
        "r3k2r/1pp2ppp/p1nb1n2/4p1B1/4P3/2NP1N2/PPP2PPP/2KR3R w kq - 0 1",
        "r3k2r/pp1bbppp/2n1pn2/3p4/3P4/2NBPN2/PPP2PPP/R1B1K2R w KQkq - 0 1",
        "r3kb1r/1pp2ppp/p1n1bn2/4p3/4P3/2NB1N2/PPP2PPP/R1B1K2R w KQkq - 0 1",
        // Tactical middlegames
        "r1bq1rk1/pp1nppbp/2pp1np1/8/2PPP3/2N1BN2/PP2BPPP/R2Q1RK1 w - - 0 1",
        "r2qkb1r/1pp2ppp/p1n1pn2/3p1b2/3P4/2NBPN2/PPP2PPP/R1BQK2R w KQkq - 0 1",
        "r1bqk2r/ppp2ppp/2n1p3/3pP3/1bPP4/2N2N2/PP3PPP/R1BQKB1R w KQkq - 0 1",
        "r1b1kb1r/ppqp1ppp/2n1pn2/2p5/4P3/2N2NP1/PPPP1PBP/R1BQK2R w KQkq - 0 1",
        "r1bqkb1r/pp1n1ppp/2p1pn2/3p4/2PP4/2NBPN2/PP3PPP/R1BQK2R w KQkq - 0 1",
        // Complex with many sliders active
        "r1bqr1k1/ppp2pbp/2n2np1/3p4/2PP4/2N1PN2/PP2BPPP/R1BQ1RK1 w - - 0 1",
        "r2q1rk1/pb1nbppp/1p2pn2/2p5/2PP4/2N1PN2/PPQ1BPPP/R1B2RK1 w - - 0 1",
        "r1bq1rk1/ppp1npbp/3p1np1/3Pp3/1PP1P3/2N2NP1/P4PBP/R1BQ1RK1 w - - 0 1",
        "r1b1r1k1/pp1n1pbp/2pp1np1/q3p3/2PPP3/2N1BNP1/PP2QPBP/R4RK1 w - - 0 1",
        "r2qr1k1/pppb1ppp/2n1bn2/3pp3/3PP3/2NB1N2/PPP1BPPP/R2QR1K1 w - - 0 1",
        // Heavy piece positions
        "2rqr1k1/pp3ppp/2p1bn2/3p4/3P1B2/2PB1N2/PP2QPPP/2R1R1K1 w - - 0 1",
        "r4rk1/1bq2ppp/pp1ppn2/2n5/2P5/1PN1PN2/PBQ2PPP/R4RK1 w - - 0 1",
        "2rq1rk1/pp1b1ppp/2n1pn2/3p4/3P1B2/2PBPN2/PP3PPP/2RQ1RK1 w - - 0 1",
        "r2q1rk1/1ppbbppp/p1n1pn2/3p4/3PP3/2NB1N2/PPP1BPPP/R2Q1RK1 w - - 0 1",
        "r1bqr1k1/pp3pbp/2np1np1/2p1p3/4P3/2PP1NP1/PP1N1PBP/R1BQR1K1 w - - 0 1",
        // Semi-open files
        "r3r1k1/pp1q1ppp/2pb1n2/3p4/3P1B2/2PB1N2/PP2QPPP/R3R1K1 w - - 0 1",
        "r4rk1/1ppq1ppp/p1n1bn2/3pp3/3PP3/2NB1N2/PPP1QPPP/R4RK1 w - - 0 1",
        "r3r1k1/ppqb1ppp/2p2n2/3pp3/3PP3/2PB1N2/PP2QPPP/R3R1K1 w - - 0 1",
        "2r1r1k1/pp1q1ppp/2n1bn2/3p4/3P4/2PB1N2/PP1NQPPP/2R1R1K1 w - - 0 1",
        "r2q1rk1/1pp2ppp/p1nb1n2/3pp1B1/3PP3/2NB1N2/PPP2PPP/R2Q1RK1 w - - 0 1",
        // Italian/Spanish style
        "r1bqk2r/pppp1ppp/2n2n2/1Bb1p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
        "r1bqk2r/pppp1ppp/2n2n2/2b1p3/1PB1P3/5N2/P1PP1PPP/RNBQK2R w KQkq - 0 1",
        "r1bqk2r/2ppbppp/p1n2n2/1p2p3/4P3/1B3N2/PPPP1PPP/RNBQ1RK1 w kq - 0 1",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
        "r1bqk2r/ppppbppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 0 1",
        // Sicilian structures
        "r1b1kb1r/pp3ppp/1qn1pn2/2pp4/3P4/2PBPN2/PP1N1PPP/R1BQK2R w KQkq - 0 1",
        "r1bqkb1r/1p2pppp/p1np1n2/8/3NP3/2N5/PPP1BPPP/R1BQK2R w KQkq - 0 1",
        "r1bqk2r/pp2ppbp/2np1np1/8/3NP3/2N1BP2/PPP3PP/R2QKB1R w KQkq - 0 1",
        "rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 1",
        "r1b1kb1r/ppqp1ppp/2n1pn2/2p5/4P3/2N2NP1/PPPPQPBP/R1B1K2R w KQkq - 0 1",
        // QGD / Slav structures
        "r1bqkb1r/ppp2ppp/2n1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - 0 1",
        "r1bqkb1r/pp3ppp/2n1pn2/2pp4/2PP4/2N1PN2/PP3PPP/R1BQKB1R w KQkq - 0 1",
        "r1bqk2r/pp1nbppp/2p1pn2/3p4/2PP4/2N1PN2/PP2BPPP/R1BQK2R w KQkq - 0 1",
        "r1bqkb1r/pp1n1ppp/2p1pn2/3p4/2PP4/2NBPN2/PP3PPP/R1BQK2R w KQkq - 0 1",
        "r1bq1rk1/pp1n1ppp/2pbpn2/3p4/2PP4/2NBPN2/PP3PPP/R1BQ1RK1 w - - 0 1",
};
constexpr int NUM_BENCH_FENS = sizeof(BENCH_FENS) / sizeof(BENCH_FENS[0]);

static void BenchMovegen(int argc, char* argv[])
{
    constexpr int ITERS = 2'000'000;

    std::vector<Position> positions;
    positions.reserve(NUM_BENCH_FENS);
    for(int i = 0; i < NUM_BENCH_FENS; ++i)
        positions.emplace_back(BENCH_FENS[i]);
    std::cout << NUM_BENCH_FENS << " positions loaded.\n";

    auto run_bench = [&](const char* label, auto gen_white, auto gen_black) {
        U64 total_moves = 0;
        U64 time_us = 1;
        {
            Timer<std::chrono::microseconds> t(&time_us);
            for(int iter = 0; iter < ITERS; ++iter)
            {
                for(int p = 0; p < NUM_BENCH_FENS; ++p)
                {
                    MoveList ml{};
                    if(positions[p].ColourToMove() == White)
                        gen_white(&positions[p], &ml);
                    else
                        gen_black(&positions[p], &ml);
                    total_moves += ml.len();
                }
            }
        }
        const double secs = double(time_us) / 1'000'000.0;
        const double mps = double(total_moves) / secs;
        std::cout << std::format("{}: {:.3f}s, {:.1f} moves/sec\n", label, secs, mps);
    };

    // Warm up both systems
    for(int i = 0; i < 100'000; ++i) {
        MoveList ml{};
        MoveGen::GeneratePseudoLegalMoves<White>(&positions[0], &ml);
        MoveGen::OldGeneratePseudoLegalMoves<White>(&positions[0], &ml);
    }

    // Select system via argv[2]: "new", "old", or "ab" (default)
    const char* mode = (argc > 2) ? argv[2] : "ab";
    if(std::string_view(mode) == "new")
    {
        run_bench("NEW (endpoint)",
            MoveGen::GeneratePseudoLegalMoves<White>,
            MoveGen::GeneratePseudoLegalMoves<Black>);
    }
    else if(std::string_view(mode) == "old")
    {
        run_bench("OLD (move_info)",
            MoveGen::OldGeneratePseudoLegalMoves<White>,
            MoveGen::OldGeneratePseudoLegalMoves<Black>);
    }
    else
    {
        for(int round = 0; round < 3; ++round)
        {
            std::cout << std::format("--- Round {} ---\n", round + 1);
            run_bench("NEW (endpoint)",
                MoveGen::GeneratePseudoLegalMoves<White>,
                MoveGen::GeneratePseudoLegalMoves<Black>);
            run_bench("OLD (move_info)",
                MoveGen::OldGeneratePseudoLegalMoves<White>,
                MoveGen::OldGeneratePseudoLegalMoves<Black>);
        }
    }
}

static void BenchLatency(const char* fens[], int num_fens)
{
    constexpr int ITERS = 100'000;

    std::vector<Position> positions;
    positions.reserve(num_fens);
    for(int i = 0; i < num_fens; ++i)
        positions.emplace_back(fens[i]);

    // Test 1: FEN parsing only
    {
        U64 time_ns = 1;
        {
            Timer<std::chrono::nanoseconds> t(&time_ns);
            for(int iter = 0; iter < ITERS; ++iter)
                for(int i = 0; i < num_fens; ++i)
                    Position pos(fens[i]);
        }
        double ns_per = double(time_ns) / double(ITERS * num_fens);
        std::cout << std::format("FEN parse:      {:.1f} ns/position\n", ns_per);
    }

    // Test 2: Movegen only (position pre-loaded)
    {
        U64 time_ns = 1;
        U64 total_moves = 0;
        {
            Timer<std::chrono::nanoseconds> t(&time_ns);
            for(int iter = 0; iter < ITERS; ++iter)
                for(int i = 0; i < num_fens; ++i)
                {
                    MoveList ml{};
                    if(positions[i].ColourToMove() == White)
                        MoveGen::GeneratePseudoLegalMoves<White>(&positions[i], &ml);
                    else
                        MoveGen::GeneratePseudoLegalMoves<Black>(&positions[i], &ml);
                    total_moves += ml.len();
                }
        }
        double ns_per = double(time_ns) / double(ITERS * num_fens);
        std::cout << std::format("Movegen only:   {:.1f} ns/call ({} avg moves)\n", ns_per, total_moves / (ITERS * num_fens));
    }

    // Test 3: FEN parse + movegen combined
    {
        U64 time_ns = 1;
        U64 total_moves = 0;
        {
            Timer<std::chrono::nanoseconds> t(&time_ns);
            for(int iter = 0; iter < ITERS; ++iter)
                for(int i = 0; i < num_fens; ++i)
                {
                    Position pos(fens[i]);
                    MoveList ml{};
                    if(pos.ColourToMove() == White)
                        MoveGen::GeneratePseudoLegalMoves<White>(&pos, &ml);
                    else
                        MoveGen::GeneratePseudoLegalMoves<Black>(&pos, &ml);
                    total_moves += ml.len();
                }
        }
        double ns_per = double(time_ns) / double(ITERS * num_fens);
        std::cout << std::format("Parse+movegen:  {:.1f} ns/call\n", ns_per);
    }
}

static void BenchUci()
{
    // Measure UCI command parsing + position setup latency (no search)
    constexpr int ITERS = 500'000;
    const char* cmds[] = {
        "position startpos",
        "position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6",
        "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "position fen r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 moves c3d5",
    };
    constexpr int NUM_CMDS = 4;

    // Suppress cout during bench
    auto* old_buf = std::cout.rdbuf(nullptr);

    Uci uci;
    U64 time_ns = 1;
    {
        Timer<std::chrono::nanoseconds> t(&time_ns);
        for(int iter = 0; iter < ITERS; ++iter)
        {
            for(int c = 0; c < NUM_CMDS; ++c)
            {
                std::string inp(cmds[c]);
                // Simulate what Loop() does: split + dispatch
                // We call HandlePosition directly via a full parse cycle
                // by feeding through the public interface indirectly
                // Actually, just measure SplitArgs + position handling
            }
        }
    }
    std::cout.rdbuf(old_buf);

    // Better approach: measure position command round-trips via pipe
    // Instead, let's just time the critical path directly
    std::cout << "UCI latency benchmark (position commands):\n";

    // Test: parse + apply "position startpos moves ..."
    {
        Position pos(STARTPOS);
        U64 t_ns = 1;
        {
            Timer<std::chrono::nanoseconds> t(&t_ns);
            for(int iter = 0; iter < ITERS; ++iter)
            {
                pos.ImportFen(STARTPOS);
                pos.HashCurrentPostion();
                // Apply 8 moves
                pos.MakeMove(UTIL::UciToMove("e2e4", pos));
                pos.MakeMove(UTIL::UciToMove("e7e5", pos));
                pos.MakeMove(UTIL::UciToMove("g1f3", pos));
                pos.MakeMove(UTIL::UciToMove("b8c6", pos));
                pos.MakeMove(UTIL::UciToMove("f1b5", pos));
                pos.MakeMove(UTIL::UciToMove("a7a6", pos));
                pos.MakeMove(UTIL::UciToMove("b5a4", pos));
                pos.MakeMove(UTIL::UciToMove("g8f6", pos));
            }
        }
        double ns_per = double(t_ns) / double(ITERS);
        std::cout << std::format("  startpos + 8 moves: {:.1f} ns/call ({:.1f} ns/move)\n", ns_per, ns_per / 8.0);
    }

    // Test: UciToMove conversion alone
    {
        Position pos(STARTPOS);
        U64 t_ns = 1;
        {
            Timer<std::chrono::nanoseconds> t(&t_ns);
            for(int iter = 0; iter < ITERS * 8; ++iter)
            {
                volatile Move m = UTIL::UciToMove("e2e4", pos);
                (void)m;
            }
        }
        double ns_per = double(t_ns) / double(ITERS * 8);
        std::cout << std::format("  UciToMove:          {:.1f} ns/call\n", ns_per);
    }
}

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);
    if(argc > 1 && std::string_view(argv[1]) == "bench")
    {
        RunBenchmark<false>();
    }
    else if(argc > 1 && std::string_view(argv[1]) == "benchmovegen")
    {
        BenchMovegen(argc, argv);
    }
    else if(argc > 1 && std::string_view(argv[1]) == "benchlatency")
    {
        BenchLatency(BENCH_FENS, NUM_BENCH_FENS);
    }
    else if(argc > 1 && std::string_view(argv[1]) == "benchuci")
    {
        BenchUci();
    }
    else
    {
        Uci uci;
        uci.Loop();
    }
    return 0;
}
