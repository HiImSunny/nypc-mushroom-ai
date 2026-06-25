#!/usr/bin/env python3
"""Simulator to run a full game between two mushroom AI processes."""
import subprocess
import sys
import time

def run_game(input_file, log_file, exec1, exec2):
    with open(input_file) as f:
        rows = [line.strip() for line in f if line.strip()]
    assert len(rows) == 10, f"Expected 10 rows, got {len(rows)}"
    board = "INIT " + " ".join(rows)

    p1 = subprocess.Popen([exec1], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, bufsize=0)
    p2 = subprocess.Popen([exec2], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, bufsize=0)

    log = []

    def read_line(p, name):
        line = p.stdout.readline()
        if not line:
            raise RuntimeError(f"{name} died.")
        return line.rstrip("\n")

    def send(p, msg):
        p.stdin.write(msg + "\n")
        p.stdin.flush()

    send(p1, "READY FIRST")
    send(p2, "READY SECOND")

    r1 = read_line(p1, "p1")
    r2 = read_line(p2, "p2")
    assert r1 == "OK", f"p1: {r1}"
    assert r2 == "OK", f"p2: {r2}"
    print("READY OK")

    send(p1, board)
    send(p2, board)
    print("INIT sent")

    prev_passed = False
    turn = 0
    max_turns = 500
    while turn < max_turns:
        turn += 1
        is_first = (turn % 2 == 1)
        current = p1 if is_first else p2
        name = "FIRST" if is_first else "SECOND"
        other = p2 if is_first else p1

        send(current, "TIME 10000 10000")
        start = time.time()
        move = read_line(current, name)
        elapsed_ms = int((time.time() - start) * 1000)

        parts = move.split()
        r1_m, c1_m, r2_m, c2_m = int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])
        log.append(f"{name} {r1_m} {c1_m} {r2_m} {c2_m} {elapsed_ms}")
        print(f"Turn {turn}: {name} plays {move} ({elapsed_ms}ms)")

        send(other, f"OPP {r1_m} {c1_m} {r2_m} {c2_m} {elapsed_ms}")

        if r1_m == -1 and prev_passed:
            break
        prev_passed = (r1_m == -1)

    send(p1, "FINISH")
    send(p2, "FINISH")
    try:
        p1.wait(timeout=5)
    except subprocess.TimeoutExpired:
        p1.kill()
    try:
        p2.wait(timeout=5)
    except subprocess.TimeoutExpired:
        p2.kill()

    print(f"\nGame ended after {turn} turns, {len(log)} log entries")

    with open(log_file, "w") as f:
        f.write(board + "\n")
        for entry in log:
            f.write(entry + "\n")
        f.write("FINISH\n")
    return log

if __name__ == "__main__":
    input_file = sys.argv[1] if len(sys.argv) > 1 else "input.txt"
    log_file = sys.argv[2] if len(sys.argv) > 2 else "log.txt"
    exec1 = sys.argv[3] if len(sys.argv) > 3 else "./mushroom_static.exe"
    exec2 = sys.argv[4] if len(sys.argv) > 4 else exec1
    run_game(input_file, log_file, exec1, exec2)
