#include "Pext.hpp"
#if USE_TITBOARDS != 1
u64 bishop_table[5248];
u64 rook_table[102400];
u64 slider_masks[2][64];
int slider_offsets[2][64];
#endif 