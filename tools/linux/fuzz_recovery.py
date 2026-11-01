#!/usr/bin/env python3
"""Breaks real programs in random ways and checks that 'saad index' still writes an index, without crashing or hanging.

    tools/linux/fuzz_recovery.py [count] [seed]

Each case takes a file of a program, makes one mistake in it (deletes a '}', ';' or ')', renames a name, cuts the file
off, or puts junk in it), and gives it to the compiler with --overlay, so no file is changed. A case fails when the
compiler crashes, takes too long, or doesn't write JSON with the diagnostics.
"""
import json, os, random, re, subprocess, sys, tempfile

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SAAD = os.path.join(REPO, "bin", "saad")

# (main file, files of it to break)
PROGRAMS = [
    ("tests/index.sd", ["tests/index.sd"]),
    ("tests/switch.sd", ["tests/switch.sd"]),
    ("tests/unions.sd", ["tests/unions.sd"]),
    ("tests/generic_structs.sd", ["tests/generic_structs.sd"]),
    ("tests/std/json.sd", ["tests/std/json.sd", "std/json/decode.sd", "std/json/encode.sd"]),
    ("tests/namespaces.sd", ["tests/namespaces.sd", "tests/namespaces/geometry/shapes.sd"]),
    ("examples/breakout/breakout.sd", ["examples/breakout/breakout.sd"]),
    ("lsp/main.sd", ["lsp/main.sd", "lsp/project.sd", "lsp/source.sd", "lsp/rpc.sd"]),
    ("tests/std/list.sd", ["std/prelude/collections.sd", "std/prelude/string.sd"]),
]

def mutate(text, rng):
    kind = rng.choice(["delete", "delete", "rename", "truncate", "junk", "duplicate", "swap"])
    if kind == "delete":
        spots = [m.start() for m in re.finditer(r"[{};()\[\]:=,]", text)]
        at = rng.choice(spots)
        return kind, text[:at] + text[at + 1:]
    if kind == "rename":
        names = [m for m in re.finditer(r"\b[A-Za-z_][A-Za-z0-9_]*\b", text)]
        m = rng.choice(names)
        return kind, text[:m.start()] + rng.choice(["undefinedName", "x", "Point", "int", "struct", "return", "1"]) + text[m.end():]
    if kind == "truncate":
        return kind, text[:rng.randrange(len(text))]
    if kind == "junk":
        at = rng.randrange(len(text))
        return kind, text[:at] + rng.choice(["@@", " ?? ", "{", "}", "(", ";;", " :: ", " ø ", "\"", "/*", " .. ", " => "]) + text[at:]
    if kind == "duplicate":
        lines = text.split("\n")
        i = rng.randrange(len(lines))
        lines.insert(i, lines[rng.randrange(len(lines))])
        return kind, "\n".join(lines)
    lines = text.split("\n")
    i, j = rng.randrange(len(lines)), rng.randrange(len(lines))
    lines[i], lines[j] = lines[j], lines[i]
    return "swap", "\n".join(lines)

def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 300
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    rng = random.Random(seed)
    work = tempfile.mkdtemp()
    failures = 0
    for case in range(count):
        main_file, files = rng.choice(PROGRAMS)
        target = rng.choice(files)
        text = open(os.path.join(REPO, target)).read()
        kind, broken = mutate(text, rng)
        overlay = os.path.join(work, f"case{case}.sd")
        open(overlay, "w").write(broken)

        command = [SAAD, "index", os.path.join(REPO, main_file), "--overlay", f"{os.path.join(REPO, target)}={overlay}"]
        try:
            result = subprocess.run(command, capture_output=True, timeout=20, cwd=REPO)
            problem = None
            if result.returncode not in (0, 1):
                problem = f"exit code {result.returncode}"
            else:
                try:
                    index = json.loads(result.stdout)
                    if "diagnostics" not in index:
                        problem = "no diagnostics"
                except ValueError as e:
                    problem = f"not JSON: {e}"
        except subprocess.TimeoutExpired:
            problem = "timeout"

        if problem:
            failures += 1
            kept = os.path.join(work, f"failed{case}.sd")
            os.rename(overlay, kept)
            print(f"case {case}: {problem} ({kind} in {target}): {' '.join(command[:3])} --overlay {target}={kept}")
        else:
            os.remove(overlay)

    print(f"{count - failures} of {count} cases passed")
    sys.exit(1 if failures else 0)

if __name__ == "__main__":
    main()
