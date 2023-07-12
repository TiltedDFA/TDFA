#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <cassert>
#include "../Core/Types.hpp"
#include "../Core/MagicConstants.hpp"
#include "../Core/BitBoard.hpp"
#include "../Core/Move.hpp"
#include "MoveList.hpp"

class MoveGen
{
public:
    MoveGen();
    void UpdateVariables(BitBoard white_pieces, BitBoard black_pieces, BitBoard empty_squares);

    void WhitePawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;
    void BlackPawnMoves(Move** move_list, BitBoard pawns, BitBoard en_passant_target_sq) noexcept;
    void WhiteKnightMoves(Move** move_list, BitBoard knights) noexcept;
    void BlackKnightMoves(Move** move_list, BitBoard knights) noexcept;
private:
    static MoveGen* instance_;
    BitBoard white_pieces_;
    BitBoard black_pieces_;
    BitBoard empty_squares_;
};

#endif // #ifndef MOVEGEN_HPP