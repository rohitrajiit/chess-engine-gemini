#include "types.h"
#include "bitboard.h"
#include "zobrist.h"
#include "uci.h"
#include "perft.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // 1. Initialize bitboards (attacks, magic tables, rays)
    zenith::init_bitboards();

    // 2. Initialize Zobrist hashing keys
    zenith::zobrist::init_zobrist();

    // Check command line arguments for quick benchmarking
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "bench" || arg == "test" || arg == "--test") {
            zenith::run_perft_suite();
            return 0;
        }
    }

    // 3. Start UCI Loop
    zenith::UCIEngine uci;
    uci.loop();

    return 0;
}
