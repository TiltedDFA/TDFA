#include "MoveGen.hpp"
using Magics::Shift;

static inline std::array<std::array<std::array<BitBoard, 2187>, 4>, 64> PrecomputeAttacks()
{
    std::array<std::array<std::array<BitBoard, 2187>, 4>, 64> result{};
    for(U8 sq = 0; sq < 64; ++sq)
    {
        for(U16 us = 0; us < 256; ++us)
        {
            for(U16 them = 0; them < 256; ++them)
            {
                //skipping useless blocker configurations
                if(us & them || (((~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) & ((~us) & Magics::BBRankOf(sq) || them & Magics::BBRankOf(sq)))) continue;

                BitBoard rank_attacks = 0ull;
                BitBoard file_attacks = 0ull;
                BitBoard diag_attacks = 0ull;
                BitBoard anti_diag_attacks = 0ull;

                const U8 rank_combined = (us | them) & ~Magics::BBFileOf(sq);
                U8 other_combined = (us | them) & ~Magics::BBRankOf(sq);

                const U8 rankofsq = Magics::RankOf(sq);
                const U8 fileofsq = Magics::FileOf(sq);

                if(us & Magics::BBFileOf(sq))
                {
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        rank_attacks |= Magics::SqToBB(sq + (current_file - fileofsq));
                        if((them >> current_file) & 1) break; //their piece
                    }
                    for(int8_t current_file = fileofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        rank_attacks |= Magics::SqToBB(sq - (fileofsq - current_file));
                        if((them >> current_file) & 1) break;
                    }
                    const U16 p1 = Magics::base_2_to_3_us[fileofsq][us & ~Magics::BBFileOf(sq)];
                    const U16 p2 = 2 * Magics::base_2_to_3_us[fileofsq][them];
                    assert((p1 + p2 ) <= 2187);
                    result.at(sq).at(Rank).at(p1 + p2) = rank_attacks;
                }

                if(us & Magics::BBRankOf(sq))
                {
                    for(int8_t current_file = rankofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break; //our piece
                        if(!((other_combined >> current_file) & 1)) //empty
                        {
                            file_attacks |= Magics::SqToBB(sq + 8 * (current_file - rankofsq));

                            if(Magics::ValidSq(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::SqToBB(sq + 9 * (current_file - rankofsq));
                            if(Magics::ValidSq(sq + 7 * (current_file - rankofsq)))
                                anti_diag_attacks |= Magics::SqToBB(sq + 7 * (current_file - rankofsq));
                            continue;
                        }
                        if((them >> current_file) & 1) //their piece
                        {
                            file_attacks |= Magics::SqToBB(sq + 8 * (current_file - rankofsq));

                            if(Magics::ValidSq(sq + 9 * (current_file - rankofsq)))
                                diag_attacks |= Magics::SqToBB(sq + 9 * (current_file - rankofsq));
                            if(Magics::ValidSq(sq + 7 * (current_file - rankofsq)))
                                anti_diag_attacks |= Magics::SqToBB(sq + 7 * (current_file - rankofsq));
                            break;
                        }
                    }
                    for(int8_t current_file = rankofsq - 1; current_file > - 1 ; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        if(!((other_combined >> current_file) & 1))
                        {
                            file_attacks |= Magics::SqToBB(sq - 8 * (rankofsq - current_file));

                            if(Magics::ValidSq(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::SqToBB(sq - 9 * (rankofsq - current_file));
                            if(Magics::ValidSq(sq - 7 * (rankofsq - current_file)))
                                anti_diag_attacks |= Magics::SqToBB(sq - 7 * (rankofsq - current_file));
                            continue;
                        }
                        if((them >> current_file) & 1)
                        {
                            file_attacks |= Magics::SqToBB(sq - 8 * (rankofsq - current_file));

                            if(Magics::ValidSq(sq - 9 * (rankofsq - current_file)))
                                diag_attacks |= Magics::SqToBB(sq - 9 * (rankofsq - current_file));
                            if(Magics::ValidSq(sq - 7 * (rankofsq - current_file)))
                                anti_diag_attacks |= Magics::SqToBB(sq - 7 * (rankofsq - current_file));
                            break;
                        }
                    }

                    diag_attacks      &= Magics::SLIDING_ATTACKS_MASK[sq][(int)Diagonal];
                    anti_diag_attacks &= Magics::SLIDING_ATTACKS_MASK[sq][(int)AntiDiagonal];

                    const U16 p1 = Magics::base_2_to_3_us[rankofsq][us & ~Magics::BBRankOf(sq)];
                    const U16 p2 = 2 * Magics::base_2_to_3_us[rankofsq][them];
                    assert((p1 + p2 ) <= 2187);
                    result.at(sq).at((U8)File           ).at(p1 + p2) = file_attacks;
                    result.at(sq).at((U8)Diagonal       ).at(p1 + p2) = diag_attacks;
                    result.at(sq).at((U8)AntiDiagonal   ).at(p1 + p2) = anti_diag_attacks;
                }
            }
        }
    }
    return result;
}

std::array<std::array<std::array<BitBoard, 2187>, 4>, 64> SLIDING_ATTACKS = PrecomputeAttacks();

static inline std::array<std::array<std::array<U8, 2187>, 4>, 64> PrecomputeEndpoints()
{
    std::array<std::array<std::array<U8, 2187>, 4>, 64> result{};
    for(U8 sq = 0; sq < 64; ++sq)
    {
        for(U16 us = 0; us < 256; ++us)
        {
            for(U16 them = 0; them < 256; ++them)
            {
                //skipping useless blocker configurations
                if(us & them || (((~us) & Magics::BBFileOf(sq) || them & Magics::BBFileOf(sq)) & ((~us) & Magics::BBRankOf(sq) || them & Magics::BBRankOf(sq)))) continue;

                const U8 rank_combined = (us | them) & ~Magics::BBFileOf(sq);
                U8 other_combined = (us | them) & ~Magics::BBRankOf(sq);

                const U8 rankofsq = Magics::RankOf(sq);
                const U8 fileofsq = Magics::FileOf(sq);

                // Rank direction: file-based index, positive=East, negative=West
                if(us & Magics::BBFileOf(sq))
                {
                    U8 pos_offset = 0;
                    U8 pos_cap = 0;
                    U8 neg_offset = 0;
                    U8 neg_cap = 0;

                    // East (positive)
                    for(int8_t current_file = fileofsq + 1; current_file < 8; ++current_file)
                    {
                        if((us >> current_file) & 1) break;
                        pos_offset++;
                        if((them >> current_file) & 1) { pos_cap = 1; break; }
                    }
                    // West (negative)
                    for(int8_t current_file = fileofsq - 1; current_file > -1; --current_file)
                    {
                        if((us >> current_file) & 1) break;
                        neg_offset++;
                        if((them >> current_file) & 1) { neg_cap = 1; break; }
                    }

                    const U8 ep_byte = (neg_cap << 7) | (pos_cap << 6) | (neg_offset << 3) | pos_offset;
                    const U16 p1 = Magics::base_2_to_3_us[fileofsq][us & ~Magics::BBFileOf(sq)];
                    const U16 p2 = 2 * Magics::base_2_to_3_us[fileofsq][them];
                    assert((p1 + p2) <= 2187);
                    result.at(sq).at(Rank).at(p1 + p2) = ep_byte;
                }

                // File/Diagonal/AntiDiagonal: rank-based index
                if(us & Magics::BBRankOf(sq))
                {
                    // Geometric limits for each direction
                    const U8 diag_max_pos   = std::min(U8(7 - rankofsq), U8(7 - fileofsq)); // NE
                    const U8 diag_max_neg   = std::min(rankofsq, fileofsq);                   // SW
                    const U8 adiag_max_pos  = std::min(U8(7 - rankofsq), fileofsq);           // NW
                    const U8 adiag_max_neg  = std::min(rankofsq, U8(7 - fileofsq));           // SE

                    // Helper lambda: compute endpoint for a single sub-direction along rank trace
                    // max_steps = geometric limit for this direction
                    // returns (offset, is_capture)
                    auto compute_sub = [&](int start, int end, int step, U8 max_steps) -> std::pair<U8, U8>
                    {
                        U8 offset = 0;
                        U8 cap = 0;
                        for(int cr = start; step > 0 ? cr < end : cr > end; cr += step)
                        {
                            if(offset >= max_steps) break; // hit board edge for this direction
                            if((us >> cr) & 1) break; // blocked by friendly
                            if(!((other_combined >> cr) & 1))
                            {
                                offset++;
                                continue;
                            }
                            if((them >> cr) & 1)
                            {
                                offset++;
                                cap = 1;
                                break;
                            }
                        }
                        return {offset, cap};
                    };

                    // File: positive=North (max: 7-rank), negative=South (max: rank)
                    auto [file_pos_offset, file_pos_cap] = compute_sub(rankofsq + 1, 8, 1, U8(7 - rankofsq));
                    auto [file_neg_offset, file_neg_cap] = compute_sub(rankofsq - 1, -1, -1, rankofsq);

                    // Diagonal: positive=NE, negative=SW
                    auto [diag_pos_offset, diag_pos_cap] = compute_sub(rankofsq + 1, 8, 1, diag_max_pos);
                    auto [diag_neg_offset, diag_neg_cap] = compute_sub(rankofsq - 1, -1, -1, diag_max_neg);

                    // AntiDiagonal: positive=NW, negative=SE
                    auto [adiag_pos_offset, adiag_pos_cap] = compute_sub(rankofsq + 1, 8, 1, adiag_max_pos);
                    auto [adiag_neg_offset, adiag_neg_cap] = compute_sub(rankofsq - 1, -1, -1, adiag_max_neg);

                    const U16 p1 = Magics::base_2_to_3_us[rankofsq][us & ~Magics::BBRankOf(sq)];
                    const U16 p2 = 2 * Magics::base_2_to_3_us[rankofsq][them];
                    assert((p1 + p2) <= 2187);

                    const U8 file_ep = (file_neg_cap << 7) | (file_pos_cap << 6) | (file_neg_offset << 3) | file_pos_offset;
                    const U8 diag_ep = (diag_neg_cap << 7) | (diag_pos_cap << 6) | (diag_neg_offset << 3) | diag_pos_offset;
                    const U8 adiag_ep = (adiag_neg_cap << 7) | (adiag_pos_cap << 6) | (adiag_neg_offset << 3) | adiag_pos_offset;

                    result.at(sq).at((U8)File).at(p1 + p2) = file_ep;
                    result.at(sq).at((U8)Diagonal).at(p1 + p2) = diag_ep;
                    result.at(sq).at((U8)AntiDiagonal).at(p1 + p2) = adiag_ep;
                }
            }
        }
    }
    return result;
}

std::array<std::array<std::array<U8, 2187>, 4>, 64> SLIDING_ENDPOINTS = PrecomputeEndpoints();

static inline std::array<std::array<std::array<ray_moves, 256>, 4>, 64> PrecomputeMoveLookup()
{
    std::array<std::array<std::array<ray_moves, 256>, 4>, 64> result{};
    for(U8 sq = 0; sq < 64; ++sq)
    {
        const U8 rank = Magics::RankOf(sq);
        const U8 file = Magics::FileOf(sq);

        for(U8 dir = 0; dir < 4; ++dir)
        {
            const int pos_step = Magics::DIRECTION_STEP[dir][0];
            const int neg_step = Magics::DIRECTION_STEP[dir][1];

            // Precompute max steps for positive and negative sub-directions
            U8 max_pos = 0, max_neg = 0;
            switch(dir)
            {
                case File:          max_pos = 7 - rank; max_neg = rank; break;
                case Rank:          max_pos = 7 - file; max_neg = file; break;
                case Diagonal:      max_pos = std::min(U8(7 - rank), U8(7 - file)); max_neg = std::min(rank, file); break;
                case AntiDiagonal:  max_pos = std::min(U8(7 - rank), file); max_neg = std::min(rank, U8(7 - file)); break;
            }

            for(U16 ep = 0; ep < 256; ++ep)
            {
                const U8 pos_offset = ep & 0x07;
                const U8 neg_offset = (ep >> 3) & 0x07;
                const U8 pos_cap    = (ep >> 6) & 1;
                const U8 neg_cap    = (ep >> 7) & 1;

                // Skip geometrically invalid endpoints
                if(pos_offset > max_pos || neg_offset > max_neg) continue;
                // Capture flag requires non-zero offset
                if(pos_cap && pos_offset == 0) continue;
                if(neg_cap && neg_offset == 0) continue;

                ray_moves& rm = result[sq][dir][ep];

                // Positive sub-direction
                for(U8 i = 1; i <= pos_offset; ++i)
                {
                    int to = int(sq) + i * pos_step;
                    assert(to >= 0 && to < 64);
                    MoveType mt = (i == pos_offset && pos_cap) ? mt_Capture : mt_Quiet;
                    rm.moves_[rm.count_++] = Moves::EncodeMove(sq, Sq(to), mt);
                }

                // Negative sub-direction
                for(U8 i = 1; i <= neg_offset; ++i)
                {
                    int to = int(sq) + i * neg_step;
                    assert(to >= 0 && to < 64);
                    MoveType mt = (i == neg_offset && neg_cap) ? mt_Capture : mt_Quiet;
                    rm.moves_[rm.count_++] = Moves::EncodeMove(sq, Sq(to), mt);
                }
            }
        }
    }
    return result;
}

std::array<std::array<std::array<ray_moves, 256>, 4>, 64> MOVE_LOOKUP = PrecomputeMoveLookup();

void MoveGen::WhitePawnMoves(Position const* pos, MoveList* ml) noexcept
{
    const BitBoard pawns = pos->Pieces(White, pt_Pawn);
    if(!pawns) return;
    const BitBoard empty = pos->EmptySqs();
    const BitBoard capturable_squares = pos->Pieces(Black);

    BitBoard bb = Shift<MD::NORTH>(pawns) & empty & ~Magics::RANK_8BB;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(index - 8, index, mt_Quiet));
    }

    bb = Shift<MD::NORTH>(pawns) & empty & Magics::RANK_8BB;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);

        ml->add(Moves::EncodeMove(index - 8, index, mt_QueenPromotion));
        ml->add(Moves::EncodeMove(index - 8, index, mt_RookPromotion));
        ml->add(Moves::EncodeMove(index - 8, index, mt_BishopPromotion));
        ml->add(Moves::EncodeMove(index - 8, index, mt_KnightPromotion));
    }

    bb = Shift<MD::NORTHNORTH>(pawns) & empty & Shift<MD::NORTH>(empty) & Magics::RANK_4BB;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(index - 16, index, mt_Quiet));
    }

    bb = Shift<MD::NORTH_EAST>(pawns) & capturable_squares;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        if(index > 55)
        {
            ml->add(Moves::EncodeMove(index - 9, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index - 9, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index - 9, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index - 9, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 9, index, mt_Capture));
        }
    }

    if((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx - 9, idx, mt_EnPassant));
    }

    bb = Shift<MD::NORTH_WEST>(pawns) & capturable_squares;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        if(index > 55)
        {
            ml->add(Moves::EncodeMove(index - 7, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index - 7, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index - 7, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index - 7, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index - 7, index, mt_Capture));
        }
    }
    if((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_WEST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_WEST>(pawns));
        ml->add(Moves::EncodeMove(idx - 7, idx, mt_EnPassant));
    }
}

void MoveGen::BlackPawnMoves(Position const* pos, MoveList* ml) noexcept
{
    const BitBoard pawns = pos->Pieces(Black, pt_Pawn);
    if(!pawns) return;
    const BitBoard empty = pos->EmptySqs();
    const BitBoard capturable_squares = pos->Pieces(White);

    BitBoard bb = Shift<MD::SOUTH>(pawns) & empty & ~Magics::RANK_1BB;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(index + 8, index, mt_Quiet));
    }

    bb = Shift<MD::SOUTH>(pawns) & empty & Magics::RANK_1BB;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(index + 8, index, mt_QueenPromotion));
        ml->add(Moves::EncodeMove(index + 8, index, mt_RookPromotion));
        ml->add(Moves::EncodeMove(index + 8, index, mt_BishopPromotion));
        ml->add(Moves::EncodeMove(index + 8, index, mt_KnightPromotion));
    }
    bb = Shift<MD::SOUTHSOUTH>(pawns) & empty & Shift<MD::SOUTH>(empty) & Magics::RANK_5BB;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(index + 16, index, mt_Quiet));
    }

    bb = Shift<MD::SOUTH_EAST>(pawns) & capturable_squares;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        if(index < 8)
        {
            ml->add(Moves::EncodeMove(index + 7, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index + 7, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index + 7, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index + 7, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 7, index, mt_Capture));
        }
    }

    if((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_EAST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx + 7, idx, mt_EnPassant));
    }

    bb = Shift<MD::SOUTH_WEST>(pawns) & capturable_squares;
    while (bb)
    {
        const Sq index = Magics::PopNRetLS1B(bb);
        if(index < 8)
        {
            ml->add(Moves::EncodeMove(index + 9, index, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(index + 9, index, mt_RookPromotion));
            ml->add(Moves::EncodeMove(index + 9, index, mt_BishopPromotion));
            ml->add(Moves::EncodeMove(index + 9, index, mt_KnightPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(index + 9, index, mt_Capture));
        }
    }
    if((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_WEST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_WEST>(pawns));
        ml->add(Moves::EncodeMove(idx + 9, idx, mt_EnPassant));
    }

}

// ── Capture-only pawn generation (for QSearch) ───────────────────
void MoveGen::WhitePawnCaptures(Position const* pos, MoveList* ml) noexcept
{
    const BitBoard pawns = pos->Pieces(White, pt_Pawn);
    if (!pawns) return;

    const BitBoard enemies = pos->Pieces(Black);
    BitBoard bb;

    // Quiet push-promotions (rank 8)
    bb = Shift<MD::NORTH>(pawns) & pos->EmptySqs() & Magics::RANK_8BB;
    while (bb)
    {
        const Sq idx = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(idx - 8, idx, mt_QueenPromotion));
        ml->add(Moves::EncodeMove(idx - 8, idx, mt_KnightPromotion));
        ml->add(Moves::EncodeMove(idx - 8, idx, mt_RookPromotion));
        ml->add(Moves::EncodeMove(idx - 8, idx, mt_BishopPromotion));
    }

    // NE captures
    bb = Shift<MD::NORTH_EAST>(pawns) & enemies;
    while (bb)
    {
        const Sq idx = Magics::PopNRetLS1B(bb);
        if (idx > 55)
        {
            ml->add(Moves::EncodeMove(idx - 9, idx, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(idx - 9, idx, mt_KnightPromotion));
            ml->add(Moves::EncodeMove(idx - 9, idx, mt_RookPromotion));
            ml->add(Moves::EncodeMove(idx - 9, idx, mt_BishopPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(idx - 9, idx, mt_Capture));
        }
    }

    if ((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx - 9, idx, mt_EnPassant));
    }

    // NW captures
    bb = Shift<MD::NORTH_WEST>(pawns) & enemies;
    while (bb)
    {
        const Sq idx = Magics::PopNRetLS1B(bb);
        if (idx > 55)
        {
            ml->add(Moves::EncodeMove(idx - 7, idx, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(idx - 7, idx, mt_KnightPromotion));
            ml->add(Moves::EncodeMove(idx - 7, idx, mt_RookPromotion));
            ml->add(Moves::EncodeMove(idx - 7, idx, mt_BishopPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(idx - 7, idx, mt_Capture));
        }
    }

    if ((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_WEST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_3BB) & Shift<MD::NORTH_WEST>(pawns));
        ml->add(Moves::EncodeMove(idx - 7, idx, mt_EnPassant));
    }
}

void MoveGen::BlackPawnCaptures(Position const* pos, MoveList* ml) noexcept
{
    const BitBoard pawns = pos->Pieces(Black, pt_Pawn);
    if (!pawns) return;

    const BitBoard enemies = pos->Pieces(White);
    BitBoard bb;

    // Quiet push-promotions (rank 1)
    bb = Shift<MD::SOUTH>(pawns) & pos->EmptySqs() & Magics::RANK_1BB;
    while (bb)
    {
        const Sq idx = Magics::PopNRetLS1B(bb);
        ml->add(Moves::EncodeMove(idx + 8, idx, mt_QueenPromotion));
        ml->add(Moves::EncodeMove(idx + 8, idx, mt_KnightPromotion));
        ml->add(Moves::EncodeMove(idx + 8, idx, mt_RookPromotion));
        ml->add(Moves::EncodeMove(idx + 8, idx, mt_BishopPromotion));
    }

    // SE captures
    bb = Shift<MD::SOUTH_EAST>(pawns) & enemies;
    while (bb)
    {
        const Sq idx = Magics::PopNRetLS1B(bb);
        if (idx < 8)
        {
            ml->add(Moves::EncodeMove(idx + 7, idx, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(idx + 7, idx, mt_KnightPromotion));
            ml->add(Moves::EncodeMove(idx + 7, idx, mt_RookPromotion));
            ml->add(Moves::EncodeMove(idx + 7, idx, mt_BishopPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(idx + 7, idx, mt_Capture));
        }
    }

    if ((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_EAST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_EAST>(pawns));
        ml->add(Moves::EncodeMove(idx + 7, idx, mt_EnPassant));
    }

    // SW captures
    bb = Shift<MD::SOUTH_WEST>(pawns) & enemies;
    while (bb)
    {
        const Sq idx = Magics::PopNRetLS1B(bb);
        if (idx < 8)
        {
            ml->add(Moves::EncodeMove(idx + 9, idx, mt_QueenPromotion));
            ml->add(Moves::EncodeMove(idx + 9, idx, mt_KnightPromotion));
            ml->add(Moves::EncodeMove(idx + 9, idx, mt_RookPromotion));
            ml->add(Moves::EncodeMove(idx + 9, idx, mt_BishopPromotion));
        }
        else
        {
            ml->add(Moves::EncodeMove(idx + 9, idx, mt_Capture));
        }
    }

    if ((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_WEST>(pawns)) [[unlikely]]
    {
        const Sq idx = Magics::FindLS1B((pos->EnPasBB() & ~Magics::RANK_6BB) & Shift<MD::SOUTH_WEST>(pawns));
        ml->add(Moves::EncodeMove(idx + 9, idx, mt_EnPassant));
    }
}