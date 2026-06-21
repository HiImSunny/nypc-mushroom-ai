import subprocess, time

# Read board 10 from log
with open("log/10.txt") as f:
    init_line = f.readline().strip()
rows = init_line.split()[1:]
board = "INIT " + " ".join(rows)

p = subprocess.Popen(["mushroom_static.exe"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=0)

p.stdin.write("READY FIRST\n"); p.stdin.flush()
assert p.stdout.readline().strip() == "OK"
p.stdin.write(board + "\n"); p.stdin.flush()

# Simulate 10 turns (opponent passes every turn to isolate our AI's behavior)
for turn in range(10):
    p.stdin.write("TIME 10000 10000\n"); p.stdin.flush()
    t0 = time.time()
    move = p.stdout.readline().strip()
    elapsed = int((time.time() - t0) * 1000)
    parts = move.split()
    if len(parts) >= 4:
        r1, c1, r2, c2 = int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])
    else:
        print(f"BAD MOVE: {move}")
        break
    print(f'FIRST ({r1},{c1})-({r2},{c2}) t={elapsed}ms')
    p.stdin.write(f"OPP -1 -1 -1 -1 0\n"); p.stdin.flush()
    if r1 == -1:
        break

p.stdin.write("FINISH\n"); p.stdin.flush()
try: p.wait(timeout=3)
except: p.kill()
