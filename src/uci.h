#pragma once

#include "board.h"
#include "search.h"
#include <string>
#include <thread>
#include <memory>

namespace zenith {

class UCIEngine {
public:
    UCIEngine();
    ~UCIEngine();

    void loop();

private:
    void handle_uci();
    void handle_isready();
    void handle_setoption(const std::string& line);
    void handle_position(const std::string& line);
    void handle_go(const std::string& line);
    void handle_stop();
    void handle_perft(const std::string& line);
    void handle_eval();
    void handle_display();

    void wait_search();

    Board board_;
    std::unique_ptr<std::thread> search_thread_;
};

} // namespace zenith
