import subprocess
import sys
import time

def run_test(log_path, exec_path):
    with open(log_path) as f:
        lines = [line.strip() for line in f if line.strip()]
    
    init_line = lines[0]
    moves = []
    for line in lines[1:]:
        if line.startswith("FIRST") or line.startswith("SECOND"):
            parts = line.split()
            player = parts[0]
            r1, c1, r2, c2 = int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])
            elapsed = int(parts[5])
            moves.append((player, r1, c1, r2, c2, elapsed))
        elif line.startswith("FINISH"):
            break

    # Determine if our AI is FIRST or SECOND
    # Let's assume we are testing as the first player or second player based on the log
    # For battle 13, the first player passed at line 32. So we are FIRST.
    # Let's run the AI as FIRST.
    p = subprocess.Popen([exec_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=None, text=True, bufsize=0)
    
    p.stdin.write("READY FIRST\n"); p.stdin.flush()
    assert p.stdout.readline().strip() == "OK"
    p.stdin.write(init_line + "\n"); p.stdin.flush()

    move_idx = 0
    # Moves are ordered chronologically.
    # FIRST plays, then SECOND plays, then FIRST plays, etc.
    # We feed the OPP moves to our AI when it is the opponent's turn.
    # When it is our turn, we send TIME, read the AI's move, check if it matches, and then continue.
    
    our_time = 10000
    opp_time = 10000
    while move_idx < len(moves):
        curr_player, r1, c1, r2, c2, elapsed = moves[move_idx]
        if curr_player == "FIRST":
            # It's our turn!
            p.stdin.write(f"TIME {our_time} {opp_time}\n"); p.stdin.flush()
            t0 = time.time()
            our_move = p.stdout.readline().strip()
            our_elapsed = int((time.time() - t0) * 1000)
            print(f"Turn {move_idx+1} (FIRST - Us): AI played {our_move} (expected {r1} {c1} {r2} {c2}), took {our_elapsed}ms (log said {elapsed}ms)")
            our_time -= elapsed
            move_idx += 1
        else:
            # Opponent's turn!
            print(f"Turn {move_idx+1} (SECOND - Opp): Feeding OPP {r1} {c1} {r2} {c2} {elapsed}")
            p.stdin.write(f"OPP {r1} {c1} {r2} {c2} {elapsed}\n"); p.stdin.flush()
            opp_time -= elapsed
            move_idx += 1

    p.stdin.write("FINISH\n"); p.stdin.flush()
    try: p.wait(timeout=3)
    except: p.kill()

if __name__ == "__main__":
    log_path = sys.argv[1] if len(sys.argv) > 1 else "log/13.txt"
    exec_path = sys.argv[2] if len(sys.argv) > 2 else "mushroom_static.exe"
    run_test(log_path, exec_path)
