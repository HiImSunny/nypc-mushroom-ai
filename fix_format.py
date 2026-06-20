import re

with open(r'D:\Code Project\Python Coding\NYPC Hackathon\main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.lstrip('\ufeff').strip()

# Fix the comment corruption first: rejoin split comment lines  
# clang-format turned "// text// more" into "// text// more", we need to fix

# Strategy: add newlines at statement boundaries
content = re.sub(r'(#\s*(?:include|define))', r'\n\1', content)
content = re.sub(r';(?!\s*(?:\)|;|,|\s*(?:else|case|default|\/\/|\*})))', r';\n', content)
content = re.sub(r'\{', r'{\n', content)
content = re.sub(r'\}', r'\n}', content)
content = re.sub(r'(using\s+namespace)', r'\n\1', content)
content = re.sub(r'//([^\n])', lambda m: '// ' + m.group(1).lstrip(), content)
content = content.strip()

with open(r'D:\Code Project\Python Coding\NYPC Hackathon\main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

lines = content.split('\n')
print(f'Lines: {len(lines)}')
