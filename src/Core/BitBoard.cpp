#include "BitBoard.hpp"
#include <array>
#include <charconv>

std::stack<BB::PosInfo> BB::Position::previous_pos_info{};

static constexpr std::string_view RemoveWhiteSpace(std::string_view str)
{
    int start_index{-1};
    int end_index{static_cast<int>(str.size())};
    while(str.at(++start_index) == ' '){}
    while(str.at(--end_index) == ' '){}
    return std::string_view(str.begin() + start_index, str.begin() + end_index + 1);
}
static inline void Split(std::string_view fen, std::array<std::string_view,6>& fen_sections)
{
        int start = 0;
        int end = -1;
        uint8_t current_fen_section = 0;
        while(static_cast<uint64_t>(++end) < fen.size())
        {
            if(fen.at(end) == ' ')
            {
                ++end;
                fen_sections.at(current_fen_section) = std::string_view(fen.begin() + start, fen.begin() + (end - 1));
                ++current_fen_section;
                start = end;
                continue;
            }
        }
        fen_sections.at(current_fen_section) = std::string_view(fen.begin()+start,fen.begin()+(++end-1));
        ++current_fen_section;
}
static inline bool isdigit(const char i)
{
    return i <= '9' && i >= '0';
}
namespace BB
{
    bool Position::ImportFen(std::string_view fen)
    {
        ResetBoard();
        fen = RemoveWhiteSpace(fen);
        std::array<std::string_view,6> fen_sections;
        Split(fen,fen_sections);

        uint8_t current_row = 7;
        uint8_t current_col = 0;
        for(const char i : fen_sections[0])
        {
            if(isdigit(i))
            {
                current_col += i - '0';
                if(current_col > '7') return false; 
                continue;
            }
            if(i == '/')
            {
                current_col = 0;
                --current_row;
                continue;
            }
            switch (i)
            {
            case('p'):
                pieces_[loc::BLACK][loc::PAWN]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('n'):
                pieces_[loc::BLACK][loc::KNIGHT]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('b'):
                pieces_[loc::BLACK][loc::BISHOP]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('r'):
                pieces_[loc::BLACK][loc::ROOK]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('q'):
                pieces_[loc::BLACK][loc::QUEEN]   |= 1ull << ((current_row * 8) + current_col);
                break;
            case('k'):
                pieces_[loc::BLACK][loc::KING]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('P'):
                pieces_[loc::WHITE][loc::PAWN]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('N'):
                pieces_[loc::WHITE][loc::KNIGHT]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('B'):
                pieces_[loc::WHITE][loc::BISHOP]  |= 1ull << ((current_row * 8) + current_col);
                break;
            case('R'):
                pieces_[loc::WHITE][loc::ROOK]    |= 1ull << ((current_row * 8) + current_col);
                break;
            case('Q'):
                pieces_[loc::WHITE][loc::QUEEN]   |= 1ull << ((current_row * 8) + current_col);
                break;
            case('K'):
                pieces_[loc::WHITE][loc::KING]    |= 1ull << ((current_row * 8) + current_col);
                break;
            default:
                return false;
            }
            ++current_col;
        }

        if(fen_sections.at(1).at(0) != 'w' && fen_sections.at(1).at(0) != 'b') return false;
        whites_turn_ = fen_sections.at(1).at(0) == 'w';

        for(const char i : fen_sections.at(2))
        {
            switch (i)
            {
            case '-':                
                break;
            case 'K':
                info_.castling_rights_ |= 0x08;
                break;
            case 'Q':
                info_.castling_rights_ |= 0x04;
                break;
            case 'k':
                info_.castling_rights_ |= 0x02;
                break;
            case 'q':
                info_.castling_rights_ |= 0x01;
                break; 
            default:
                return false;
            }
        }

        if(fen_sections.at(3) != "-")
        {
            if(fen_sections.at(3).at(0) < 'a' || fen_sections.at(3).at(0) > 'h') return false;
            if(fen_sections.at(3).at(1) != '3' && fen_sections.at(3).at(1) != '6') return false;

            int en_passant_index = 0;
            en_passant_index += (fen_sections.at(3).at(0)) - 'a';
            en_passant_index += (fen_sections.at(3).at(1) - '1') * 8;
            info_.en_passant_target_sq_ = en_passant_index;
        }

        std::from_chars(fen_sections.at(4).data(), fen_sections.at(4).data() + fen_sections.size(), info_.half_moves_);
        std::from_chars(fen_sections.at(5).data(), fen_sections.at(5).data() + fen_sections.size(), full_moves_);

        return true;
    }
}