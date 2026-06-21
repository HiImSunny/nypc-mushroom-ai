import os
f = 'D:\\Code Project\\Python Coding\\NYPC Hackathon\\main.cpp'
with open(f, 'rb') as fh:
    data = fh.read()

# 1. Add mobility parameter to evaluate signature
data = data.replace(b'int evaluate(int player) const {', b'int evaluate(int player, int mobility = -1) const {')

# 2. Add mobility bonus before return score
data = data.replace(b'score += (myAdjMush - oppAdjMush) * 1;\r\n\r\n\r\n        return score;', b'score += (myAdjMush - oppAdjMush) * 1;\r\n\r\n        // Mobility: more valid rects = more future potential\r\n        if (mobility >= 0) score += mobility;\r\n\r\n        return score;')

# 3. Pass mobility from alphaBeta depth-0
data = data.replace(b'if (depth == 0) {\r\n            return b.evaluate(player);\r\n        }', b'if (depth == 0) {\r\n            return b.evaluate(player, (int)rects.size());\r\n        }')

# 4. Update version
data = data.replace(b'mushroom_ai_v17_20260620', b'mushroom_ai_v18_20260620')

with open(f, 'wb') as fh:
    fh.write(data)
print('v18: mobility term in evaluate')
