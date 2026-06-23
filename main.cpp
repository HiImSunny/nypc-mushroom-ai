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
#define VERSION_STR "mushroom_ai_v35_20260624"
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
const int SAFETY_BUFFER_MS = 150;
const int TT_SIZE = 1 << 16;
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
static uint64_t zobristGrid[ROWS][COLS][10];
static uint64_t zobristOwner[ROWS][COLS][3];
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
struct Solver {
    int myPlayer;
    int oppPlayer;
    steady_clock::time_point turnStart;
    int64_t remainingTime;
    TranspositionTable* tt;
    Rect killerMoves[MAX_PLY][2];

    Solver(int player) : myPlayer(player), oppPlayer((player == 1) ? 2 : 1) {
        tt = new TranspositionTable();
        memset(killerMoves, 0, sizeof(killerMoves));
    }
    ~Solver() { delete tt; }

    int64_t elapsedMs() const {
        return duration_cast<milliseconds>(steady_clock::now() - turnStart).count();
    }

    bool timeUp(int64_t limit) const {
        return elapsedMs() >= limit;
    }

    // Quick move heuristic: mushrooms + opponent cells captured
    int scoreRectQuick(const Board& b, const Rect& r, int player) const {
        int opp = (player == 1) ? 2 : 1;
        int mushrooms = 0, oppCells = 0, myLost = 0;
        for (int rr = r.r1; rr <= r.r2; rr++) {
            for (int c = r.c1; c <= r.c2; c++) {
                if (b.grid[rr][c] > 0) mushrooms++;
                if (b.owner[rr][c] == opp) oppCells++;
                if (b.owner[rr][c] == player) myLost++;
            }
        }
        return mushrooms * 100 + oppCells * 200 - myLost * 50;
    }

    // PVS with LMR and killer moves
    int alphaBeta(Board& b, int depth, int alpha, int beta, int player, bool prevPassed,
                  int64_t timeLimit, int ply) {
        if ((ply & 7) == 0 && timeUp(timeLimit)) return b.evaluate(player);

        TTEntry* tte = tt->get(b.hash);
        bool ttHit = tte->valid && tte->hash == b.hash && tte->depth >= depth;
        if (ttHit) {
            if (tte->flag == 0) return tte->score;
            if (tte->flag == 1 && tte->score >= beta) return tte->score;
            if (tte->flag == 2 && tte->score <= alpha) return tte->score;
        }

        int opp = (player == 1) ? 2 : 1;
        auto rects = b.findValidRects();

        if (prevPassed && rects.empty()) return b.evaluate(player);
        if (depth <= 0) return b.evaluate(player);

        if (rects.empty()) {
            Board next = b;
            int v = -alphaBeta(next, depth - 1, -beta, -alpha, opp, true, timeLimit, ply + 1);
            tt->store(b.hash, depth, v, 0, {-1,-1,-1,-1});
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
        sort(scored.begin(), scored.end(), [](const pair<int, Rect>& a, const pair<int, Rect>& b) { return a.first > b.first; });

        int K = (depth <= 2) ? min((int)scored.size(), 12) :
                (depth <= 4) ? min((int)scored.size(), 7) :
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
                // PVS: zero-window search
                int lmr = 0;
                if (i >= 3) lmr = min(depth - 1, 2);
                if (i >= 6) lmr = min(depth - 1, 3);
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
        tt->store(b.hash, depth, bestScore, flag, bestMove);
        return bestScore;
    }

    Rect selectMove(Board& b, int64_t t1) {
        turnStart = steady_clock::now();
        remainingTime = t1;
        tt->clear();
        memset(killerMoves, 0, sizeof(killerMoves));

        auto rects = b.findValidRects();
        if (rects.empty()) return {-1, -1, -1, -1};

        // Time budget: dynamic based on remaining time and game state
        int mushLeft = b.countMushrooms();
        int estMovesLeft = max(2, mushLeft / 5 + (int)rects.size() / 4);
        int64_t budget = (remainingTime - SAFETY_BUFFER_MS) / max(estMovesLeft, 1);
        budget = max(budget, (int64_t)50);
        budget = min(budget, (int64_t)700);

        // Score and sort moves (do NOT include pass in root list)
        vector<pair<int, Rect>> scored;
        for (auto& r : rects) {
            Board next = b;
            next.applyMove(r.r1, r.c1, r.r2, r.c2, myPlayer);
            int s = next.evaluate(myPlayer);
            scored.push_back({s, r});
        }
        sort(scored.begin(), scored.end(), [](const pair<int, Rect>& a, const pair<int, Rect>& b) { return a.first > b.first; });

        Rect bestMove = scored[0].second;

        // Endgame depth extension
        int maxDepth = 8;
        if ((int)rects.size() <= 2) maxDepth = 14;
        else if ((int)rects.size() <= 6) maxDepth = 12;

        // Iterative deepening
        for (int depth = 2; depth <= maxDepth; depth += 2) {
            int64_t elapsed = elapsedMs();
            if (elapsed >= budget) break;
            int64_t depthBudget = budget - elapsed;
            if (depthBudget < 20) break;

            int bestScore = -INF;
            Rect depthBest = {-1,-1,-1,-1};
            int K = min((int)scored.size(), 12);

            for (int i = 0; i < K; i++) {
                if (elapsedMs() >= depthBudget) break;
                auto& r = scored[i].second;
                Board next = b;
                next.applyMove(r.r1, r.c1, r.r2, r.c2, myPlayer);
                int v = -alphaBeta(next, depth - 1, -INF, INF, oppPlayer, false, depthBudget, 0);
                if (v > bestScore) { bestScore = v; depthBest = r; }
            }
            if (depthBest.r1 >= 0) bestMove = depthBest;
        }

        // Strategic pass check: after ID, test if passing is better than our best move
        // Only pass if we genuinely have nothing useful to do
        {
            int64_t passBudget = min((int64_t)50, budget / 3);
            Board passBoard = b;
            int passScore = -alphaBeta(passBoard, 3, -INF, INF, oppPlayer, true, passBudget, 0);
            Board moveBoard = b;
            moveBoard.applyMove(bestMove.r1, bestMove.c1, bestMove.r2, bestMove.c2, myPlayer);
            int moveScore = -alphaBeta(moveBoard, 3, -INF, INF, oppPlayer, false, passBudget, 0);
            // Only pass if it's clearly better (allows opponent to walk into zugzwang)
            if (passScore > moveScore + 100) {
                return {-1, -1, -1, -1};
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
        } else if (cmd == "TIME") {
            int64_t t1, t2;
            iss >> t1 >> t2;
            if (!initialized) {
                cout << "-1 -1 -1 -1" << '\n';
                continue;
            }
            Solver solver(player);
            Rect move = solver.selectMove(board, t1);
            cout << move.r1 << ' ' << move.c1 << ' ' << move.r2 << ' ' << move.c2 << '\n';
            if (move.r1 >= 0) board.applyMove(move.r1, move.c1, move.r2, move.c2, player);
        } else if (cmd == "OPP") {
            int x1, y1, x2, y2, t;
            iss >> x1 >> y1 >> x2 >> y2 >> t;
            if (x1 >= 0) board.applyMove(x1, y1, x2, y2, (player == 1) ? 2 : 1);
        } else if (cmd == "FINISH") {
            break;
        }
    }
    return 0;
}
