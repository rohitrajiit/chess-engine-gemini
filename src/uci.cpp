#include "uci.h"
#include "perft.h"
#include "evaluate.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace zenith {

UCIEngine::UCIEngine() {
    board_.reset();
}

UCIEngine::~UCIEngine() {
    handle_stop();
}

void UCIEngine::wait_search() {
    if (search_thread_ && search_thread_->joinable()) {
        search_thread_->join();
        search_thread_.reset();
    }
}

void UCIEngine::handle_uci() {
    std::cout << "id name Zenith 1.0\n";
    std::cout << "id author Antigravity\n";
    std::cout << "option name Hash type spin default 64 min 1 max 1024\n";
    std::cout << "option name Clear Hash type button\n";
    std::cout << "uciok" << std::endl;
}

void UCIEngine::handle_isready() {
    std::cout << "readyok" << std::endl;
}

void UCIEngine::handle_setoption(const std::string& line) {
    std::istringstream ss(line);
    std::string token, name, val_str;
    ss >> token; // "setoption"

    while (ss >> token) {
        if (token == "name") {
            ss >> name;
            // Check if name has multiple words (e.g. "Clear Hash")
            std::string extra;
            while (ss >> extra && extra != "value") {
                name += " " + extra;
            }
            if (extra == "value") {
                ss >> val_str;
            }
        } else if (token == "value") {
            ss >> val_str;
        }
    }

    if (name == "Hash" && !val_str.empty()) {
        int mb = std::stoi(val_str);
        if (mb >= 1 && mb <= 1024) {
            g_tt.resize(static_cast<size_t>(mb));
        }
    } else if (name == "Clear Hash") {
        g_tt.clear();
    }
}

void UCIEngine::handle_position(const std::string& line) {
    wait_search();

    std::istringstream ss(line);
    std::string token;
    ss >> token; // "position"

    if (!(ss >> token)) return;

    if (token == "startpos") {
        board_.reset();
        ss >> token; // optional "moves"
    } else if (token == "fen") {
        std::string fen;
        while (ss >> token && token != "moves") {
            if (!fen.empty()) fen += " ";
            fen += token;
        }
        board_.set_fen(fen);
    }

    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            Move m = parse_move(board_, move_str);
            if (!m.is_none()) {
                board_.make_move(m);
            }
        }
    }
}

void UCIEngine::handle_go(const std::string& line) {
    wait_search();

    SearchLimits limits;
    std::istringstream ss(line);
    std::string token;
    ss >> token; // "go"

    while (ss >> token) {
        if (token == "wtime") {
            ss >> limits.time[WHITE];
        } else if (token == "btime") {
            ss >> limits.time[BLACK];
        } else if (token == "winc") {
            ss >> limits.inc[WHITE];
        } else if (token == "binc") {
            ss >> limits.inc[BLACK];
        } else if (token == "movestogo") {
            ss >> limits.movestogo;
        } else if (token == "depth") {
            ss >> limits.depth;
        } else if (token == "nodes") {
            ss >> limits.nodes;
        } else if (token == "movetime") {
            ss >> limits.movetime;
        } else if (token == "infinite") {
            limits.infinite = true;
        }
    }

    // Launch search on background thread
    g_search.prepare_search();
    search_thread_ = std::make_unique<std::thread>([this, limits]() {
        g_search.start(board_, limits);
    });
}

void UCIEngine::handle_stop() {
    g_search.stop();
    wait_search();
}

void UCIEngine::handle_perft(const std::string& line) {
    wait_search();
    std::istringstream ss(line);
    std::string token;
    int depth = 1;
    ss >> token >> depth;
    perft_divide(board_, depth);
}

void UCIEngine::handle_eval() {
    wait_search();
    int eval = evaluate(board_);
    std::cout << "Static Evaluation (relative to "
              << (board_.side_to_move() == WHITE ? "White" : "Black")
              << "): " << eval << " cp" << std::endl;
}

void UCIEngine::handle_display() {
    board_.print();
}

void UCIEngine::loop() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "uci") {
            handle_uci();
        } else if (command == "isready") {
            handle_isready();
        } else if (command == "setoption") {
            handle_setoption(line);
        } else if (command == "ucinewgame") {
            wait_search();
            g_tt.clear();
            g_search.clear_history();
            board_.reset();
        } else if (command == "position") {
            handle_position(line);
        } else if (command == "go") {
            handle_go(line);
        } else if (command == "stop") {
            handle_stop();
        } else if (command == "perft") {
            handle_perft(line);
        } else if (command == "bench" || command == "test") {
            wait_search();
            run_perft_suite();
        } else if (command == "eval") {
            handle_eval();
        } else if (command == "d" || command == "display") {
            handle_display();
        } else if (command == "quit") {
            handle_stop();
            break;
        }
    }
    wait_search();
}

} // namespace zenith
