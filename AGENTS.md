# NYPC Hackathon — Battle AI (Mushroom Game)

## Project
- **Root**: `D:\Code Project\Python Coding\NYPC Hackathon`
- **Code**: `main.cpp` (v51 — PVS + LMR + Killer moves + TT (1M) + ID + Column Pruning + Dynamic Cap + Granular Endgame + ID Fix + Move Ordering Heuristic + Terminating Pass Optimization + Mathematically Aligned Move Ordering + Precise Time Budgeting)
- **Logs**: `log\` folder (numbered battle logs)
- **Binaries**: `mushroom_v*.exe`

## Workflow
1. User submits `main.cpp` to NYPC website → system does 14 battles
2. User copies battle results from web into chat
3. I check `log\` folder for latest battle log
4. Analyze losses/draws → improve algorithm → fix bugs → compile
5. User re-submits to web → repeat

## Game
- 10×17 board (mushrooms 0-9), rectangles sum=10, must have mushroom on all 4 sides
- Double pass = game over. More cells = win.
- Each player has own time bank. Time management critical.

## Current Strategy (v51)
- **PVS** (Principal Variation Search) with zero-window on non-PV nodes
- **LMR** (Late Move Reduction): reduce depth for later moves (i >= 3 → -1, i >= 6 → -2)
- **Killer moves**: 2 per ply, +50000/+25000 boost
- **TT**: 1<<20 (1M) entries, replace-always
- **Evaluation**: cells*100 + connectivity*10 + adjMush*8 (Prioritizes compact connected territories)
- **Move Ordering**: quick evaluation uses `totalCells * 100 + oppCells * 200 - myLost * 100` (mathematically exact representation of the cell count difference gain)
- **Endgame extension**: ≤2 rects → depth 18, ≤5 rects → depth 14, ≤8 rects → depth 12
- **Search Widths (K)**: K (12/7/5) for deep lookahead and maximum time efficiency, saving time for the endgame
- **Strategic pass**: after ID, test if passing is better than our best move. If the opponent has passed, we can pass to terminate the game immediately and evaluate the exact final board state.
- **Time mgmt**: budget = (time - SAFETY)/max(1, totalMushSum / 18), dynamically capped between 50ms and min(1200ms, remainingTime / 8).
- **Iterative Deepening**: fixed pollution bug (incomplete depths are discarded) + time-to-completion prediction (`lastDepthTime * 3` check). Default maxDepth is 10 (was 8).

## Battle History (v51 — 13.0 pts) ⭐ New Record
| # | Result | Us | Opp | Comment |
|---|--------|----|-----|---------|
| 1 | Win | 83 | 0 | Opponent inactive |
| 2 | Win | 79 | 0 | Opponent inactive |
| 3 | Win | 78 | 7 | Dominant (+71) |
| 4 | Win | 46 | 27 | Dominant (+19) |
| 5 | Win | 81 | 28 | Dominant (+53) |
| 6 | Win | 48 | 44 | Good (+4) |
| 7 | Win | 39 | 34 | Reclaimed (+5) |
| 8 | Win | 45 | 40 | Reclaimed (+5) |
| 9 | Win | 46 | 43 | Reclaimed (+3) |
| 10 | Win | 48 | 44 | Reclaimed (+4) |
| 11 | Win | 33 | 32 | Close (+1) |
| 12 | Lose | 45 | 54 | -9 |
| 13 | Win | 52 | 46 | Reclaimed (+6) |
| 14 | Win | 37 | 34 | Reclaimed (+3) |

**13W 0D 1L → 13.0/14 pts**

## Known Issues
- v51 has a single loss in Game 12 (45-54) against a strong opponent that uses quick positional tricks to seize large territory slabs early. The opponent spent only ~150ms/move, suggesting a high-speed deterministic strategy.
- v52 (adaptive strategic pass with `passMargin=0` in endgame) caused regressions (11W 3L locally vs mushroom_test) due to premature passes — reverted.

## Fixes Applied
- **v50**: Corrected the `myLost` coefficient from `150` to `100` to mathematically align sorting with cell count difference gain.
- **v51**: Replaced the heuristic move estimator in time budgeting with a mathematically precise upper-bound estimator `estMovesLeft = totalMushSum / 18` (since each move consumes exactly 10 mushroom points, the remaining moves for both players combined is strictly bounded by `totalMushSum / 10`). This safely doubles our time budget per move, allowing the engine to search deeper and match the lookahead horizon of advanced tournament opponents. Achieved **13W 0D 1L → 13.0/14 pts** on the tournament web.
- **v52** (reverted): Adaptive strategic pass depth (depth 5 in endgame) and `passMargin=0` when `rects <= 8` caused premature passes and regressed to 11W 3L locally.
