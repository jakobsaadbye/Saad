#!/usr/bin/env python3
# Checks the JSON of 'saad index' (on stdin) for a file:
#   * every declaration spells its name where the index says it is
#   * every identifier in the file is a declaration or a use of one, except the ones that have no declaration in Saad
# Usage: bin/saad index tests/index.sd | tools/linux/check_index.py tests/index.sd
import json, re, sys

NO_DECLARATION = {'self', 'kind', 'count', 'data', 'it', '_', 'assert', 'typeof', 'sizeof'}
KEYWORDS = set('if else for while return break continue struct enum union method switch case in as use import namespace '
               'null true false int i8 i16 i32 i64 u8 u16 u32 u64 uint f32 f64 float bool string void any char xx'.split())

d = json.load(sys.stdin)
target = sys.argv[1]
files = d['files']
matches = [i for i, f in enumerate(files) if f.endswith(target)]
if not matches:
    print('the file is not in the index'); sys.exit(1)
fid = matches[0]
texts = {}
def lines(i):
    if i not in texts: texts[i] = open(files[i], encoding='utf-8').read().split('\n')
    return texts[i]

problems = []
for s in d['symbols']:
    st, en = s['nameRange']['start'], s['nameRange']['end']
    if s['kind'] == 'namespace': continue
    text = lines(s['file'])[st['line']][st['character']:en['character']]
    if text != s['name']: problems.append(f"declaration {s['name']} spells {text!r}")

covered = set()
for r in d['references'] + [{'file': s['file'], 'range': s['nameRange']} for s in d['symbols']]:
    if r['file'] != fid: continue
    st, en = r['range']['start'], r['range']['end']
    if st['line'] == en['line']:
        covered.update((st['line'], c) for c in range(st['character'], en['character']))

for number, line in enumerate(lines(fid)):
    code = re.sub(r'"(\\.|[^"\\])*"', lambda m: ' ' * len(m.group(0)), line).split('//')[0]
    for m in re.finditer(r'[A-Za-z_][A-Za-z0-9_]*', code):
        name = m.group(0)
        if name in KEYWORDS or name in NO_DECLARATION: continue
        if (number, m.start()) not in covered: problems.append(f"{target}:{number + 1}: '{name}' is not in the index")

for p in problems: print(p)
sys.exit(1 if problems else 0)
