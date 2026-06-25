import sys
import os

ROWS = 10
COLS = 17

class Board:
    def __init__(self, init_line):
        parts = init_line.strip().split()
        assert parts[0] == "INIT"
        self.grid = []
        for row_str in parts[1:]:
            self.grid.append([int(c) for c in row_str])
        assert len(self.grid) == ROWS and all(len(r) == COLS for r in self.grid)
        self.owner = [[0] * COLS for _ in range(ROWS)]

    def apply_move(self, r1, c1, r2, c2, player):
        if r1 == -1:
            return
        for r in range(r1, r2 + 1):
            for c in range(c1, c2 + 1):
                self.grid[r][c] = 0
                self.owner[r][c] = player

    def print_board(self):
        print("Board Grid:")
        for r in range(ROWS):
            print(" ".join(f"{self.grid[r][c]}" for c in range(COLS)))
        print("\nBoard Owners:")
        for r in range(ROWS):
            print(" ".join(f"{self.owner[r][c]}" for c in range(COLS)))
        print(f"Cells: P1={self.count_cells(1)}, P2={self.count_cells(2)}")
        print(f"Mushrooms: {self.count_mushrooms()}")
        print(f"Evaluation (P1): {self.evaluate(1)}")
        print(f"Evaluation (P2): {self.evaluate(2)}")

    def count_cells(self, player):
        return sum(1 for r in range(ROWS) for c in range(COLS) if self.owner[r][c] == player)

    def count_mushrooms(self):
        return sum(1 for r in range(ROWS) for c in range(COLS) if self.grid[r][c] > 0)

    def evaluate(self, player):
        opp = 2 if player == 1 else 1
        my_cells = self.count_cells(player)
        opp_cells = self.count_cells(opp)
        score = (my_cells - opp_cells) * 100
        
        my_conn = 0
        opp_conn = 0
        for r in range(ROWS):
            for c in range(COLS):
                if self.owner[r][c] == player:
                    if r > 0 and self.owner[r-1][c] == player: my_conn += 1
                    if c > 0 and self.owner[r][c-1] == player: my_conn += 1
                elif self.owner[r][c] == opp:
                    if r > 0 and self.owner[r-1][c] == opp: opp_conn += 1
                    if c > 0 and self.owner[r][c-1] == opp: opp_conn += 1
        score += (my_conn - opp_conn) * 10

        my_adj = 0
        opp_adj = 0
        for r in range(ROWS):
            for c in range(COLS):
                if self.grid[r][c] > 0:
                    adj_me = False
                    adj_opp = False
                    for dr, dc in [(-1,0), (1,0), (0,-1), (0,1)]:
                        nr, nc = r + dr, c + dc
                        if 0 <= nr < ROWS and 0 <= nc < COLS:
                            if self.owner[nr][nc] == player: adj_me = True
                            elif self.owner[nr][nc] == opp: adj_opp = True
                    if adj_me: my_adj += 1
                    if adj_opp: opp_adj += 1
        score += (my_adj - opp_adj) * 8
        return score

    def rect_sum(self, r1, c1, r2, c2):
        s = 0
        for r in range(r1, r2 + 1):
            for c in range(c1, c2 + 1):
                s += self.grid[r][c]
        return s

    def has_mushroom_on_sides(self, r1, c1, r2, c2):
        # top row
        if not any(self.grid[r1][c] > 0 for c in range(c1, c2+1)): return False
        # bottom row
        if not any(self.grid[r2][c] > 0 for c in range(c1, c2+1)): return False
        # left col
        if not any(self.grid[r][c1] > 0 for r in range(r1, r2+1)): return False
        # right col
        if not any(self.grid[r][c2] > 0 for r in range(r1, r2+1)): return False
        return True

    def find_valid_rects(self):
        rects = []
        for r1 in range(ROWS):
            for r2 in range(r1, ROWS):
                for c1 in range(COLS):
                    for c2 in range(c1, COLS):
                        if self.rect_sum(r1, c1, r2, c2) == 10:
                            if self.has_mushroom_on_sides(r1, c1, r2, c2):
                                rects.append((r1, c1, r2, c2))
        return rects

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_log.py <log_file> [target_line_number]")
        return
    
    log_file = sys.argv[1]
    target_line = int(sys.argv[2]) if len(sys.argv) > 2 else 999999

    with open(log_file) as f:
        lines = [line.strip() for line in f if line.strip()]

    board = Board(lines[0])
    print(f"Initial board from {log_file}")
    
    # Process moves
    # Lines are 1-indexed, line 1 is INIT
    current_line = 1
    for line in lines[1:]:
        current_line += 1
        if current_line > target_line:
            break
        if line.startswith("FIRST") or line.startswith("SECOND"):
            parts = line.split()
            player = 1 if parts[0] == "FIRST" else 2
            r1, c1, r2, c2 = int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])
            print(f"Line {current_line}: {parts[0]} plays ({r1},{c1})-({r2},{c2})")
            board.apply_move(r1, c1, r2, c2, player)
        elif line.startswith("FINISH"):
            break

    print(f"\nState after processing up to line {min(current_line, len(lines))}:")
    board.print_board()

    valid_rects = board.find_valid_rects()
    print(f"\nValid Rects count: {len(valid_rects)}")
    for r in valid_rects:
        # score quick
        total_cells = (r[2] - r[0] + 1) * (r[3] - r[1] + 1)
        print(f"  {r}: size={total_cells}")

if __name__ == "__main__":
    main()
