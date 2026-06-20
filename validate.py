#!/usr/bin/env python3
"""Validate a game log to check that all moves are legal."""
import sys

def parse_log(filename):
    with open(filename) as f:
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
        parts = line.split()
        if len(parts) == 6:
            name, r1, c1, r2, c2, t = parts
            moves.append((name, int(r1), int(c1), int(r2), int(c2)))
    return grid, owner, moves

def rect_sum(grid, owner, r1, c1, r2, c2):
    s = 0
    for r in range(r1, r2+1):
        for c in range(c1, c2+1):
            s += grid[r][c]
    return s

def sides_have_mushroom(grid, r1, c1, r2, c2):
    # Top
    if not any(grid[r1][c] > 0 for c in range(c1, c2+1)):
        return False
    # Bottom
    if not any(grid[r2][c] > 0 for c in range(c1, c2+1)):
        return False
    # Left
    if not any(grid[r][c1] > 0 for r in range(r1, r2+1)):
        return False
    # Right
    if not any(grid[r][c2] > 0 for r in range(r1, r2+1)):
        return False
    return True

def validate(grid, owner, moves):
    for i, (name, r1, c1, r2, c2) in enumerate(moves):
        player = 1 if name == "FIRST" else 2
        opp = 2 if player == 1 else 1
        if r1 == -1:
            continue  # pass
        if r1 > r2 or c1 > c2 or r1 < 0 or c2 >= 17:
            print(f"Turn {i+1} ({name}): INVALID bounds {r1},{c1},{r2},{c2}")
            return False
        s = rect_sum(grid, owner, r1, c1, r2, c2)
        if s != 10:
            print(f"Turn {i+1} ({name}): sum={s}, not 10, move=({r1},{c1},{r2},{c2})")
            return False
        if not sides_have_mushroom(grid, r1, c1, r2, c2):
            print(f"Turn {i+1} ({name}): sides lack mushroom")
            return False
        # Apply move
        for r in range(r1, r2+1):
            for c in range(c1, c2+1):
                grid[r][c] = 0
                owner[r][c] = player
    return True

def count_score(owner):
    p1 = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 1)
    p2 = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 2)
    return p1, p2

if __name__ == "__main__":
    log_file = sys.argv[1] if len(sys.argv) > 1 else "log2.txt"
    grid, owner, moves = parse_log(log_file)
    valid = validate(grid, owner, moves)
    p1, p2 = count_score(owner)
    print(f"Valid: {valid}")
    print(f"FIRST cells: {p1}, SECOND cells: {p2}")
    if not valid:
        print(f"Total moves: {len(moves)}")
