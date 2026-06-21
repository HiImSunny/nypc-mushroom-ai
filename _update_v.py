f = "D:\\Code Project\\Python Coding\\NYPC Hackathon\\main.cpp"
with open(f, "rb") as fh:
    data = fh.read()

# Update VERSION_STR
old_v = b'#define VERSION_STR "mushroom_ai_v6_final"'
new_v = b'#define VERSION_STR "mushroom_ai_v10_20260620"'
data = data.replace(old_v, new_v)

# Update change log: replace S9 closing + S10 entry
old_log = b'//   Insight: cai thien tu 57% len 71% nho search sau 2 ply o endgame\r\n// ============================================================'
new_log = b'//   Insight: cai thien tu 57% len 71% nho search sau 2 ply o endgame\r\n// Session 10 - 2026-06-20\r\n//   Tang threshold depth 8: scored.size() <= 4 => <= 8\r\n//   Self-play: 5W 0L (72-44, 423ms), Battle: 9W 1D 4L (64%, cells +138)\r\n// ============================================================'
data = data.replace(old_log, new_log)

with open(f, "wb") as fh:
    fh.write(data)
print("OK")
