#include "perft.h"
#include "movegen.h"
#include <iostream>
#include <chrono>
#include <vector>

namespace zenith {

uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1ULL;

    MoveList moves;
    generate_pseudo_legal_moves(board, moves);

    uint64_t nodes = 0ULL;

    for (int i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (!board.make_move(m)) continue;

        nodes += (depth == 1) ? 1ULL : perft(board, depth - 1);
        board.unmake_move(m);
    }

    return nodes;
}

void perft_divide(Board& board, int depth) {
    if (depth <= 0) return;

    MoveList moves;
    generate_pseudo_legal_moves(board, moves);

    auto start = std::chrono::steady_clock::now();
    uint64_t total_nodes = 0ULL;

    std::cout << "\nPerft divide depth " << depth << ":\n";

    for (int i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (!board.make_move(m)) continue;

        uint64_t nodes = (depth == 1) ? 1ULL : perft(board, depth - 1);
        board.unmake_move(m);

        total_nodes += nodes;
        std::cout << m.to_uci() << ": " << nodes << "\n";
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    uint64_t nps = (elapsed_ms > 0) ? (total_nodes * 1000 / elapsed_ms) : (total_nodes * 1000);

    std::cout << "\nTotal nodes: " << total_nodes << "\n";
    std::cout << "Time: " << elapsed_ms << " ms (" << nps << " NPS)\n\n";
}

struct PerftTest {
    const char* name;
    const char* fen;
    int depth;
    uint64_t expected_nodes;
};

void run_perft_suite() {
    std::vector<PerftTest> tests = {
        { "Startpos D1", Board::START_FEN, 1, 20ULL },
        { "Startpos D2", Board::START_FEN, 2, 400ULL },
        { "Startpos D3", Board::START_FEN, 3, 8902ULL },
        { "Startpos D4", Board::START_FEN, 4, 197281ULL },
        { "Startpos D5", Board::START_FEN, 5, 4865609ULL },

        { "Kiwipete D1", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48ULL },
        { "Kiwipete D2", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039ULL },
        { "Kiwipete D3", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862ULL },
        { "Kiwipete D4", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603ULL },

        { "Position 3 D1", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14ULL },
        { "Position 3 D2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191ULL },
        { "Position 3 D3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812ULL },
        { "Position 3 D4", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238ULL },
        { "Position 3 D5", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624ULL },

        { "Position 4 D1", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6ULL },
        { "Position 4 D2", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264ULL },
        { "Position 4 D3", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467ULL },
        { "Position 4 D4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333ULL },

        { "Position 5 D1", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44ULL },
        { "Position 5 D2", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486ULL },
        { "Position 5 D3", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379ULL },
        { "Position 5 D4", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487ULL },

        { "Position 6 D1", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46ULL },
        { "Position 6 D2", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079ULL },
        { "Position 6 D3", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890ULL },
        { "Position 6 D4", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594ULL }
    };

    std::cout << "\n================ Running Perft Suite ================\n";
    int passed = 0;
    auto suite_start = std::chrono::steady_clock::now();
    uint64_t grand_total_nodes = 0ULL;

    for (const auto& t : tests) {
        Board b(t.fen);
        auto t_start = std::chrono::steady_clock::now();
        uint64_t nodes = perft(b, t.depth);
        auto t_end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
        grand_total_nodes += nodes;

        if (nodes == t.expected_nodes) {
            std::cout << "[PASS] " << t.name << ": " << nodes << " nodes (" << ms << " ms)\n";
            ++passed;
        } else {
            std::cout << "[FAIL] " << t.name << ": Got " << nodes << ", Expected " << t.expected_nodes << "\n";
        }
    }

    auto suite_end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(suite_end - suite_start).count();

    std::cout << "=====================================================\n";
    std::cout << "Results: " << passed << "/" << tests.size() << " tests passed\n";
    std::cout << "Total Nodes: " << grand_total_nodes << " in " << total_ms << " ms\n\n";
}

} // namespace zenith
