#include "Pext.hpp"

u64 Pext::bishop_table[5248];
u64 Pext::rook_table[102400];
u64 Pext::slider_masks[2][64];
int Pext::slider_offsets[2][64];
