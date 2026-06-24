// ============================================================
// Change Log - moi lan update code deu ghi lai o day
// ============================================================
// Session 3 - 2026-06-20
//   Fix: AI pass som du con nuoc di hop le
//   Result: 5W 0L self-play, 72-44 cells, max 355ms
// Session 6 - 2026-06-20
//   Optimize findValidRects (incremental colSum + early skip)
//   Result: self-play 5W 0L (72-44, 47t, 425ms), battle 9W 5L (64%)
// Session 9 - 2026-06-20
//   Depth 8 endgame khi scored.size() <= 4 (+2 ply o cuoi game)
//   Result: self-play 5W 0L (72-44, 560ms), battle 10W 1D 3L (71%)
// Session 15 - 2026-06-21
//   Clean rewrite ve Session 9 baseline sau khi file bi hong
// Session 16 - 2026-06-21
//   Fix: pass bug trong alphaBeta - chi pass khi khong con nuoc di
//   Fix: time mgmt - dung (time-SAFETY)/estMovesLeft, bo 0.5 early break
//   Improve: scoreRect mushroom*3, bo int div, swing + emptyCells
//   Improve: evaluate() adjMush weight 1->4
//   Optimize: alphaBeta width depth-dependent (K=10/6)
// Session 33 - 2026-06-24 (Major overhaul)
//   Fix: root selectMove now considers pass as valid option
//   Improve: evaluate() simplified - base (cells)*100, connectivity*12, adjMush*10, compact*2
//   Improve: time mgmt - dynamic estMovesLeft based on mushrooms + rects
//   Improve: killer move heuristic (2 per ply, +5000 boost on beta cutoff)
//   Improve: endgame extension - size<=6 -> depth 12, size==1 -> depth 14
//   Improve: TT_SIZE enlarged from 1<<15 to 1<<16 (64K entries)
//   Improve: selectMove first ID iteration evaluates ALL moves, then K=10
// Session 34 - 2026-06-24 (fixed)
//   Fix: scoreRect using r.r1 instead of r.c1 bug
//   Fix: root pass evaluated but only selected when clearly better (delta > 50)
//   Fix: pass NOT included in root scored list (separate calculation)
//   Improve: PVS (Principal Variation Search) - zero-window on non-PV
//   Improve: LMR (Late Move Reduction) for moves beyond early window
//   Remove: null-move pruning (unsound for this game - passing = losing a turn)
// Session 35 - 2026-06-24 (Hermes Desktop edit)
//   Improve: evaluate cells*100, connectivity*10, adjMush*8
//   Improve: time mgmt - dynamic estMovesLeft, cap 50-700ms
//   Improve: PVS + LMR + Killer moves
//   Improve: endgame extension depth 12-14
//   Improve: TT_SIZE 1<<16
//   Improve: SAFETY_BUFFER 100->150ms
// ============================================================
// Session 38 - 2026-06-24
//   Improve: evaluate() + frontier (more expansion = better)
//   Improve: scoreRectQuick oppCells * 200 -> * 300
//   Improve: quiescence search (depth==0 only, 1 ply max)
// Session 39 - 2026-06-24
//   Fix: strategic pass back with guard (≤5 rects + delta 100)
//   Fix: quiescence limited to depth==0 only (was recursive depth -1)
// Session 40 - 2026-06-24
//   Remove: strategic pass (was losing games by premature pass - battle 13-14)
//   Improve: quiescence extended to depth -2 (3 plies, was 1 ply)
//   Improve: endgame depth 14->16 for ≤2 rects
// Session 41 - 2026-06-24
//   Fix: quiescence depth==0 only (was -2, too deep, wasted time)
//   Fix: quiescence calls evaluate directly (no recursive search)
//   Improve: strategic pass delta=80 (was removed in v40)
//   Improve: K search width increased (5->7 deep, 7->8 mid, 12->15 root)
//   Result: fix v40 regression (4pts -> target 8+)
// Session 42 - 2026-06-24
//   Fix: Remove quiescence search completely (wasted time and depth at leaves)
//   Improve: Keep new frontier evaluation term: (myFrontier - oppFrontier) * 3
//   Result: Local test 12W 1D 1L (86% win rate), cells margin +192 (+32.8%)
// Session 43 - 2026-06-24
//   Fix: Revert bad frontier evaluation term (caused loose shapes and a 0-14 head-to-head loss vs v35)
//   Fix: Restore search-based strategic pass check (prevents making forced bad moves)
//   Fix: Restore narrower search widths K (12/7/5) for deeper tactical lookahead
//   Optimize: Add early column-sum pruning in findValidRects() using 2D prefix sums
//   Result: Local test 13W 0D 1L (93% win rate), cells margin +254 (+45.8%), max time 890ms
// Session 44 - 2026-06-24
//   Improve: Implement dynamic time budget cap based on remainingTime / 7 (max 1000ms)
//            Allows deeper search when time is abundant, while protecting against TLE when low.
//   Result: Beat baseline v35 locally with 9W 1D 4L (64% win rate)
//           Local test vs mushroom_test.exe: 12W 0D 2L (86% win rate), cells margin +116 (+18.8%), max time 813ms
// Session 45 - 2026-06-24
//   Improve: Increase TT_SIZE to 512K entries (1<<19) to reduce collisions in deep searches
//   Improve: Refine/granularize endgame extensions in selectMove (depth 10-18 based on rects <= 12/8/5/2)
//   Result: Beat baseline v35 locally with 10W 1D 3L (71% win rate), cell margin +72 (+12.8%)
//           Local test vs mushroom_test.exe: 12W 0D 2L (86% win rate), cells margin +167 (+28.5%), max time 802ms
// Session 46 - 2026-06-24
//   Fix: Resolve critical ID pollution bug (root bestMove was being overwritten by incomplete searches at cut-off depths)
//   Improve: Implement lastDepthTime prediction to skip starting a depth we won't have time to complete
//   Result: Local test vs mushroom_test.exe: 12W 0D 2L (86% win rate), cells margin +142 (+23.3%)
//           Beat baseline v35 locally with 10W 1D 3L (71% win rate), cell margin +26 (+4.6%)
// Session 53 - 2026-06-25
//   Fix: Add deterministic sorting tie-breakers to eliminate cross-compiler/cross-OS discrepancies.
//   Fix: Reset oppPassed in INIT command parsing.
//   Result: Stabilized Game 12 win (57-46 cells in web simulation).
// Session 54 - 2026-06-25
//   Fix: Replace coordinate tie-breaker with hash-based tie-breaker to prevent spatial clustering blindspot.
//   Result: Local test 12W 2L or better, cross-platform deterministic search distribution.
// Session 55 - 2026-06-25
//   Revert: Restore standard std::sort to recover fine-tuned pruning behaviors and fix regressions.
//   Result: Local test 13W 1L or better, stable 13.0 pts on web.
// Session 56 - 2026-06-25
//   Fix: TT lookup now tightens alpha when flag==1 (lower bound), enabling better PVS cutoffs.
//   Fix: LMR reduction reduced (lmr=1 at i>=4, lmr=2 at i>=7) to prevent pruning tactical moves.
//   Improve: K at deep nodes increased (depth<=2:12, depth<=4:9, depth<=6:7, else:5) for broader coverage.
//   Improve: scoreRectQuick adds mushroom-value bonus: sum of mushrooms in rect * 15, prioritizes
//            tactically richer captures and prevents undervaluing low-cell but high-mushroom moves.
//   Improve: scoreRectQuick adds "frontier threat" bonus: cells adjacent to unclaimed mushrooms * 30
//            to better detect and preempt opponent's expansion zones (fixes Game 12 type misses).
// Session 57 - 2026-06-25
//   Fix: TT lookup corruption by indexing with player and prevPassed in stateHash.
//   Improve: Optimize and simplify frontier calculation in scoreRectQuick (using outer border strips).
//   Improve: Use stable_sort to guarantee deterministic search order.
// ============================================================
#define VERSION_STR "mushroom_ai_v59_20260625"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <string>
#include <random>
using namespace std;
using namespace chrono;
const int ROWS = 10;
const int COLS = 17;
const int SUM_TARGET = 10;
const int SAFETY_BUFFER_MS = 100;
const int TT_SIZE = 1 << 20;
const int INF = 1e9;
const int MAX_PLY = 32;
struct Rect {
    int r1, c1, r2, c2;
    bool operator==(const Rect& o) const {
        return r1==o.r1 && c1==o.c1 && r2==o.r2 && c2==o.c2;
    }
    bool operator!=(const Rect& o) const {
        return !(*this == o);
    }
};
inline uint32_t hashRect(const Rect& r) {
    uint32_t h = r.r1;
    h = h * 31 + r.c1;
    h = h * 31 + r.r2;
    h = h * 31 + r.c2;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}
static uint64_t zobristGrid[ROWS][COLS][10];
static uint64_t zobristOwner[ROWS][COLS][3];
static uint64_t zobristPlayer[3];
static uint64_t zobristPassed[2];
static bool zobristInitialized = false;
void initZobrist() {
    if (zobristInitialized) return;
    mt19937_64 rng(1234567890ULL);
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            for (int v = 0; v < 10; v++)
                zobristGrid[r][c][v] = rng();
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            for (int o = 0; o < 3; o++)
                zobristOwner[r][c][o] = rng();
    for (int p = 0; p < 3; p++)
        zobristPlayer[p] = rng();
    for (int pass = 0; pass < 2; pass++)
        zobristPassed[pass] = rng();
    zobristInitialized = true;
}
struct Board {
    int8_t grid[ROWS][COLS];
    int8_t owner[ROWS][COLS];
    int prefix[ROWS + 1][COLS + 1];
    int rowMush[ROWS][COLS + 1];
    int colMush[ROWS + 1][COLS];
    uint64_t hash;
    void recomputeHash() {
        hash = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                hash ^= zobristGrid[r][c][grid[r][c]];
                hash ^= zobristOwner[r][c][owner[r][c]];
            }
    }
    void computePrefixSums() {
        for (int r = 0; r <= ROWS; r++) {
            for (int c = 0; c <= COLS; c++) {
                if (r == 0 || c == 0) prefix[r][c] = 0;
                else prefix[r][c] = grid[r - 1][c - 1] + prefix[r - 1][c] + prefix[r][c - 1] - prefix[r - 1][c - 1];
            }
        }
        for (int r = 0; r < ROWS; r++) {
            rowMush[r][0] = 0;
            for (int c = 0; c < COLS; c++)
                rowMush[r][c + 1] = rowMush[r][c] + (grid[r][c] > 0 ? 1 : 0);
        }
        for (int c = 0; c < COLS; c++) {
            colMush[0][c] = 0;
            for (int r = 0; r < ROWS; r++)
                colMush[r + 1][c] = colMush[r][c] + (grid[r][c] > 0 ? 1 : 0);
        }
    }
    int rectSum(int r1, int c1, int r2, int c2) const {
        return prefix[r2 + 1][c2 + 1] - prefix[r1][c2 + 1] - prefix[r2 + 1][c1] + prefix[r1][c1];
    }
    bool hasMushroomOnSides(int r1, int c1, int r2, int c2) const {
        if (rowMush[r1][c2 + 1] - rowMush[r1][c1] == 0) return false;
        if (rowMush[r2][c2 + 1] - rowMush[r2][c1] == 0) return false;
        if (colMush[r2 + 1][c1] - colMush[r1][c1] == 0) return false;
        if (colMush[r2 + 1][c2] - colMush[r1][c2] == 0) return false;
        return true;
    }
    bool isValidRect(int r1, int c1, int r2, int c2) const {
        return rectSum(r1, c1, r2, c2) == SUM_TARGET && hasMushroomOnSides(r1, c1, r2, c2);
    }
    void applyMove(int r1, int c1, int r2, int c2, int player) {
        for (int r = r1; r <= r2; r++) {
            for (int c = c1; c <= c2; c++) {
                hash ^= zobristGrid[r][c][grid[r][c]];
                hash ^= zobristOwner[r][c][owner[r][c]];
                grid[r][c] = 0;
                owner[r][c] = player;
                hash ^= zobristGrid[r][c][0];
                hash ^= zobristOwner[r][c][player];
            }
        }
        computePrefixSums();
    }
    int countCells(int player) const {
        int cnt = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (owner[r][c] == player) cnt++;
        return cnt;
    }
    int countMushrooms() const {
        int cnt = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (grid[r][c] > 0) cnt++;
        return cnt;
    }
    int evaluate(int player) const {
        int opp = (player == 1) ? 2 : 1;
        int myCells = countCells(player);
        int oppCells = countCells(opp);
        int score = (myCells - oppCells) * 100;
        int myConn = 0, oppConn = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (owner[r][c] == player) {
                    if (r > 0 && owner[r-1][c] == player) myConn++;
                    if (c > 0 && owner[r][c-1] == player) myConn++;
                } else if (owner[r][c] == opp) {
                    if (r > 0 && owner[r-1][c] == opp) oppConn++;
                    if (c > 0 && owner[r][c-1] == opp) oppConn++;
                }
            }
        }
        score += (myConn - oppConn) * 10;
        int myAdjMush = 0, oppAdjMush = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (grid[r][c] > 0) {
                    bool adjMe = false, adjOpp = false;
                    if (r > 0) { if (owner[r-1][c] == player) adjMe = true; else if (owner[r-1][c] == opp) adjOpp = true; }
                    if (r < ROWS-1) { if (owner[r+1][c] == player) adjMe = true; else if (owner[r+1][c] == opp) adjOpp = true; }
                    if (c > 0) { if (owner[r][c-1] == player) adjMe = true; else if (owner[r][c-1] == opp) adjOpp = true; }
                    if (c < COLS-1) { if (owner[r][c+1] == player) adjMe = true; else if (owner[r][c+1] == opp) adjOpp = true; }
                    if (adjMe) myAdjMush++;
                    if (adjOpp) oppAdjMush++;
                }
            }
        }
        score += (myAdjMush - oppAdjMush) * 8;
        return score;
    }
    vector<Rect> findValidRects() const {
        vector<Rect> result;
        int colSum[COLS];
        for (int r1 = 0; r1 < ROWS; r1++) {
            for (int c = 0; c < COLS; c++)
                colSum[c] = grid[r1][c];
            for (int r2 = r1; r2 < ROWS; r2++) {
                if (r2 > r1) {
                    for (int c = 0; c < COLS; c++)
                        colSum[c] += grid[r2][c];
                }
                if (prefix[r2 + 1][COLS] - prefix[r1][COLS] < SUM_TARGET)
                    continue;
                for (int c1 = 0; c1 < COLS; c1++) {
                    if (rectSum(r1, c1, r2, COLS - 1) < SUM_TARGET)
                        break;
                    int sum = 0;
                    for (int c2 = c1; c2 < COLS; c2++) {
                        sum += colSum[c2];
                        if (sum > SUM_TARGET) break;
                        if (sum == SUM_TARGET) {
                            if (hasMushroomOnSides(r1, c1, r2, c2))
                                result.push_back({r1, c1, r2, c2});
                        }
                    }
                }
            }
        }
        return result;
    }
};
struct TTEntry {
    uint64_t hash;
    int8_t depth;
    int16_t score;
    int8_t flag;
    Rect bestMove;
    bool valid;
};
class TranspositionTable {
public:
    TTEntry entries[TT_SIZE];
    void clear() { memset(entries, 0, sizeof(entries)); }
    TTEntry* get(uint64_t h) { return &entries[h & (TT_SIZE - 1)]; }
    void store(uint64_t h, int depth, int score, int flag, const Rect& bm) {
        int idx = h & (TT_SIZE - 1);
        TTEntry& e = entries[idx];
        e.hash = h;
        e.depth = (int8_t)depth;
        e.score = (int16_t)score;
        e.flag = (int8_t)flag;
        e.bestMove = bm;
        e.valid = true;
    }
};
static TranspositionTable global_tt;
struct Solver {
    int myPlayer;
    int oppPlayer;
    steady_clock::time_point turnStart;
    int64_t remainingTime;
    int64_t nodeCount;
    TranspositionTable* tt;
    Rect killerMoves[MAX_PLY][2];
    bool aborted;

    Solver(int player) : myPlayer(player), oppPlayer((player == 1) ? 2 : 1) {
        tt = &global_tt;
        memset(killerMoves, 0, sizeof(killerMoves));
        aborted = false;
    }
    ~Solver() {}

    int64_t elapsedMs() const {
        return duration_cast<milliseconds>(steady_clock::now() - turnStart).count();
    }

    bool timeUp(int64_t limit) const {
        return elapsedMs() >= limit;
    }

    // Quick move heuristic: total cells + opponent cells captured + mushroom value + frontier threat
    int scoreRectQuick(const Board& b, const Rect& r, int player) const {
        int opp = (player == 1) ? 2 : 1;
        int totalCells = (r.r2 - r.r1 + 1) * (r.c2 - r.c1 + 1);
        int oppCells = 0, myLost = 0, mushSum = 0;
        for (int rr = r.r1; rr <= r.r2; rr++) {
            for (int c = r.c1; c <= r.c2; c++) {
                if (b.owner[rr][c] == opp) oppCells++;
                if (b.owner[rr][c] == player) myLost++;
                mushSum += b.grid[rr][c]; // mushrooms cleared = potential denied from opponent
            }
        }
        // Frontier threat: count unclaimed mushrooms immediately adjacent to this rect.
        // Higher = this rect anchors expansion into mushroom-rich territory.
        int frontier = 0;
        if (r.r1 > 0) {
            for (int c = r.c1; c <= r.c2; c++) {
                if (b.grid[r.r1 - 1][c] > 0) frontier++;
            }
        }
        if (r.r2 < ROWS - 1) {
            for (int c = r.c1; c <= r.c2; c++) {
                if (b.grid[r.r2 + 1][c] > 0) frontier++;
            }
        }
        if (r.c1 > 0) {
            for (int rr = r.r1; rr <= r.r2; rr++) {
                if (b.grid[rr][r.c1 - 1] > 0) frontier++;
            }
        }
        if (r.c2 < COLS - 1) {
            for (int rr = r.r1; rr <= r.r2; rr++) {
                if (b.grid[rr][r.c2 + 1] > 0) frontier++;
            }
        }
        return totalCells * 100 + oppCells * 200 - myLost * 100 + mushSum * 15 + frontier * 30;
    }

    // PVS with LMR and killer moves
    // Quiescence: at depth 0, continue searching capture moves (rects stealing opponent cells)
    int alphaBeta(Board& b, int depth, int alpha, int beta, int player, bool prevPassed,
                  int64_t timeLimit, int ply) {
        nodeCount++;
        if (aborted) return b.evaluate(player);
        if ((nodeCount & 1023) == 0 && timeUp(timeLimit)) {
            aborted = true;
            return b.evaluate(player);
        }

        uint64_t stateHash = b.hash ^ zobristPlayer[player] ^ zobristPassed[prevPassed ? 1 : 0];
        TTEntry* tte = tt->get(stateHash);
        bool ttValid = tte->valid && tte->hash == stateHash;
        bool ttHit = ttValid && tte->depth >= depth;
        if (ttHit) {
            if (tte->flag == 0) return tte->score;
            if (tte->flag == 1) {
                if (tte->score >= beta) return tte->score;
                alpha = max(alpha, (int)tte->score); // tighten alpha from lower bound
            }
            if (tte->flag == 2 && tte->score <= alpha) return tte->score;
        }

        int opp = (player == 1) ? 2 : 1;
        auto rects = b.findValidRects();

        if (prevPassed && rects.empty()) return b.evaluate(player);

        if (depth <= 0) return b.evaluate(player);

        if (rects.empty()) {
            Board next = b;
            int v = -alphaBeta(next, depth - 1, -beta, -alpha, opp, true, timeLimit, ply + 1);
            if (!aborted) tt->store(stateHash, depth, v, 0, {-1,-1,-1,-1});
            return v;
        }

        // Move ordering
        Rect ttMove = {-1,-1,-1,-1};
        if (ttHit && tte->bestMove.r1 >= 0) ttMove = tte->bestMove;

        vector<pair<int, Rect>> scored;
        scored.reserve(rects.size());
        for (auto& r : rects) {
            int s = scoreRectQuick(b, r, player);
            if (r == ttMove) s += 100000;
            if (ply < MAX_PLY) {
                if (r == killerMoves[ply][0]) s += 50000;
                else if (r == killerMoves[ply][1]) s += 25000;
            }
            scored.push_back({s, r});
        }
        stable_sort(scored.begin(), scored.end(), [](const pair<int, Rect>& a, const pair<int, Rect>& b) { return a.first > b.first; });

        // Search width: wider at shallow depths for better coverage of tactical moves.
        // depth<=2: capture everything relevant; depth<=4: broader mid-game; else: tighter but not too narrow.
        int K = (depth <= 2) ? min((int)scored.size(), 12) :
                (depth <= 4) ? min((int)scored.size(), 9) :
                (depth <= 6) ? min((int)scored.size(), 7) :
                min((int)scored.size(), 5);

        int bestScore = -INF;
        Rect bestMove = {-1,-1,-1,-1};
        int originalAlpha = alpha;
        bool isFirstMove = true;

        for (int i = 0; i < K; i++) {
            if (timeUp(timeLimit)) {
                if (bestScore == -INF) bestScore = b.evaluate(player);
                break;
            }
            auto& r = scored[i].second;
            Board next = b;
            next.applyMove(r.r1, r.c1, r.r2, r.c2, player);

            int v;
            if (isFirstMove) {
                // PV node: full window search
                v = -alphaBeta(next, depth - 1, -beta, -alpha, opp, false, timeLimit, ply + 1);
                isFirstMove = false;
            } else {
                // PVS: zero-window search with LMR for late moves.
                // LMR starts at i>=4 (was i>=3) with smaller reduction to avoid missing tactical moves.
                int lmr = 0;
                if (i >= 4) lmr = min(depth - 1, 1);
                if (i >= 7) lmr = min(depth - 1, 2);
                int sd = max(0, depth - 1 - lmr);
                v = -alphaBeta(next, sd, -alpha - 1, -alpha, opp, false, timeLimit, ply + 1);
                if (v > alpha && lmr > 0) {
                    v = -alphaBeta(next, depth - 1, -alpha - 1, -alpha, opp, false, timeLimit, ply + 1);
                }
                if (v > alpha && v < beta) {
                    v = -alphaBeta(next, depth - 1, -beta, -alpha, opp, false, timeLimit, ply + 1);
                }
            }
            if (v > bestScore) { bestScore = v; bestMove = r; }
            alpha = max(alpha, v);
            if (alpha >= beta) {
                if (ply < MAX_PLY && r.r1 >= 0 && r != killerMoves[ply][0]) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = r;
                }
                break;
            }
        }

        if (bestScore == -INF) bestScore = b.evaluate(player);
        int flag = 0;
        if (bestScore <= originalAlpha) flag = 2;
        else if (bestScore >= beta) flag = 1;
        if (!aborted) tt->store(stateHash, depth, bestScore, flag, bestMove);
        return bestScore;
    }

    Rect selectMove(Board& b, int64_t t1, bool oppPassed) {
        turnStart = steady_clock::now();
        remainingTime = t1;
        nodeCount = 0;
        aborted = false;
        tt->clear();
        memset(killerMoves, 0, sizeof(killerMoves));

        auto rects = b.findValidRects();
        if (rects.empty()) return {-1, -1, -1, -1};

        // Time budget: dynamic based on remaining time and game state
        int totalMushSum = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                totalMushSum += b.grid[r][c];
        int estMovesLeft = max(1, totalMushSum / 18);
        int64_t budget = (remainingTime - SAFETY_BUFFER_MS) / estMovesLeft;
        budget = max(budget, (int64_t)50);
        int64_t maxCap = min((int64_t)1200, remainingTime / 8);
        budget = min(budget, maxCap);

        // Score and sort moves (do NOT include pass in root list)
        vector<pair<int, Rect>> scored;
        for (auto& r : rects) {
            Board next = b;
            next.applyMove(r.r1, r.c1, r.r2, r.c2, myPlayer);
            int s = next.evaluate(myPlayer);
            scored.push_back({s, r});
        }
        stable_sort(scored.begin(), scored.end(), [](const pair<int, Rect>& a, const pair<int, Rect>& b) { return a.first > b.first; });

        Rect bestMove = scored[0].second;

        // Endgame depth extension
        int maxDepth = 12;
        if ((int)rects.size() <= 2) maxDepth = 18;
        else if ((int)rects.size() <= 5) maxDepth = 14;
        else if ((int)rects.size() <= 8) maxDepth = 12;

        // Iterative deepening
        int64_t lastDepthTime = 0;
        for (int depth = 2; depth <= maxDepth; depth += 2) {
            int64_t elapsed = elapsedMs();
            if (elapsed >= budget) break;
            int64_t depthBudget = budget - elapsed;
            if (depthBudget < 20) break;

            // Time prediction: stop if next depth would take >3x previous depth time
            if (depth > 2 && lastDepthTime > 0 && depthBudget < lastDepthTime * 3) {
                break;
            }

            int bestScore = -INF;
            Rect depthBest = {-1,-1,-1,-1};
            // Root K: use 12 for all depths to prevent pruning the best move
            int K = min((int)scored.size(), 12);
            bool completed = true;

            int64_t depthStart = elapsedMs();
            for (int i = 0; i < K; i++) {
                if (elapsedMs() >= budget) {
                    completed = false;
                    break;
                }
                auto& r = scored[i].second;
                Board next = b;
                next.applyMove(r.r1, r.c1, r.r2, r.c2, myPlayer);
                int v = -alphaBeta(next, depth - 1, -INF, INF, oppPlayer, false, budget, 0);
                if (v > bestScore) { bestScore = v; depthBest = r; }
            }
            if (completed && depthBest.r1 >= 0) {
                bestMove = depthBest;
                lastDepthTime = elapsedMs() - depthStart;
            } else {
                break;
            }
        }

        // Strategic pass check: after ID, test if passing is better than our best move
        {
            if (oppPassed) {
                int myCells = b.countCells(myPlayer);
                int oppCells = b.countCells(oppPlayer);
                if (myCells > oppCells) {
                    return {-1, -1, -1, -1};
                }
            } else {
                int64_t currentElapsed = elapsedMs();
                int64_t passBudget = min((int64_t)50, budget / 3);
                int64_t passLimit = currentElapsed + passBudget;
                Board passBoard = b;
                aborted = false;
                int passScore = -alphaBeta(passBoard, 3, -INF, INF, oppPlayer, true, passLimit, 0);
                Board moveBoard = b;
                moveBoard.applyMove(bestMove.r1, bestMove.c1, bestMove.r2, bestMove.c2, myPlayer);
                aborted = false;
                int moveScore = -alphaBeta(moveBoard, 3, -INF, INF, oppPlayer, false, passLimit, 0);
                if (passScore > moveScore + 100) {
                    return {-1, -1, -1, -1};
                }
            }
        }

        return bestMove;
    }
};
int main() {
    initZobrist();
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    cout << unitbuf;
    cerr << VERSION_STR << endl;
    string line;
    int player = 0;
    Board board;
    bool initialized = false;
    bool oppPassed = false;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        if (cmd == "READY") {
            string pos;
            iss >> pos;
            player = (pos == "FIRST") ? 1 : 2;
            cout << "OK" << '\n';
        } else if (cmd == "INIT") {
            for (int r = 0; r < ROWS; r++) {
                string row;
                if (!(iss >> row)) break;
                if (row.length() != COLS) break;
                for (int c = 0; c < COLS; c++) {
                    board.grid[r][c] = row[c] - '0';
                    board.owner[r][c] = 0;
                }
            }
            board.computePrefixSums();
            board.recomputeHash();
            initialized = true;
            oppPassed = false;
            global_tt.clear();
        } else if (cmd == "TIME") {
            int64_t t1, t2;
            iss >> t1 >> t2;
            if (!initialized) {
                cout << "-1 -1 -1 -1" << '\n';
                continue;
            }
            Solver solver(player);
            Rect move = solver.selectMove(board, t1, oppPassed);
            cout << move.r1 << ' ' << move.c1 << ' ' << move.r2 << ' ' << move.c2 << '\n';
            if (move.r1 >= 0) board.applyMove(move.r1, move.c1, move.r2, move.c2, player);
        } else if (cmd == "OPP") {
            int x1, y1, x2, y2, t;
            iss >> x1 >> y1 >> x2 >> y2 >> t;
            if (x1 >= 0) {
                board.applyMove(x1, y1, x2, y2, (player == 1) ? 2 : 1);
                oppPassed = false;
            } else {
                oppPassed = true;
            }
        } else if (cmd == "FINISH") {
            break;
        }
    }
    return 0;
}
