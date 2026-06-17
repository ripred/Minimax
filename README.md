# Minimax – Embedded-Friendly Alpha-Beta Library for Arduino & C++

[![CI](https://img.shields.io/static/v1?style=flat&label=CI&message=passing&color=2da44e&labelColor=24292f)](https://github.com/ripred/Minimax/actions/workflows/ci.yml)
[![Arduino-lint](https://img.shields.io/static/v1?style=flat&label=Arduino-lint&message=passing&color=2da44e&labelColor=24292f)](https://github.com/ripred/Minimax/actions/workflows/arduino-lint.yml)
[![Arduino Library Manager](https://img.shields.io/static/v1?style=flat&label=Arduino+Library+Manager&message=available&color=00878f&labelColor=24292f)](https://www.ardu-badge.com/Minimax)
[![Release](https://img.shields.io/static/v1?style=flat&label=Release&message=Minimax_v1.00&color=8250df&labelColor=24292f)](https://github.com/ripred/Minimax/releases/latest)
[![License](https://img.shields.io/static/v1?style=flat&label=License&message=MIT&color=0969da&labelColor=24292f)](https://github.com/ripred/Minimax/blob/main/LICENSE)
[![Stars](https://img.shields.io/static/v1?style=flat&label=Stars&message=4&color=bf8700&labelColor=24292f)](https://github.com/ripred/Minimax/stargazers)
[![Forks](https://img.shields.io/static/v1?style=flat&label=Forks&message=1&color=6f42c1&labelColor=24292f)](https://github.com/ripred/Minimax/network/members)

![Arduino IDE ≥ 1.8](https://img.shields.io/badge/Arduino-1.8%2B-00979D?logo=arduino&logoColor=white)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![CI](https://img.shields.io/github/actions/workflow/status/ripred/Minimax/ci.yml?label=build)


A **single-header, STL-free** implementation of classic **Minimax search with α–β pruning**, crafted for 2 KB microcontrollers yet flexible enough for desktop use.

* **Tiny footprint** – ≈ 5 KB flash  
* **Zero dynamic allocation** – no `new`, no STL containers  
* **Template-based** – plug in *your* board & move types  
* **Five-function API** – override, compile, play  
* **Playable examples** – Checkers, Connect Four, Gomoku & Othello

---

## 1 • Why Another Minimax?

Most hobby libraries assume megabytes of RAM and `std::vector`.  
This header was born while squeezing an AI into an **ATmega328** (2 KB RAM).

---

## 2 • Getting Started

### 2.1 Install

<details><summary><b>Arduino IDE / Arduino-CLI</b></summary>

    
    # Clone into your libraries folder
    cd ~/Documents/Arduino/libraries
    git clone https://github.com/ripred/Minimax.git
    

Restart the IDE to auto-discover the library.  
</details>

<details><summary><b>PlatformIO</b></summary>

    
    lib_deps =
        ripred/Minimax @ ^1.0.0   ; once published
    

</details>

### 2.2 Hello (Tic-Tac-Toe) World

    
    #include "Minimax.h"
    
    struct Board   { /* … */ };
    struct Move    { uint8_t r, c; };
    
    class TicTacToeLogic : public Minimax<Board, Move>::GameLogic {
    public:
        int  evaluate(const Board& s) override { /* +10 / −10 / 0 */ }
        int  generateMoves(const Board& s, Move mv[], int max) override { /* … */ }
        void applyMove(Board& s, const Move& m) override { /* … */ }
        bool isTerminal(const Board& s) override { /* win / draw */ }
        bool isMaximizingPlayer(const Board& s) override { return s.turn == 0; }
    };
    
    TicTacToeLogic logic;
    Minimax<Board, Move, 9, 9> ai(logic);
    
    void loop() {
        Board state = /* current board */;
        Move best   = ai.findBestMove(state);
        Serial.printf("AI chose %d,%d (score %d, nodes %d)\n",
                      best.r, best.c, ai.getBestScore(), ai.getNodesSearched());
    }
    

Compile with `-std=gnu++17` (earlier standards also work if your toolchain lacks C++17).

---

## 3 • Library API

| Method | Purpose |
|--------|---------|
| `Move findBestMove(const GameState& s)` | Run α–β search up to `MaxDepth`, returning the best move |
| `int getBestScore() const` | Score from the last search |
| `int getNodesSearched() const` | Positions evaluated (profiling) |

### Game Logic Interface

Implement these in a subclass of `Minimax<GameState, Move>::GameLogic`:

1. `evaluate()` – heuristic for the *current* player  
2. `generateMoves()` – fill an array of legal moves, return count  
3. `applyMove()` – mutate `GameState` with a move  
4. `isTerminal()` – true on win/loss/draw  
5. `isMaximizingPlayer()` – whose turn?  

---

## 4 • Examples

| Sketch | MCU RAM | Notes |
|--------|---------|-------|
| Checkers | ~1.7 KB | 8×8 American with kinging & jumps |
| Connect-Four | ~1.4 KB | 7×6 board, depth-4 search fits on AVR |
| Gomoku | ~1.6 KB | 15×15 “Five-in-a-Row” |
| Othello | ~1.8 KB | 8×8 reversible-disc game |

Open **File → Examples → Minimax → *(game)*** after installing, or compile on desktop to profile deeper depths.

---

## 5 • Tuning for Your Board

| Knob | Effect | Trade-off |
|------|--------|-----------|
| `MaxMoves` | Size of the scratch array | Static RAM cost |
| `MaxDepth` | Search depth | Flash & time exponential |
| Heuristic | Score range | Consider `int16_t` on 8-bit MCUs |

Disable `Serial` prints in examples to save ~2 KB flash.

---

## 6 • Roadmap

* Iterative deepening + time cut-off  
* Optional transposition table when RAM ≥ 32 KB  
* Board-symmetry pruning helpers  

---

## 7 • Contributing

1. Fork → create feature branch  
2. Follow [coding style](CONTRIBUTING.md) (4-space indents, `snake_case`, right-side `const`)  
3. Submit PR – CI (AVR & x86) must pass

Bug reports and new example ports are **very** welcome!

---

## 8 • License

Released under the MIT License – see [LICENSE](LICENSE).

<sup>© 2025 by <strong>Trent M. Wyatt</strong></sup>
