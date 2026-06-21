#!/usr/bin/env python3
"""Visualize a game log - show board state at each turn."""
import sys

def parse_log(log_path):
    with open(log_path) as f:
        lines = [l.strip() for l in f if l.strip()]
    
    init_line = lines[0]
    assert init_line.startswith("INIT")
    rows = init_line.split()[1:]
    
    grid = [[int(c) for c in row] for row in rows]
    owner = [[0]*17 for _ in range(10)]
    
    moves = []
    for line in lines[1:]:
        if line == "FINISH":
            break
        if line.startswith("SCORE"):
            continue
        parts = line.split()
        if len(parts) == 6:
            name, r1, c1, r2, c2, t = parts
            moves.append((name, int(r1), int(c1), int(r2), int(c2), int(t)))
    
    return grid, owner, moves

def print_board(grid, owner, highlight_move=None, turn_label=""):
    symbols = {0: ".", 1: "F", 2: "S"}
    print(f"  {turn_label}")
    print("  " + "".join(str(c % 10) for c in range(17)))
    for r in range(10):
        row = []
        for c in range(17):
            if highlight_move and r >= highlight_move[0] and r <= highlight_move[2] and c >= highlight_move[1] and c <= highlight_move[3]:
                # Highlight the move cells
                row.append(f"\033[7m{grid[r][c] if grid[r][c] > 0 else symbols[owner[r][c]]}\033[0m")
            elif grid[r][c] > 0:
                row.append(str(grid[r][c]))
            elif owner[r][c] > 0:
                row.append(symbols[owner[r][c]].lower())
            else:
                row.append(".")
        print(f"{r} " + "".join(row))
    p1 = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 1)
    p2 = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 2)
    mush = sum(1 for r in range(10) for c in range(17) if grid[r][c] > 0)
    print(f"  FIRST={p1} SECOND={p2} mushrooms={mush}")

def analyze(log_path, max_turns=15):
    grid, owner, moves = parse_log(log_path)
    print(f"\n{'='*60}")
    print(f"Analyzing: {log_path}")
    print(f"{'='*60}")
    
    # Print initial board
    print_board(grid, owner, turn_label="INITIAL BOARD")
    
    for i, (name, r1, c1, r2, c2, t) in enumerate(moves):
        if i >= max_turns:
            break
        
        player_num = 1 if name == "FIRST" else 2
        
        if r1 >= 0:
            # Apply move
            s = 0
            cells = []
            for r in range(r1, r2+1):
                for c in range(c1, c2+1):
                    s += grid[r][c]
                    cells.append((r, c, grid[r][c]))
            for r in range(r1, r2+1):
                for c in range(c1, c2+1):
                    grid[r][c] = 0
                    owner[r][c] = player_num
        
        label = f"Turn {i+1}: {name} ({r1},{c1})-({r2},{c2})  sum={s if r1>=0 else 'pass'}  time={t}ms"
        print()
        print_board(grid, owner, highlight_move=(r1, c1, r2, c2) if r1>=0 else None, turn_label=label)
    
    # Final score
    p1 = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 1)
    p2 = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 2)
    print(f"\nFINAL: FIRST={p1} SECOND={p2} ({'FIRST wins' if p1>p2 else 'SECOND wins' if p2>p1 else 'DRAW'})")

if __name__ == "__main__":
    log_path = sys.argv[1] if len(sys.argv) > 1 else "log/10.txt"
    max_turns = int(sys.argv[2]) if len(sys.argv) > 2 else 15
    analyze(log_path, max_turns)
