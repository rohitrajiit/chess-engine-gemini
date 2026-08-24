cmake_minimum_required(VERSION 3.16)
project(zenith CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
if(MSVC)
    add_compile_options(/O2 /W4)
else()
    add_compile_options(-O3 -Wall -Wextra -pedantic)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|x86_64")
        add_compile_options(-march=native)
    endif()
endif()

set(SOURCES
    src/main.cpp
    src/bitboard.cpp
    src/zobrist.cpp
    src/board.cpp
    src/movegen.cpp
    src/evaluate.cpp
    src/tt.cpp
    src/search.cpp
    src/uci.cpp
    src/perft.cpp
)

add_executable(zenith ${SOURCES})
target_include_directories(zenith PRIVATE src)
