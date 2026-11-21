# Bubble Shooter: Code Review

A function-by-function review of `bubbleshooter.cpp`, measured against the course's *DSA Implementation Sheet* (the assignment brief; "sheet Q1", "sheet Q5" and so on refer to its questions).

- **Original code:** saved unchanged as `bubbleshooter.cpp.bak`
- **Fixed code:** `code/bubbleshooter.cpp`. The line numbers below refer to this file. Since this review, the whole program has become **one file**. Part 1 holds the reviewed logic, and Part 2 is the SFML window that replaced the text menu (see `PROGRAM_OVERVIEW.md`). The menu's scoreboard options are now the keys **H S F D T B**. Functions that only the text menu used (`displayBoard`, `displayQueue`, `readInt`) were removed; their rows are marked below.
- **How it was checked:** every bug listed as "reproduced" was triggered on the original build first and then re-tested on the fixed build. The fixed code was also run against 79 unit checks that follow the sheet's Question 14 testing table, plus about 48,000 random menu, coordinate and junk inputs. Results: 0 failures, 0 crashes, 0 hangs, 0 compiler warnings (`-Wall -Wextra -Wpedantic -Wshadow`, C++11 and C++17). Those were the checks at review time. The current one-file program has 83 unit checks plus window and keyboard tests (see `PROGRAM_OVERVIEW.md` section 8).

### Legend

| Mark | Meaning |
|---|---|
| ✅ | Correct as written. Left unchanged. |
| 🔧 | Bug. Fixed. |
| 🧹 | Worked, but was cleaned up (duplicate code, magic numbers, unused helpers). |
| ➕ | Missing. The sheet requires it, or a fix needed it. Added. |
| ⚠️ | Worth knowing, but left as it is (a design choice or out of scope). |

---

## 1. Verdict

The data structures are mostly **correct and well chosen**:

- The circular queue uses `%` properly.
- The undo snapshot also saves the queue.
- Matching uses a DFS with a `visited` array, and the group is recorded first and removed afterwards, exactly as the sheet's "Important" note asks.
- The linked list is merge sorted by **relinking nodes**, not by swapping data.
- The BST uses the composite key `(score, gameID)`, so equal scores are never lost.
- All three BST delete cases are handled correctly.

The real problems were in the **glue code**: what happens before a game starts, after a game ends, when the undo history fills up, and when the user types something unexpected. Two of these were game-breaking.

## 2. Bugs found (in order of severity)

| # | Severity | Bug | Evidence (original build) | Fix |
|---|---|---|---|---|
| 1 | **Critical** | **Undo before starting a game** gives an empty board with bubble colour `0`. The first shot then "matches" all 100 empty cells and you win instantly with 1000 points, which is saved to the leaderboard. | Reproduced: menu `4`, then `3` → `5 5` printed `Connected bubbles: 100`, `Score: 1000`, `You won!` | `Stack::top` now starts at `-1`. `undoMove()` needs an active game. `isMatchingColor()` rejects colour 0. |
| 2 | **Critical** | **Typing a letter freezes the program.** `cin` enters a fail state and every later `cin >>` fails instantly, so the menu loops forever. This also happened at the row, column and ID prompts. | Reproduced: input `abc` printed **26 million lines in 3 seconds** | New `readInt()` clears and skips bad input. End of input (Ctrl+Z) exits cleanly. *Later the menu was replaced by the window, which reads numbers one key at a time, so this can't happen any more.* |
| 3 | **High** | **A bubble could be placed in any empty cell**, even floating in mid-air or on the bottom row (instant loss). In a bubble shooter the bubble stops only when it hits the ceiling or another bubble (sheet Q5: `checkCollision`). | Reproduced: first shot at `(9,0)` → `Game Over! You lost!` | New `isAttached()`: the cell must be in row 0 or touch a bubble above or beside it. |
| 4 | **High** | **Undo after game over brought the finished game back** (`gameActive = true`). Its record was already saved, so finishing it again saved a **duplicate record** for the same game. | Reproduced: lose, Undo, lose → history shows **Game ID 1 and Game ID 2** from one game | Undo only works during an active game. `endGame()` clears the undo stack. |
| 5 | **High** | **After 20 shots, Undo skips moves.** When the stack was full, `push()` refused to save, so from shot 21 on Undo jumped back 2, 3, 4… moves. | Reproduced: 21 shots, Undo, 1 shot → `Shots: 20` (should be 21) | When full, `push()` drops the **oldest** state, so the newest 20 moves are always undoable. |
| 6 | Medium | **An active game could never be restarted.** Option 1 said *"Finish or restart the current game first!"* but no restart option existed, so `restartGame()` was never called. | Reproduced | A restart option was added. *It is now the **New Game** button.* |
| 7 | Medium (not yet triggered) | `updateRecord()` changed the score in the list but **not in the BST**. The leaderboard would show the old score, and deleting that game later would leave a ghost node in the BST, because `deleteNode` would search with the new score. | Logic. It isn't in the menu, so it hadn't been hit yet. | Delete the old `(score,id)` key and insert the new one. |
| 8 | Low (not yet triggered) | `getHighScore()` returned the **current** score, not the highest recorded one (sheet Q6). | Logic | Now returns `findMax(root)->score`. |
| 9 | Low (not yet triggered) | `pop()` / `peek()` had **no empty check** (sheet Q3: "Check empty"), so they could read `states[-1]`. | Logic | Now `bool pop(GameState&)` / `bool peek(GameState&)`. |
| 10 | Low (not yet triggered) | `isEmpty(row,col)` had **no bounds check** (sheet Q1: "a *valid* cell contains 0"). | Logic | Now `isValidCell(...) && board[...] == 0`. |
| 11 | Low | `removeMatches()` removed the group whatever its size (the size check lived in another function), which goes against sheet Q2. | Logic | It now checks `MIN_MATCH` itself and returns how many it removed. |
| 12 | Cosmetic | Column numbers under the board were printed 3 characters to the left of their columns. | Visible in every board print | The header now starts with 4 spaces. *`displayBoard` was later removed, because the window draws the board.* |

---

## 3. Function-by-function review

### Constants & globals (lines 22–38)

| Item | Mark | Review |
|---|---|---|
| `ROWS, COLS, QUEUE_SIZE, STACK_SIZE, COLORS` | ✅ | Good use of named constants. |
| `4` (start rows), `5` (queue length), `3` (min match), `ROWS-2` (danger row) | 🧹 | These were magic numbers scattered through the code. They are now `START_ROWS`, `START_QUEUE`, `MIN_MATCH` and `DANGER_ROW`. |
| `board`, `visited`, `score`, `shotsTaken`, `currentBubble`, `nextGameID`, `gameActive` | ✅ | Globals are fine for this project; the sheet explicitly allows "structures and ordinary functions". |

### Queue: upcoming bubbles (lines 40–117)

| Function | Mark | Review |
|---|---|---|
| `class Queue` | 🔧 | `front/rear/count` had no initial values, so the global started with `rear = 0` instead of `-1`. That was harmless here because `initializeQueue()` resets them, but they now have defaults (`0, -1, 0`), consistent with the Stack fix. |
| `isQueueEmpty()` / `isQueueFull()` | ✅ | Correct: both use `count`, which avoids the classic "front == rear is ambiguous" problem. |
| `enqueue(color)` | ✅ | Correct circular step `(rear + 1) % QUEUE_SIZE`, with a full check first. |
| `dequeue()` | ✅ | Correct. Returning `-1` when empty is a safe sentinel because colours are 1–5. |
| `getFront()` | ✅ | Correct. It is not called anywhere, because the window draws all 5 upcoming bubbles by walking the queue, but the sheet requires it (Q4). |
| `generateRandomBubble()` | ✅ 🔧 | Originally returned any colour 1–5. **Changed later on request:** it now picks only colours **still on the board** (via `colorOnBoard`). Otherwise, with one blue bubble left, you could keep getting the other 4 colours forever and the game would never end. On an empty board it falls back to `randomColor()`. |
| `randomColor()` / `colorOnBoard(color)` | ➕ | `randomColor()` returns any colour 1–5 and is used to fill the starting board, which is empty while it is being filled. `colorOnBoard()` scans the board for a colour. |
| `initializeQueue()` | ✅ 🧹 | The logic was right, but it copied the body of `enqueue` and called `rand()` itself. It now calls `enqueue(generateRandomBubble())` `START_QUEUE` times. |
| `nextBubble()` | ➕ | Required by sheet Q4. This logic was inline in `shootBubble`. It also replaces any queued colour that has since been cleared from the board. `startGame()` now also takes the first bubble **from the queue**, so the queue is the single source of bubbles. |
| `displayQueue()` | ✅ *(removed later)* | Correctly walked `front … front+count-1` modulo the size. Removed with the text menu: the window draws the upcoming bubbles with the same loop. |

### Stack: undo (lines 119–190)

| Function | Mark | Review |
|---|---|---|
| `class GameState` | ✅ | Very good. It saves the board, score, shots, current bubble **and the whole queue**, which is exactly what the sheet's "complete undo" asks for. Assigning a class that contains arrays copies them element by element, so `savedQueue = bubbleQueue` works. |
| `class Stack` | 🔧 | **Root cause of bug 1.** `top` had no initial value, and global objects are zero-initialised, so `top` started at **0**. The stack therefore "contained" one all-zero state before any game. It is now `int top = -1;`. |
| `initializeStack()` / `isStackEmpty()` / `isStackFull()` | ✅ | Correct. |
| `push()` | ✅ 🔧 | The snapshot logic is correct, and it is called **after** validation, so invalid shots don't use up undo slots. **Bug 5:** when full it refused to save. It now shifts states `[1..19]` down to `[0..18]`, dropping the oldest, and pushes the new one. That is how undo works in every editor. |
| `pop()` | 🔧 | **Bug 9:** no empty check. It is now `bool pop(GameState &state)` and returns `false` when empty. |
| `peek()` | 🔧 | Same fix as `pop()`. It is unused, but the sheet requires it. |
| `undoMove()` | ✅ 🔧 | The restore logic is correct. **Bugs 1 and 4:** it ran with no game and forced `gameActive = true`. It now refuses unless a game is active and never changes `gameActive`. |

### Board: 2D array (lines 192–249)

| Function | Mark | Review |
|---|---|---|
| `isValidCell(row,col)` | ✅ | Correct. |
| `isEmpty(row,col)` | 🔧 | **Bug 10:** no bounds check. Every caller happened to check first, but it was one careless call away from reading outside the array. |
| `initializeBoard()` | ✅ 🧹 | Correct. It now uses `START_ROWS` and `randomColor()`. |
| `resetBoard()` | ➕ | Required by sheet Q1. |
| `countBubbles()` | ➕ | Required by sheet Q1. It is shown in the side panel as "Bubbles left: N". |
| `displayBoard()` | 🔧 *(removed later)* | **Bug 12:** misaligned column header. Fixed, then removed with the text menu, because the window draws the board. |
| `placeBubble(row,col,color)` | ✅ | Validates before writing. ⚠️ It fails silently (returns `void`), which is fine because `shootBubble` validates first. |
| `removeBubble(row,col)` | ✅ | Correct. |
| `isBoardEmpty()` | ✅ | Correct, and it returns early on the first bubble found. |
| `isAttached(row,col)` | ➕ | **Fix for bug 3.** A bubble flying upward can only stop when it touches the ceiling (row 0) or a bubble **above or beside** it. A bubble only *below* the cell can't stop it, because the shot would have hit that bubble first. The window's `findNearestCell` only picks cells that pass `isAttached`, so a bubble can never stop in empty space. |

### Scoring (lines 251–254, 570)

| Function | Mark | Review |
|---|---|---|
| `calculateScore(n)` / `updateScore(points)` | ✅ 🧹 | Correct, but **never called**: `processMatches` did `score += totalMatches * 10` itself. They are now used. |
| `resetScore()` | ✅ | Correct. |
| `getHighScore()` | 🔧 | **Bug 8:** returned the current score. It now returns the BST maximum, shown as "Best score so far" after each game. It was moved below the BST section because it needs `findMax`. |

### Recursion: matching (lines 256–312)

| Function | Mark | Review |
|---|---|---|
| `matchRows[] / matchCols[] / matchCount` | ✅ | A good design: the group is **recorded** during the search and removed only afterwards, which is the sheet's "Important" rule. |
| `resetVisited()` | ➕ | Required by sheet Q2. It was inline inside `findMatches`. |
| `isMatchingColor(row,col,color)` | 🔧 | The sheet says to reject *invalid, **empty**, different-colour or visited* cells, but empty cells weren't rejected. With colour `0`, the DFS floods every empty cell, which is how bug 1 produced "100 connected bubbles". `color != 0` was added. |
| `checkConnected(row,col,color)` | ✅ | A textbook DFS: base case first, the cell is marked visited **before** recursing (which prevents infinite recursion), and it checks 4 neighbours (right for a rectangular grid, per the sheet). The maximum depth is 100, which is safe. |
| `findMatches(row,col)` | ✅ 🔧 | Correct. It now calls `resetVisited()` and returns 0 for an invalid or empty start cell instead of reading out of bounds. |
| `countMatches()` | ➕ | Required by sheet Q2. |
| `removeMatches()` | 🔧 | **Bug 11:** it now enforces `MIN_MATCH` itself and returns the number removed. |
| `processMatches(row,col)` | ✅ 🧹 | The flow was right. It now uses `removeMatches()`'s return value together with `calculateScore` / `updateScore`. |

### Win / lose (lines 314–324)

| Function | Mark | Review |
|---|---|---|
| `checkWin()` | ✅ | Board empty means win. |
| `checkLose()` | ✅ 🧹 | Correct. The hard-coded checks of `ROWS-1` and `ROWS-2` became a loop from `DANGER_ROW` to the bottom. |

### Singly linked list: game history (lines 326–449, 602)

| Function | Mark | Review |
|---|---|---|
| `class GameRecord` | ✅ | Matches the sheet. |
| `insertRecord(...)` | ✅ | Inserting at the tail keeps chronological order. ⚠️ It is O(n) because there is no tail pointer, which is fine for a game history. |
| `displayRecords()` | ✅ | Handles the empty list. |
| `searchRecord(id)` | ✅ | This **is** your `linearSearch` from sheet Q9. Say so in the viva. |
| `deleteRecord(id)` | ✅ | Handles the empty list, the head, a middle node, the tail and "not found", and frees memory each time. Well done. |
| `countRecords()` | ✅ | Correct. |
| `getMiddle(start)` | ✅ | Correct. Starting `fast` at `start->next` means a 2-node list splits 1 + 1. Starting it at `start` would loop forever, a classic mistake you avoided. |
| `mergeLists(a,b)` | ✅ | Relinks nodes as the sheet requires. `>=` keeps it **stable** and sorts **descending**. ⚠️ The recursion depth equals the list length, which is fine for hundreds of records but not millions. |
| `mergeSort(start)` | ✅ | Correct, O(n log n). |
| `sortRecords()` | ✅ | ⚠️ Sorting **permanently reorders** the history. Games played after a sort are appended at the end, so the list ends up neither chronological nor fully sorted. This was left as is because the sheet asks for in-place relinking, but be ready to explain it. |
| `updateRecord(id,newScore)` | 🔧 | **Bug 7:** didn't update the BST. It now removes the old BST key and inserts the new one, and was moved below the BST section because it needs those functions. |

### BST: leaderboard (lines 451–575)

| Function | Mark | Review |
|---|---|---|
| `class TreeNode` | ✅ | Matches the sheet. |
| `comesBefore(...)` | ✅ | Excellent. The composite key `(score, gameID)` is exactly the sheet's advice, and since IDs are unique, no two keys are ever equal. |
| `insertNode(...)` | ✅ | Correct. ⚠️ The tree is not balanced: if scores arrive in increasing order it becomes a "linked list" with O(n) operations. AVL is optional per the sheet. |
| `searchNode(...)` | ✅ | Correct. Not used by any key or button, which is fine: the sheet only requires it to exist as a BST operation. |
| `findMin()` / `findMax()` | ✅ | Correct, and handle an empty tree. |
| `inorder()` / `preorder()` / `postorder()` | ✅ | Correct. ⚠️ Cosmetic: `inorder` prints one line per game while the other two print scores on one line, so the key **T** output looks mixed. Also, `inorder` lists the **lowest** score first. The window's top-5 list uses a reverse inorder (right, root, left) to show the highest first. |
| `deleteNode(...)` | ✅ | All three cases are correct, and the inorder successor is removed with its **full key**, so the right node is deleted even when scores tie. |
| `height()` | ✅ | ⚠️ Counts **nodes** (empty = 0, one node = 1). The sheet mentions the **edge** convention (empty = −1). Both are valid; state which one you use. |
| `countNodes()` | ➕ | A helper used to size the binary-search array. |

### Searching (lines 577–599)

| Function | Mark | Review |
|---|---|---|
| `storeInorder(node, arr, index)` | ➕ | An inorder traversal of a BST visits keys in ascending order, so it fills an already **sorted** array. |
| `binarySearch(arr, size, key)` | ➕ | **Required by sheet Q9 and was missing.** Iterative, and uses `mid = low + (high-low)/2`. Key **B** demonstrates it on the leaderboard scores. |

### Game control (lines 601–686, the end of Part 1)

| Function | Mark | Review |
|---|---|---|
| `saveGameResult(won)` | ✅ | Inserts into **both** the list and the BST with the same ID. Now also prints the best score. |
| `shootBubble(row,col)` | ✅ 🔧 | Correct order: validate → `push()` → place → match → next bubble. **Bug 3:** it now also requires `isAttached()`. |
| `startGame()` | ✅ | Resets everything. It now takes the first bubble via `nextBubble()`. |
| `restartGame()` | ✅ 🔧 | The body was fine, but it was **unreachable** (bug 6). It is now reached through the **New Game** button. The abandoned game is not saved, which matches sheet Q10. |
| `endGame(won)` | ➕ | Required by sheet Q10. It stops the game, saves the record and BST node, and clears the undo history (the fix for bug 4). |
| `clearRecords()` | ✅ | Frees every node. |
| `clearTree(node)` | ✅ | Postorder delete, the correct order (children before parent). |
| `quitGame()` | ➕ | Required by sheet Q10. Frees the list and the tree, and sets `root = NULL`. |
| `readInt(prompt)` | ➕ *(removed later)* | **Fix for bug 2** in the text-menu version. Removed with the menu: numbers are now typed in the game window, one key at a time (`handleTyping`). |
| `main()` | ✅ 🔧 | The original menu loop was reviewed and fixed: `readInt` everywhere, a restart option, active-game checks, and a new binary-search option. Now `main()` opens the SFML window and runs the frame loop, and the menu's scoreboard options became the keys **H S F D T B**. |

---

## 4. Not fixed: things to know or do later

1. **Floating bubbles don't fall.** In a real bubble shooter, after a pop, any bubbles no longer connected to the ceiling drop for bonus points. A second DFS starting from row 0 that marks everything reachable would find them; whatever is left unmarked has to fall. It would be a good extra recursion demo, but it isn't in the sheet. Without it you have to pop every bubble by matching, so winning takes a long time.
2. **`inorder` lists scores in ascending order** (see BST notes). The window's top-5 list already uses a reverse inorder to show the highest first.
3. **`sortRecords()` loses chronological order** for good (see linked-list notes).
4. **The BST can become unbalanced.** AVL is optional in the sheet.
5. **Everything is in one file** (`bubbleshooter.cpp`: Part 1 logic, Part 2 window). This was chosen on purpose for simplicity. Sheet Q13 suggests one file per module (`Board.h/.cpp`, `Queue.h/.cpp`, and so on); each section header in the file maps onto one of those files if you ever want to split it.
6. **The SFML shooting functions are now done** in Part 2: `aimLauncher`, `launchBubble` (the sheet's `shootBubble`), `updateBubblePosition`, `checkWallCollision`, `checkCollision`, `findNearestCell`, `snapBubbleToGrid` and `finishShot`. `finishShot` passes the landing cell to the reviewed `shootBubble(row, col)` in Part 1.
7. **The starting board can already contain groups of 3+** that stay until hit. That is normal for the genre.

## 5. How to rebuild

Close the game first, because Windows locks a running exe. Then double-click **`code/build.bat`**, or run it from any terminal. It builds `bubbleshooter.exe` as a standalone exe.
