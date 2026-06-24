# NYPC Hackathon — Battle AI (Mushroom Game)

## Project
- **Root**: `D:\Code Project\Python Coding\NYPC Hackathon`
- **Code**: `main.cpp` (v57 — PVS + LMR + Killer moves + TT (1M) + ID + Column Pruning + Dynamic Cap + Granular Endgame + ID Fix + Move Ordering Heuristic + Terminating Pass Optimization + Mathematically Aligned Move Ordering + Precise Time Budgeting + stateHash TT Fix + Fast Frontier)
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

## Current Strategy (v57)
- **PVS** (Principal Variation Search) with zero-window on non-PV nodes
- **LMR** (Late Move Reduction): reduce depth for later moves (i >= 4 → -1, i >= 7 → -2)
- **Killer moves**: 2 per ply, +50000/+25000 boost
- **TT**: 1<<20 (1M) entries, replace-always. Indexed by `stateHash` (which XORs board hash with player and prevPassed states) to prevent TT lookup corruption during the strategic pass checks.
- **Evaluation**: cells*100 + connectivity*10 + adjMush*8 (Prioritizes compact connected territories)
- **Move Ordering**: quick evaluation uses `totalCells * 100 + oppCells * 200 - myLost * 100` plus a frontier threat bonus (`+30` per unclaimed adjacent mushroom calculated efficiently using border outer strips) and a mushroom sum bonus.
- **Endgame extension**: ≤2 rects → depth 18, ≤5 rects → depth 14, ≤8 rects → depth 12
- **Search Widths (K)**: K (12/9/7/5) based on depth for maximum coverage at shallow levels and tight/deep search at deeper levels.
- **Strategic pass**: after ID, test if passing is better than our best move. If the opponent has passed, we can pass to terminate the game immediately if we are winning.
- **Time mgmt**: budget = (time - SAFETY)/max(1, totalMushSum / 18), dynamically capped between 50ms and min(1200ms, remainingTime / 8).
- **Iterative Deepening**: fixed pollution bug (incomplete depths are discarded) + time-to-completion prediction (`lastDepthTime * 3` check). Default maxDepth is 12.

## Battle History (v51 — 13.0 pts) ⭐ Prior Record
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
- v51 had a single loss in Game 12 (45-54) against a strong opponent that uses quick positional tricks to seize large territory slabs early.
- v56 (which introduced the new move ordering heuristics and alpha-tightening) regressed to 13W 1L (losing Game 4) due to a Transposition Table lookup corruption bug in the strategic pass checks.

## Fixes Applied
- **v53**: Addressed coordinates tie-breakers and oppPassed reset.
- **v55**: Reverted custom sorting lambda to standard std::sort to recover fine-tuned pruning behaviors.
- **v56**: Overhauled move ordering using mushroom and frontier threat bonuses.
- **v57**: Fixed the TT lookup corruption bug by XORing the player ID and prevPassed status into the Zobrist hash (`stateHash`). Simplified and optimized the frontier calculations in `scoreRectQuick` using outer border strips. Restored `stable_sort` to ensure search determinism. Achieved a perfect **14W 0D 0L** local battle test and beat v51 head-to-head with **9W 5L (+14 cells)**.

