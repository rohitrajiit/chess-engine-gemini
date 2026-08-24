#pragma once

#include "types.h"
#include "board.h"
#include <cstdint>

namespace zenith {

uint64_t perft(Board& board, int depth);
void perft_divide(Board& board, int depth);
void run_perft_suite();

} // namespace zenith
