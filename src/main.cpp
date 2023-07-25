#include <iostream>

#include "Core/MagicConstants.hpp"
#include "MoveGen/MoveList.hpp"
#include "MoveGen/MoveGen.hpp"
#include "Core/Testing.hpp"
#include "Core/Debug.hpp"

constexpr unsigned long long b = 0xFF;
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define TEST_FEN_LONG "rnbq1rk1/pp2ppbp/6p1/2pp4/2PPnB2/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8"
#define TEST_FEN_WHITE_SPACE "                            8/8/8/8/4Pp2/8/8/8 w - e3 0 1                              "
#define TEST_FEN_TIT_ONE "8/8/8/8/1r1R2N1/8/8/8 w - - 0 1"
#define TEST_FEN_TIT_TWO "8/8/8/8/8/8/1r2q1N1/8 w - - 0 1"
#define TEST_FEN_TIT_THREE "1R3rp1/8/8/8/8/8/8/8 w - - 0 1"

void PrintBB(BitBoard board, bool mirrored=true)
{
    std::string output{},current_line{};
    for(int row{0}; row < 8; ++row)
    {
        for(int col{0}; col < 8;++col)
        {
            if(((board >> (col + row*8)))&1ull)
            {
                current_line = mirrored ?   current_line + "1 " : "1 " + current_line;
            }
            else
            {
                current_line = mirrored ?   current_line + "0 " : "0 " + current_line;
            }
        }
        current_line += "|" + std::to_string(row + 1) + " \n";
        output = current_line + output;
        current_line = "";
    }                    
    output += "----------------\n";
    output += mirrored ? "A B C D E F G H" : "H G F E D C B A";
    std::cout << output << std::endl;
}
/*
    BB::Position pos{};
    pos.ImportFen(START_FEN);
    MoveList instance;
    MoveGen generator;
    std::cout << instance.First() << "\n";
    std::cout << instance.Last() << "\n";
    for(uint16_t i = 0; i < MAX_MOVES;++i)
    {
        if(i == 198) *(*instance.Current())++ = std::numeric_limits<Move>::max()-1; 
        else *(*instance.Current())++ = i;
    }
    std::cout << instance.Last() << "\n";
    std::cout << instance.Contains(Move(std::numeric_limits<Move>::max())) << '\t' << std::numeric_limits<Move>::max();
    CmpMoveLists(instance,std::vector<Move>());
    generator.UpdateVariables(pos.GetPieces<true>(),pos.GetPieces<false>());
    Debug::PrintEntireBoard(pos.pieces_);
    for(uint8_t i = 0; i < 64;++i)
    {
        std::cout << "Index: " << (int)i << " Rank: " << static_cast<int>(Magics::rank_of(i)) << " File: " << static_cast<int>(Magics::file_of(i)) << std::endl;
    }
    Debug::PrintBB(Magics::CROSS_DIAG);
    Debug::PrintBB(Magics::ANTI_CROSS_DIAG);
    const std::array<std::array<BitBoard, 4>, 64> data = PrecomputeMask();
    for(int i = 0; i < 64;++i)
    {
        for(int j = 0; j < 4;++j)
        {
            std::cout << "SQ: " << i << " DIR: " << j << std::endl;
            Debug::PrintBB(data[i][j]);
        }
    }
    */
int main(void)
{
    /*
    BB::Position pos{};
    pos.ImportFen(TESTFEN9);
    BitBoard pieces = 0x448800;//pos.GetPieces<true>();
    Debug::PrintBB(pieces);
    Debug::PrintBB(Magics::CollapsedFilesIndex(pieces));
    std::cout << Magics::CollapsedFilesIndex(pieces);
    */
    BB::Position pos;
    pos.ImportFen(TEST_FEN_TIT_ONE);
    MoveGen generator;

    uint8_t sq = 27;
    uint16_t p1 = Magics::base_2_to_3[Magics::file_of(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];
    uint16_t p2 = 2 * Magics::base_2_to_3[Magics::file_of(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];
    uint16_t index = p1+p2;
    std::cout << "index = " << index << std::endl;
    auto info = generator.SLIDING_ATTACK_CONFIG.at(sq).at(0).at(p1+p2);
    Debug::PrintBB(pos.GetPieces<true>() | pos.GetPieces<false>());
    Debug::PrintEncodedMovesMoveInfo(info);
    for(int i = 0; i < info.count;++i)
    {
        Debug::ShortPrintEncodedMoveStr(info.encoded_move[i]);
    }

    pos.ImportFen(TEST_FEN_TIT_TWO);

    sq = 9;
    p1 = Magics::base_2_to_3[Magics::file_of(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];
    p2 = 2 * Magics::base_2_to_3[Magics::file_of(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];
    index = p1+p2;
    std::cout << "index = " << index << std::endl;
    info = generator.SLIDING_ATTACK_CONFIG.at(sq).at(0).at(p1+p2);
    Debug::PrintBB(pos.GetPieces<true>() | pos.GetPieces<false>());
    Debug::PrintEncodedMovesMoveInfo(info);
    for(int i = 0; i < info.count;++i)
    {
        Debug::ShortPrintEncodedMoveStr(info.encoded_move[i]);
    }

    pos.ImportFen(TEST_FEN_TIT_THREE);

    sq = 61;
    p1 = Magics::base_2_to_3[Magics::file_of(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<false>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];
    p2 = 2 * Magics::base_2_to_3[Magics::file_of(sq)][Magics::CollapsedFilesIndex(pos.GetPieces<true>() & Magics::SLIDING_ATTACKS_MASK[sq][1])];
    index = p1+p2;
    std::cout << "index = " << index << std::endl;
    info = generator.SLIDING_ATTACK_CONFIG.at(sq).at(0).at(p1+p2);
    Debug::PrintBB(pos.GetPieces<true>() | pos.GetPieces<false>());
    Debug::PrintEncodedMovesMoveInfo(info);
    for(int i = 0; i < info.count;++i)
    {
        Debug::ShortPrintEncodedMoveStr(info.encoded_move[i]);
    }
    /*
    std::cout << Magics::base_2_to_3[0][0x7F] << std::endl;
    */
    return 0;
}
