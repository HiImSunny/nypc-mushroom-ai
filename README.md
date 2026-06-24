# NYPC Mushroom Board Game AI (nypc-mushroom-ai)

A state-of-the-art C++ game-playing engine designed for the **NYPC Mushroom Game** (10×17 board grid). It uses advanced chess-programming techniques tailored for abstract board games, achieving a **100% win rate (14W / 0D / 0L)** in local benchmark tests and matching the top-tier tournament records.

---

## 🎮 Game Rules & Challenge
* **Board Size**: 10 rows × 17 columns. Each cell contains a mushroom with a value from 0 to 9.
* **Making a Move**: A player selects a rectangle such that the sum of mushrooms inside it is **exactly 10**, and all 4 borders of the rectangle must contain at least one mushroom.
* **Cell Ownership**: When a rectangle is claimed, all cells in it are colored/claimed by the player (setting their mushroom values to 0). Claiming cells owned by the opponent "steals" them.
* **Game End**: A player can pass their turn. If both players pass consecutively (double-pass), the game terminates.
* **Winning Condition**: The player with the most cells at the end of the game wins.
* **Time Constraints**: Each player has an individual time bank (typically 10,000ms total). Efficient time budgeting is critical.

---

## 🛠️ Engine Architecture (v52 - Latest)

The engine applies standard tournament-grade chess-programming heuristics modified for the board partition mechanics of the Mushroom Game:

### 1. Search Algorithms
* **PVS (Principal Variation Search)**: Performs zero-window searches (`beta = alpha + 1`) on non-PV nodes to prune huge parts of the game tree.
* **LMR (Late Move Reduction)**: Reduces search depth for moves ranked lower in the move ordering to focus search time on the most promising lines.
* **Killer Move Heuristic**: Remembers the 2 best moves causing beta-cutoffs per ply and boosts their sorting scores (`+50,000` / `+25,000`) in subsequent sibling nodes.
* **Transposition Table (TT)**: A `1 << 20` (1 Million entries) hash table using Zobrist Hashing to cache search results, depth levels, and best moves to prevent redundant subtree evaluations.

### 2. Time & Depth Control
* **Iterative Deepening**: Begins search at depth 2 and increments by 2 plies. Uses a `lastDepthTime * 3` check to predict if the next depth level can be fully completed within the remaining budget.
* **Endgame Depth Extension**: Dynamically increases search depth as the number of valid moves shrinks:
  * $> 8$ valid rects $\rightarrow$ depth 10
  * $\le 8$ valid rects $\rightarrow$ depth 12
  * $\le 5$ valid rects $\rightarrow$ depth 14
  * $\le 2$ valid rects $\rightarrow$ depth 18
* **Node-Based Time Checks**: Checks remaining time exactly every 1024 nodes (`nodeCount & 1023 == 0`) rather than depth-based checks. This guarantees stable search cutoffs and avoids search instability.
* **Dynamic Time Budgeting**: Allocates time per move via `budget = (remainingTime - SAFETY_BUFFER) / (totalMushSum / 18)`, capped dynamically between `50ms` and `min(1200ms, remainingTime / 8)`.

### 3. Evaluation Function
The positional strength of a board state is evaluated as:
$$\text{Score} = (\text{myCells} - \text{oppCells}) \times 100 + (\text{myConn} - \text{oppConn}) \times 10 + (\text{myAdjMush} - \text{oppAdjMush}) \times 8$$
* **Cells (weight 100)**: Direct territory owned.
* **Connectivity (weight 10)**: Counts adjacent cells of the same owner, encouraging compact and unified shapes.
* **Adjacency (weight 8)**: Counts adjacent unclaimed mushrooms, encouraging territorial expansion potential.

### 4. Strategic Pass Checks
* If the opponent passed, the engine checks if we are ahead. If `myCells > oppCells`, we **pass immediately to terminate and secure the win** without risk.
* If the opponent didn't pass, we evaluate the virtual score of passing using a shallow depth 3 alpha-beta search. We only pass if the score is significantly higher than making the best available move (`passScore > moveScore + 100`), enabling us to force the opponent into zugzwang.

---

## 📈 Version Strategy History

* **v16**: Implemented basic pass logic, time limits, and initial `scoreRect` / `evaluate` functions.
* **v17 - v21**: Tweaked time management, introduced cell connectivity, and improved board compactness scores.
* **v23 - v32**: Added Zobrist Hashing and Heap-based Transposition Table (64K entries).
* **v33**: Major overhaul. Added PVS search, LMR, Killer moves, and expanded TT to 512K entries.
* **v35 - v42**: Experimented with frontier expansions and quiescence searches. Quiescence was eventually removed as leaf evaluations were more efficient.
* **v43 - v50**:
  * Optimized move ordering using the formula `totalCells * 100 + oppCells * 200 - myLost * 100` (exact difference gain).
  * Added early column-sum pruning in `findValidRects()` using 2D prefix sums.
* **v51**: Upgraded time budgeting with the precise moves-left estimator `estMovesLeft = totalMushSum / 18` and enlarged TT to 1M entries. (Achieved **13W 1L** on tournament web).
* **v52 (Current)**:
  * Fixed time budgeting bug in iterative deepening.
  * Solved Game 2 draw bug against inactive bots by only passing to terminate when winning.
  * Fixed root search width $K = 12$ to prevent pruning tactical moves.
  * Implemented node-based time check frequency (`nodeCount & 1023`), resulting in a perfect **14W 0L** local score.

---

## 🚀 Compilation & Running

### Prerequisites
* GCC compiler with C++11 support or higher.
* Python 3.x (for running battle simulations).

### Compile
Compile the C++ source file with high optimization levels:
```bash
g++ -O3 main.cpp -o mushroom_static.exe
```

### Run Battle Tests
Run local head-to-head simulations of your compiled binary against the benchmark `mushroom_test.exe` on all 14 boards:
```bash
python battle_test.py log mushroom_static.exe mushroom_test.exe
```
