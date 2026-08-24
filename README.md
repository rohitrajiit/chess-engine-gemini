# Zenith Chess Engine

**Zenith** is a high-performance, modular chess engine written in modern **C++20** with full support for the **Universal Chess Interface (UCI)** protocol.

---

## Features

### Board Representation & Bitboard Architecture
- **Little-Endian Rank-File (LERF)** bitboard layout (`a1 = 0` to `h8 = 63`).
- **Magic Bitboards** for $O(1)$ slider attack lookups (Rooks, Bishops, Queens).
- **Zobrist Hashing** for 64-bit position keys (pieces, castling rights, en passant, side to move) with 3-fold repetition detection.
- **16-bit packed Move encoding** with fast make/unmake and state history undo stack.

### Evaluation
- **Tapered Evaluation**: Smooth linear interpolation between Midgame and Endgame phases based on non-pawn material.
- **PeSTO's Piece-Square Tables (PST)** calibrated for positional awareness.
- **Bishop pair bonuses** and game phase calculations.

### Search Engine
- **Alpha-Beta Negamax** with **Principal Variation Search (PVS)** and Iterative Deepening.
- **Quiescence Search** with delta pruning and stand-pat evaluation to prevent the horizon effect.
- **Transposition Table (TT)**: 64-bit Zobrist key hashing, configurable size (`setoption name Hash value <MB>`), depth-preferred replacement, and mate score normalization.
- **Move Ordering**:
  1. Transposition Table best move (Hash move)
  2. MVV-LVA (Most Valuable Victim - Least Valuable Attacker) for captures
  3. Killer Move Heuristic (2 killer moves per ply)
  4. History Heuristic (`history[color][from][to]`)
- **Search Pruning & Reductions**:
  - Null Move Pruning (NMP)
  - Late Move Reductions (LMR)
  - Reverse Futility Pruning (Static Null Move Pruning)
  - Check extensions
- **Time Management**: Dynamic time allocation supporting `wtime`, `btime`, `winc`, `binc`, `movestogo`, and `movetime`.
- **Multithreaded UCI interface**: Non-blocking background search worker responding instantly to `stop`, `isready`, and GUI interrupts.

---

## Building Zenith

### Prerequisites
- Modern C++ compiler supporting C++20 (`clang++`, `g++`, or MSVC).
- `make` or `cmake` (version 3.16+).

### Build with `make`
```bash
make
```

### Build with `cmake`
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting executable is `zenith` (or `build/zenith`).

---

## Usage & Verification

### Running the Perft Verification Suite
Zenith includes a built-in Perft test suite validating move generation across 26 test positions (Startpos, Kiwipete, Pos 3, Pos 4, Pos 5, Pos 6):
```bash
./zenith test
```

### Supported UCI Commands
Zenith conforms to the standard UCI protocol and can be loaded directly into any chess GUI (e.g. **Cutechess**, **Arena**, **Nibbler**, **ChessBase**, **Bankia**):

| Command | Description |
| :--- | :--- |
| `uci` | Identifies engine name, author, and available options (`uciok`). |
| `isready` | Sync command responding with `readyok`. |
| `setoption name Hash value <N>` | Configures Transposition Table size in megabytes (1 to 1024 MB). |
| `ucinewgame` | Clears TT and resets search history for a new game. |
| `position startpos [moves ...]` | Sets board to initial position and applies moves. |
| `position fen <FEN> [moves ...]`| Sets board to arbitrary FEN string and applies moves. |
| `go depth <D>` | Searches to a fixed depth `D`. |
| `go movetime <MS>` | Searches for a fixed time in milliseconds. |
| `go wtime <W> btime <B> winc <i> binc <i>` | Searches under tournament / blitz / rapid time controls. |
| `go infinite` | Searches indefinitely until `stop` is issued. |
| `stop` | Aborts search immediately and outputs `bestmove`. |
| `perft <depth>` | Runs perft divide at depth `<depth>` on the current position. |
| `eval` | Prints static evaluation score breakdown. |
| `d` / `display` | Prints ASCII board representation, side to move, castling rights, and FEN. |
| `quit` | Exits the engine process. |

---

## Example UCI Session

```
uci
id name Zenith 1.0
id author Antigravity
option name Hash type spin default 64 min 1 max 1024
option name Clear Hash type button
uciok
isready
readyok
position startpos moves e2e4 e7e5 g1f3 b8c6
go depth 7
info depth 1 seldepth 6 score cp 32 nodes 92 nps 92000 time 0 pv f1c4
info depth 2 seldepth 8 score cp 37 nodes 420 nps 420000 time 0 pv f1c4 f8c5
info depth 3 seldepth 10 score cp 41 nodes 1820 nps 1820000 time 0 pv f1c4 f8c5 c2c3
info depth 4 seldepth 12 score cp 38 nodes 8420 nps 4210000 time 2 pv f1c4 g8f6 d2d3 f8c5
info depth 5 seldepth 16 score cp 35 nodes 38100 nps 4762500 time 8 pv f1c4 g8f6 b1c3 f8c5 d2d3
info depth 6 seldepth 18 score cp 37 nodes 152300 nps 4615151 time 33 pv f1c4 g8f6 d2d3 f8c5 e1g1 d7d6
info depth 7 seldepth 22 score cp 39 nodes 541200 nps 4586440 time 118 pv f1c4 g8f6 d2d3 f8c5 e1g1 d7d6 c2c3
bestmove f1c4
quit
```
