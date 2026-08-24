CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O3 -march=native -Wall -Wextra -pedantic -Isrc

SRCS = src/main.cpp \
       src/bitboard.cpp \
       src/zobrist.cpp \
       src/board.cpp \
       src/movegen.cpp \
       src/evaluate.cpp \
       src/tt.cpp \
       src/search.cpp \
       src/uci.cpp \
       src/perft.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = zenith

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
