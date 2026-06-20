#!/usr/bin/env python3
"""Battle test: run our AI vs mushroom_test.exe on 14 boards, aggregate results."""
import subprocess
import sys
import time
import os

def run_game(board_rows, exec1, exec2):
    """Run one game between exec1 (FIRST) and exec2 (SECOND), return (valid, p1, p2, turns, max_time, status)."""
    board = "INIT " + " ".join(board_rows)

    p1 = subprocess.Popen([exec1], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=0)
    p2 = subprocess.Popen([exec2], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=0)

    try:
        p1.stdin.write("READY FIRST\n"); p1.stdin.flush()
        p2.stdin.write("READY SECOND\n"); p2.stdin.flush()
        assert p1.stdout.readline().strip() == "OK"
        assert p2.stdout.readline().strip() == "OK"
        p1.stdin.write(board + "\n"); p1.stdin.flush()
        p2.stdin.write(board + "\n"); p2.stdin.flush()

        grid = [[int(c) for c in row] for row in board_rows]
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

def extract_board_from_log(log_path):
    """Extract the 10 rows from a log file's INIT line."""
    with open(log_path) as f:
        line = f.readline().strip()
    assert line.startswith("INIT "), f"Expected INIT line in {log_path}"
    rows = line[5:].split()
    assert len(rows) == 10, f"Expected 10 rows, got {len(rows)}"
    return rows

if __name__ == "__main__":
    log_dir = sys.argv[1] if len(sys.argv) > 1 else "log"
    exec1 = sys.argv[2] if len(sys.argv) > 2 else "mushroom_static.exe"
    exec2 = sys.argv[3] if len(sys.argv) > 3 else "mushroom_test.exe"
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 14

    total_our_cells = 0
    total_opp_cells = 0
    total_turns = 0
    total_max_time = 0
    wins = losses = draws = 0

    print(f"{'Game':>6} {'Result':>20} {'Our cells':>10} {'Opp cells':>10} {'Turns':>6} {'Max ms':>7}")
    print("-" * 65)

    actual_games = 0
    for i in range(1, n + 1):
        log_path = os.path.join(log_dir, f"{i}.txt")
        if not os.path.exists(log_path):
            log_path = os.path.join(log_dir, f"{i} (1).txt")
        if not os.path.exists(log_path):
            print(f"{i:>6} {'SKIP':>20} {'-':>10} {'-':>10} {'-':>6} {'-':>7}  (file not found)")
            continue
        actual_games += 1
        board_rows = extract_board_from_log(log_path)

        valid, our, opp, turns, maxt, status = run_game(board_rows, exec1, exec2)
        if not valid:
            print(f"{i:>6} {'INVALID':>8} {'-':>10} {'-':>10} {turns:>6} {maxt:>7}  {status}")
            continue

        total_our_cells += our
        total_opp_cells += opp
        total_turns += turns
        total_max_time = max(total_max_time, maxt)

        if our > opp:
            wins += 1; result = f"WIN ({our}-{opp})"
        elif our < opp:
            losses += 1; result = f"LOSE ({our}-{opp})"
        else:
            draws += 1; result = f"DRAW ({our}-{opp})"
        print(f"{i:>6} {result:>20} {our:>10} {opp:>10} {turns:>6} {maxt:>7}")

    print("-" * 65)
    print(f"\nRan {actual_games} boards. Tong ket: {wins}W / {draws}D / {losses}L ({wins/max(1,actual_games)*100:.0f}% win rate)")
    if actual_games > 0:
        print(f"TB turns: {total_turns/max(1,actual_games):.1f}, Max per-move time: {total_max_time}ms")
        print(f"Total our cells: {total_our_cells}, Total opp cells: {total_opp_cells}")
        print(f"Cells margin: +{total_our_cells - total_opp_cells} ({'+' if total_our_cells > total_opp_cells else ''}{(total_our_cells-total_opp_cells)/total_opp_cells*100:.1f}%)")
