#!/usr/bin/env python3
"""Checks that the compiler goes on after a mistake: it reports the mistake (and not many errors that come from it), and
the index still has the names after it. Each case makes one mistake in tests/index.sd, given with --overlay.

    tools/linux/check_recovery.py
"""
import json, os, subprocess, sys, tempfile

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SAAD = os.path.join(REPO, "bin", "saad")
FIXTURE = os.path.join(REPO, "tests", "index.sd")

# (what, the text to replace, its replacement, the most errors, (line, name) uses that must still be in the index)
CASES = [
    ("a syntax error in a statement",
     "    c.grow(by: 1.0);", "    c.grow(by: 1.0;", 1,
     [(53, "Shape"), (60, "swapped"), (70, "drawnName"), (44, "area")]),
    ("an unknown name in a statement",
     "    status := Status.open;", "    status := Status.opne;", 2,
     [(56, "Limit"), (60, "swapped"), (62, "Origin"), (70, "drawnName")]),
    ("an undeclared variable",
     "    total += n;", "    total += m;", 1,
     [(59, "Pair"), (60, "swapped"), (65, "total")]),
    ("a missing '}' of a function",
     "    c.radius += by;\n}", "    c.radius += by;\n", 1,
     [(43, "s"), (52, "grow"), (60, "swapped")]),
    ("a type error in a function above main",
     "        case .square => v * v;", "        case .square => v * \"text\";", 1,
     [(52, "grow"), (60, "swapped"), (70, "drawnName")]),
    ("a broken declaration at the top",
     "Limit :: 10;", "Limit :: ;", 1,
     [(60, "swapped"), (70, "drawnName"), (52, "grow")]),
    ("a struct member with an unknown type",
     "    radius: f32 = 1.0;", "    radius: Flaot = 1.0;", 3,
     [(60, "swapped"), (54, "Status"), (70, "drawnName")]),
    ("a junk character",
     "    q := swapped(p);", "    q := swapped(p); §", 1,
     [(60, "swapped"), (70, "drawnName")]),
]

def main():
    text = open(FIXTURE).read()
    work = tempfile.mkdtemp()
    failed = 0
    for what, old, new, most_errors, uses in CASES:
        assert old in text, old
        overlay = os.path.join(work, "case.sd")
        open(overlay, "w").write(text.replace(old, new, 1))
        result = subprocess.run([SAAD, "index", FIXTURE, "--overlay", f"{FIXTURE}={overlay}"], capture_output=True, text=True, cwd=REPO)
        problems = []
        try:
            index = json.loads(result.stdout)
        except ValueError:
            print(f"FAILED {what}: no index (exit code {result.returncode})\n{result.stderr[-500:]}")
            failed += 1
            continue

        errors = [d for d in index["diagnostics"] if d["severity"] == 1]
        if not errors:
            problems.append("no error")
        if len(errors) > most_errors:
            problems.append(f"{len(errors)} errors, wanted at most {most_errors}: " + "; ".join(
                f"{d['range']['start']['line'] + 1}: {d['message'].splitlines()[0]}" for d in errors))

        file = index["files"].index(FIXTURE) if FIXTURE in index["files"] else -1
        symbols = index["symbols"]
        for line, name in uses:
            found = any(r["file"] == file and r["range"]["start"]["line"] == line - 1 and symbols[r["symbol"]]["name"] == name
                        for r in index["references"])
            if not found:
                problems.append(f"the use of '{name}' on line {line} is not in the index")

        if problems:
            failed += 1
            print(f"FAILED {what}:\n    " + "\n    ".join(problems))
        else:
            print(f"ok     {what}: {len(errors)} error(s)")

    print(f"\n{len(CASES) - failed} of {len(CASES)} cases passed")
    sys.exit(1 if failed else 0)

if __name__ == "__main__":
    main()
