// ============================================================
// Change Log � moi lan update code deu ghi lai o day
// ============================================================
// Session 3 � 2026-06-20
//   Fix: AI pass som du con nuoc di hop le
//   - selectMove(root): Xoa block "Try passing". Neu con rect, luon danh.
//   - alphaBeta: Chuyen "Try passing" xuong sau "Try moves".
//   Result: 5W 0L self-play, 72-44 cells, max 355ms (truoc: 2W 1L, 753ms)
// Session 6 � 2026-06-20
//   Optimize findValidRects (incremental colSum + early skip) + merge game-over check
//   Result: self-play 5W 0L (72-44, 47t, 425ms), battle 9W 5L (64%)
// ============================================================
#define VERSION_STR "mushroom_ai_v6_final"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <string>
#include <unordered_map>
#include <random>
#include <cstdlib>
using namespace std;
using namespace chrono;

// ============================================================
// Constants
// ============================================================
const int ROWS = 10;
const int COLS = 17;
const int SUM_TARGET = 10;
const int SAFETY_BUFFER_MS = 300;
const int TT_SIZE = 1 << 13;  // 8K entries (~256KB, safe on Windows stack)

const int INF = 1e9;

// ============================================================
// Types
// ============================================================
struct Rect {
    int r1, c1, r2, c2;
    bool operator==(const Rect& o) const {
        return r1==o.r1 && c1==o.c1 && r2==o.r2 && c2==o.c2;
    }
};
// ============================================================
// Zobrist hashing
// ============================================================
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

// ============================================================
// Board state
// ============================================================
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

    // Evaluate: territory + connectedness + position + mobility
    int evaluate(int player) const {
        int opp = (player == 1) ? 2 : 1;
        int myCells = countCells(player);
        int oppCells = countCells(opp);

        // Core: territory advantage (weighted by game phase)
        int score = (myCells - oppCells) * 12;

        // Connected territory bonus: count edge-adjacent owned pairs
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
        score += (myConn - oppConn) * 4;

        // Center proximity bonus
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (owner[r][c] != 0) {
                    int dr = min(r, ROWS-1-r);
                    int dc = min(c, COLS-1-c);
                    int central = dr + dc;
                    int bonus = max(0, 6 - central);
                    if (owner[r][c] == player) score += bonus;
                    else score -= bonus;
                }
            }
        }

        // Adjacent mushrooms: mushrooms next to our territory = expansion potential
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
        score += (myAdjMush - oppAdjMush) * 1;


        return score;
    }

    vector<Rect> findValidRects() const {
        vector<Rect> result;
        int colSum[COLS];

        for (int r1 = 0; r1 < ROWS; r1++) {
            // Initialize colSum from top row's grid values
            for (int c = 0; c < COLS; c++)
                colSum[c] = grid[r1][c];

            for (int r2 = r1; r2 < ROWS; r2++) {
                if (r2 > r1) {
                    // Incremental: add next row instead of recomputing from prefix
                    for (int c = 0; c < COLS; c++)
                        colSum[c] += grid[r2][c];
                }

                // Early skip: if total mushrooms in rows [r1..r2] < 10, no rect possible
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
// ============================================================
// Transposition Table
// ============================================================
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

    TTEntry* get(uint64_t h) {
        return &entries[h & (TT_SIZE - 1)];
    }

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

// ============================================================
// Solver
// ============================================================
struct Solver {
    int myPlayer;
    int oppPlayer;
    steady_clock::time_point turnStart;
    int64_t remainingTime;
    TranspositionTable tt;

    Solver(int player) : myPlayer(player), oppPlayer((player == 1) ? 2 : 1) {}

    bool timeUp(int64_t timeLimit) const {
        auto now = steady_clock::now();
        return duration_cast<milliseconds>(now - turnStart).count() >= timeLimit;
    }

    int scoreRect(const Board& b, const Rect& r, int player) const {
        int opp = (player == 1) ? 2 : 1;
        int oppEmpty = 0, mushrooms = 0;
        int area = (r.r2 - r.r1 + 1) * (r.c2 - r.c1 + 1);
        for (int rr = r.r1; rr <= r.r2; rr++) {
            for (int c = r.c1; c <= r.c2; c++) {
                if (b.owner[rr][c] == opp) oppEmpty++;
                if (b.grid[rr][c] > 0) mushrooms++;
            }
        }
        int swing = mushrooms + 2 * oppEmpty;
        int emptyCells = area - mushrooms;
        int efficiency = (mushrooms > 0) ? (emptyCells / mushrooms) : emptyCells;

        int centerR = (r.r1 + r.r2) / 2;
        int centerC = (r.c1 + r.c2) / 2;
        int distFromCenter = abs(centerR - 4) + abs(centerC - 8);
        int posBonus = max(0, 12 - distFromCenter);

        return swing + efficiency + posBonus;
    }

    // Negamax alpha-beta with transposition table
    int alphaBeta(Board& b, int depth, int alpha, int beta, int player, bool prevPassed, int64_t timeLimit) {
        if (timeUp(timeLimit)) {
            return b.evaluate(player);
        }

        // Check transposition table
        TTEntry* tte = tt.get(b.hash);
        bool ttHit = tte->valid && tte->hash == b.hash && tte->depth >= depth;

        if (depth == 0) {
            return b.evaluate(player);
        }

        auto rects = b.findValidRects();
        if (prevPassed && rects.empty()) return b.evaluate(player);

        int opp = (player == 1) ? 2 : 1;

        // Try TT best move first if available
        Rect ttMove = {-1,-1,-1,-1};
        if (ttHit && tte->bestMove.r1 >= 0) {
            ttMove = tte->bestMove;
        }

        // Score and sort
        vector<pair<int, Rect>> scored;
        scored.reserve(rects.size());
        for (auto& r : rects) {
            int s = scoreRect(b, r, player);
            if (r == ttMove) s += 10000;
            scored.push_back({s, r});
        }
        sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.first > b.first; });

        int K = min((int)scored.size(), 12);

        int originalAlpha = alpha;
        int bestScore = -INF;
        Rect bestMove = {-1,-1,-1,-1};

        // Try moves first - real rects preferred over passing
        for (int i = 0; i < K; i++) {
            if (timeUp(timeLimit)) return bestScore;
            auto& r = scored[i].second;
            Board next = b;
            next.applyMove(r.r1, r.c1, r.r2, r.c2, player);
            int v = -alphaBeta(next, depth - 1, -beta, -alpha, opp, false, timeLimit);
            if (v > bestScore) { bestScore = v; bestMove = r; }
            alpha = max(alpha, v);
            if (alpha >= beta) break;
        }

        // Try passing last � chi hieu qua khi opponent cung pass (double pass)
        {
            Board next = b;
            int v = -alphaBeta(next, depth - 1, -beta, -alpha, opp, true, timeLimit);
            if (v > bestScore) { bestScore = v; bestMove = {-1,-1,-1,-1}; }
            alpha = max(alpha, v);
        }

        // Store in TT with correct flag logic
        int flag = 0;
        if (bestScore <= originalAlpha) flag = 2;
        else if (bestScore >= beta) flag = 1;
        tt.store(b.hash, depth, bestScore, flag, bestMove);

        return bestScore;
    }

    Rect selectMove(Board& b, int64_t t1) {
        turnStart = steady_clock::now();
        remainingTime = t1;
        tt.clear();

        int mushLeft = b.countMushrooms();
        int estMovesLeft = max(5, mushLeft / 10);
        int64_t budget = min(remainingTime - SAFETY_BUFFER_MS, remainingTime / max(estMovesLeft, 1));
        budget = max(budget, (int64_t)100);

        auto rects = b.findValidRects();
        if (rects.empty()) return {-1, -1, -1, -1};

        // Score and sort
        vector<pair<int, Rect>> scored;
        for (auto& r : rects) scored.push_back({scoreRect(b, r, myPlayer), r});
        sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.first > b.first; });
        Rect bestMove = scored[0].second;

        // Iterative deepening (2, 4, 6)
        int maxDepth = ((int)scored.size() <= 4) ? 8 : 6;
        for (int depth = 2; depth <= maxDepth; depth += 2) {
            auto now = steady_clock::now();
            int64_t elapsed = duration_cast<milliseconds>(now - turnStart).count();
            if (elapsed >= budget * 0.5) break;
            int64_t depthBudget = budget - elapsed;
            if (depthBudget < 50) break;

            int bestScore = -INF;
            Rect depthBest = {-1, -1, -1, -1};
            int K = min((int)scored.size(), 15);

            // Khong try pass o root. Neu con rect hop le, luon danh.

            for (int i = 0; i < K; i++) {
                if (timeUp(depthBudget)) break;
                auto& r = scored[i].second;
                Board next = b;
                next.applyMove(r.r1, r.c1, r.r2, r.c2, myPlayer);
                int v = -alphaBeta(next, depth - 1, -INF, INF, oppPlayer, false, depthBudget);
                if (v > bestScore) { bestScore = v; depthBest = r; }
            }

            if (depthBest.r1 >= 0) {
                bestMove = depthBest;
            }
        }

        return bestMove;
    }
};

// ============================================================
// Main
// ============================================================
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
            if (move.r1 >= 0) {
                board.applyMove(move.r1, move.c1, move.r2, move.c2, player);
            }
        } else if (cmd == "OPP") {
            int x1, y1, x2, y2, t;
            iss >> x1 >> y1 >> x2 >> y2 >> t;
            if (x1 >= 0) {
                board.applyMove(x1, y1, x2, y2, (player == 1) ? 2 : 1);
            }
        } else if (cmd == "FINISH") {
            break;
        }
    }

    return 0;
}
