#!/usr/bin/env python3
"""Run multiple self-play games and aggregate stats."""
import subprocess
import sys
import time

def run_one(input_file, exec1, game_id):
    with open(input_file) as f:
        rows = [line.strip() for line in f if line.strip()]
    board = "INIT " + " ".join(rows)

    p1 = subprocess.Popen([exec1], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=0)
    p2 = subprocess.Popen([exec1], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=0)

    try:
        p1.stdin.write("READY FIRST\n"); p1.stdin.flush()
        p2.stdin.write("READY SECOND\n"); p2.stdin.flush()
        assert p1.stdout.readline().strip() == "OK"
        assert p2.stdout.readline().strip() == "OK"
        p1.stdin.write(board + "\n"); p1.stdin.flush()
        p2.stdin.write(board + "\n"); p2.stdin.flush()

        grid = [[int(c) for c in row] for row in rows]
        owner = [[0]*17 for _ in range(10)]
        prev_passed = False
        turn = 0
        max_time = 0
        while turn < 500:
            turn += 1
            is_first = (turn % 2 == 1)
            current = p1 if is_first else p2
            other = p2 if is_first else p1
            name = "FIRST" if is_first else "SECOND"
            player_num = 1 if is_first else 2

            current.stdin.write("TIME 10000 10000\n"); current.stdin.flush()
            t0 = time.time()
            move = current.stdout.readline().strip()
            elapsed = int((time.time() - t0) * 1000)
            max_time = max(max_time, elapsed)

            parts = move.split()
            r1, c1, r2, c2 = int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])
            other.stdin.write(f"OPP {r1} {c1} {r2} {c2} {elapsed}\n"); other.stdin.flush()

            if r1 >= 0:
                s = 0
                for r in range(r1, r2+1):
                    for c in range(c1, c2+1):
                        s += grid[r][c]
                if s != 10:
                    return (False, 0, 0, turn, max_time, f"Invalid sum={s}")
                for r in range(r1, r2+1):
                    for c in range(c1, c2+1):
                        grid[r][c] = 0
                        owner[r][c] = player_num

            if r1 == -1 and prev_passed:
                break
            prev_passed = (r1 == -1)
    finally:
        try:
            p1.stdin.write("FINISH\n"); p1.stdin.flush()
            p2.stdin.write("FINISH\n"); p2.stdin.flush()
        except: pass
        try: p1.wait(timeout=2)
        except: p1.kill()
        try: p2.wait(timeout=2)
        except: p2.kill()

    p1_cells = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 1)
    p2_cells = sum(1 for r in range(10) for c in range(17) if owner[r][c] == 2)
    return (True, p1_cells, p2_cells, turn, max_time, "OK")

if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    input_file = sys.argv[2] if len(sys.argv) > 2 else "input.txt"
    exec1 = sys.argv[3] if len(sys.argv) > 3 else "./mushroom_static.exe"

    wins = 0
    losses = 0
    draws = 0
    total_turns = 0
    total_max_time = 0
    for i in range(n):
        valid, p1, p2, turns, maxt, status = run_one(input_file, exec1, i)
        if not valid:
            print(f"Game {i+1}: INVALID - {status}")
            continue
        total_turns += turns
        total_max_time = max(total_max_time, maxt)
        if p1 > p2:
            wins += 1; result = f"WIN  {p1}-{p2}"
        elif p1 < p2:
            losses += 1; result = f"LOSS {p1}-{p2}"
        else:
            draws += 1; result = f"DRAW {p1}-{p2}"
        print(f"Game {i+1}: {result}, turns={turns}, max_time={maxt}ms")

    print(f"\nSummary: {wins}W {losses}L {draws}D out of {n} games")
    print(f"Avg turns: {total_turns/n:.1f}, Max per-move time across all games: {total_max_time}ms")
