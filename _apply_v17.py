import os
f = 'D:\\Code Project\\Python Coding\\NYPC Hackathon\\main.cpp'
with open(f, 'rb') as fh:
    data = fh.read()
data = data.replace(b'int posBonus = max(0, 30 - distFromCenter * 2);', b'int posBonus = max(0, 12 - distFromCenter);')
old = b'            if (!initialized) {'
new = b'            if (t1 < 100 || t2 < 0) t1 = 10000;\r\n            if (!initialized) {'
data = data.replace(old, new)
data = data.replace(b'mushroom_ai_v16_20260620', b'mushroom_ai_v17_20260620')
with open(f, 'wb') as fh:
    fh.write(data)
print('v17 applied')
