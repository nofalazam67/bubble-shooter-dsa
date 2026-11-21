# Bubble Shooter: Combo Chaos

A simple bubble shooter written in C++ with SFML 3, built as a Data Structures & Algorithms project.
The whole program is one file: [`code/bubbleshooter.cpp`](code/bubbleshooter.cpp).

| Data structure / algorithm | Used for |
|---|---|
| 2D array | the 10 × 10 board |
| Recursion (DFS + visited array) | finding groups of 3+ same-colour bubbles |
| Circular queue | the upcoming bubbles |
| Stack | Undo |
| Singly linked list + merge sort | game history |
| Binary search tree | leaderboard |
| Linear search / binary search | finding a game / finding a score |

## Build and run (Windows)

1. Install [MSYS2](https://www.msys2.org/), then SFML: `pacman -S mingw-w64-ucrt-x86_64-sfml`
2. Double-click `code/build.bat`
3. Run `code/bubbleshooter.exe`

## Controls

- **Mouse:** aim. **Left click:** shoot. Buttons: **New Game**, **Undo**.
- Keys, typed in the game window, with results in the text window:
  - **H**: history
  - **S**: sort
  - **F**: find a game
  - **D**: delete a game
  - **T**: BST traversals
  - **B**: binary search

## Documentation

- [`docs/PROGRAM_OVERVIEW.md`](docs/PROGRAM_OVERVIEW.md): how everything works, with a map of the file, flowcharts and viva questions.
- [`docs/CODE_REVIEW.md`](docs/CODE_REVIEW.md): a function-by-function review of the original code and the bugs that were fixed.
