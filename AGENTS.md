# NYPC Hackathon — Battle AI (Mushroom Game)

## Project
- **Root**: `D:\Code Project\Python Coding\NYPC Hackathon`
- **Code**: `main.cpp` (v34 — PVS + LMR + Killer moves)
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

## Current Strategy (v34)
- **PVS** (Principal Variation Search) with zero-window on non-PV nodes
- **LMR** (Late Move Reduction): reduce depth for later moves (i >= 3 → -1, i >= 6 → -2)
- **Killer moves**: 2 per ply, +50000/+25000 boost
- **TT**: 1<<16 (64K) entries, replace-always
- **Evaluation**: cells*100 + connectivity*10 + adjMush*8
- **Endgame extension**: ≤2 rects → depth 14, ≤6 rects → depth 12
- **Time mgmt**: budget = (time - SAFETY)/max(2, mush/5 + rects/4), capped 50-700ms
- **Strategic pass**: only pass if pass > move + 100 delta

## Battle History (v34 — 8.5 pts)
| # | Result | Us | Opp | Comment |
|---|--------|----|-----|---------|
| 1 | Win | 91 | 0 | Dominant |
| 2 | Win | 57 | 0 | Dominant |
| 3 | Win | 89 | 7 | Dominant |
| 4 | Win | 66 | 22 | Good |
| 5 | Win | 68 | 40 | Midgame close |
| 6 | Lose | 35 | 41 | -6 close |
| 7 | Draw | 35 | 35 | Equal |
| 8 | Lose | 39 | 44 | -5 close |
| 9 | Win | 46 | 41 | +5 close |
| 10 | Lose | 32 | 36 | -4 close |
| 11 | Win | 37 | 35 | +2 close |
| 12 | Lose | 38 | 46 | -8 |
| 13 | Win | 49 | 45 | +4 close |
| 14 | Lose | 41 | 44 | -3 close |

**7W 1D 5L → 8.5/14 pts**

## Known Issues
- All losses are close (3-6 cells) — endgame evaluation not sharp enough
- Opponent plays very fast (0.001-0.2s) — greedy/local optimal
- LMR may miss crucial deep tactics in endgame
- Pass detection may be too conservative (delta 100 too high)

## Fixes Applied
- `auto` lambda → explicit `pair<int, Rect>&` for C++11 compat (libc++ LLVM-MinGW)
