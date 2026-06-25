# NYPC Hackathon — Battle AI (Mushroom Game)

## Project
- **Root**: `D:\Code Project\Python Coding\NYPC Hackathon`
- **Code**: [main.cpp](file:///D:/Code%20Project/Python%20Coding/NYPC%20Hackathon/main.cpp) (v66 — PVS + LMR + Killer moves + TT (2M) + ID + Column Pruning + Dynamic Cap + Granular Endgame + ID Fix + Move Ordering Heuristic + Terminating Pass Optimization + Mathematically Aligned Move Ordering + Precise Time Budgeting + stateHash TT Fix + Fast Frontier + Integrated Symmetric PASS Search + Tuned Frontier Weight)
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

## Current Strategy (v66)
- **PVS** (Principal Variation Search) with zero-window on non-PV nodes
- **LMR** (Late Move Reduction): reduce depth for later moves (i >= 4 → -1, i >= 7 → -2)
- **Killer moves**: 2 per ply, +50000/+25000 boost
- **TT**: 1<<21 (2M) entries, replace-always. Indexed by `stateHash` (which XORs board hash with player and prevPassed states).
- **TT Entry Score**: Stored as `int32_t` (was `int16_t`) to prevent truncation of terminal evaluations.
- **Evaluation**: cells*100 + connectivity*10 + adjMush*12 (Prioritizes compact connected territories with high expansion potential)
- **Terminal Evaluation**: cells * 100 with +/- 1000000 win/loss bonuses to mathematically align terminal wins/losses as dominant over heuristic scores.
- **Move Ordering**: quick evaluation uses `totalCells * 100 + oppCells * 200 - myLost * 100` plus a frontier threat bonus (`+30` per unclaimed adjacent mushroom calculated efficiently using border outer strips) and a mushroom sum bonus.
- **Endgame extension**: ≤2 rects → depth 18, ≤5 rects → depth 16, ≤8 rects → depth 14
- **Search Widths (K)**: K (12/9/7/5) based on depth for maximum coverage at shallow levels and tight/deep search at deeper levels.
- **Symmetric Pass Search**: The PASS move is integrated directly into the `alphaBeta` search loop at all plies (sorted last).
- **Time mgmt**: budget = (time - SAFETY)/max(1, totalMushSum / 18), dynamically capped between 50ms and min(1200ms, remainingTime / 8).
- **Iterative Deepening**: fixed pollution bug (incomplete depths are discarded) + time-to-completion prediction (`lastDepthTime * 2.2` check). Default maxDepth is 14.

## Battle History (v66 — 13.0 pts) Local
| # | Result | Us | Opp | Comment |
|---|--------|----|-----|---------|
| 1 | Win | 55 | 47 | Reclaimed (+8) |
| 2 | Win | 43 | 34 | Reclaimed (+9) |
| 3 | Lose | 43 | 65 | Local Test Opponent (different path) |
| 4 | Win | 67 | 30 | Dominant (+37) |
| 5 | Win | 61 | 52 | Reclaimed (+9) |
| 6 | Win | 59 | 39 | Reclaimed (+20) |
| 7 | Win | 57 | 36 | Reclaimed (+21) |
| 8 | Win | 53 | 36 | Reclaimed (+17) |
| 9 | Win | 49 | 39 | Reclaimed (+10) |
| 10 | Win | 66 | 27 | Dominant (+39) |
| 11 | Win | 50 | 37 | Reclaimed (+13) |
| 12 | Win | 55 | 53 | Reclaimed (+2) |
| 13 | Win | 68 | 40 | Dominant (+28) |
| 14 | Win | 58 | 48 | Reclaimed (+10) |

**13W 0D 1L → 13.0/14 pts**

## Known Issues
- None currently known.

## Fixes Applied
- **v53**: Addressed coordinates tie-breakers and oppPassed reset.
- **v55**: Reverted custom sorting lambda to standard std::sort to recover fine-tuned pruning behaviors.
- **v56**: Overhauled move ordering using mushroom and frontier threat bonuses.
- **v57**: Fixed the TT lookup corruption bug by XORing the player ID and prevPassed status into the Zobrist hash (`stateHash`). Simplified and optimized the frontier calculations in `scoreRectQuick` using outer border strips. Restored `stable_sort` to ensure search determinism.
- **v59**: Solved strategic pass horizon effect by integrating the PASS option search directly within each completed depth of the main Iterative Deepening loop. Avoided the depth asymmetry (comparing depth-3 pass check to depth-11 TT hits for rect moves). Achieved a perfect **14W 0D 0L** local battle test vs `mushroom_test.exe` with a +294 cell margin (+54.2%).
- **v60**: Resolved the strategic pass horizon / asymmetry bug by integrating the PASS option directly as a symmetric move searched inside `alphaBeta` at all plies. Fixed cases where the search assumed the opponent could not pass strategically and was forced to play losing moves, leading to premature passes. Achieved a perfect **14W 0D 0L** in local battle test against `mushroom_test.exe` with a +288 cell margin (+52.9%).
- **v62**: Resolved strategic pass blunders on the web (which caused losses in web battles 8, 11, and draw in 14) by introducing `evaluateTerminal` to return large dominant win/loss bonuses (+/- 1,000,000) for terminal states, forcing the AI to play moves instead of passing into a loss. Fixed a transposition table score truncation bug by upgrading `TTEntry::score` from `int16_t` to `int32_t`. Disabled root-level pass search when the opponent has passed and we are not winning (never resign when moves remain).
- **v63**: Resolved strategic pass blunders when the opponent has not passed yet but we are losing (which caused losses in web battles 8 and 13) by strictly disabling root-level pass search in `selectMove` when `myCells < oppCells` (never pass when losing, as passing is equivalent to resigning/forfeiting). Achieved a perfect **14W 0D 0L** local battle test.
- **v65**: Enlarged Transposition Table size to `1 << 21` (2M entries). Granularized iterative deepening depth extensions to reach Depth 14/16 earlier in the endgame. Tuned the ID time-to-completion prediction threshold from `3.0x` to `2.2x` to search deeper when time is abundant, resulting in a significantly higher cell margin (+201 cells total).
- **v66**: Tuned evaluation function weights using local A/B testing on all 14 boards. Increased the weight of adjacent mushrooms (frontier potential) from 8 to 12. In a head-to-head 14-game match against the v65 baseline, the v66 code won **11W 3L (79% win rate)** with a massive **+153 cell margin** (+24.9%). Updated both `main.cpp` and `main_SUBMIT.cpp`.
