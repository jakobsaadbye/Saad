"""Reads what 'saad index' writes (version 2, see docs/lsp.md) into the objects of version 1, which are easier to check:
a symbol is {"id", "name", "kind", "file", "nameRange", "range", "detail", "container", "doc"} and a reference is
{"file", "range", "symbol"}, with each range {"start": {"line", "character"}, "end": {...}}"""
import json

def _range(numbers):
    return {"start": {"line": numbers[0], "character": numbers[1]}, "end": {"line": numbers[2], "character": numbers[3]}}

def load(text):
    d = json.loads(text)
    if d.get("version") != 2:
        raise ValueError(f"version {d.get('version')} of the index, not 2")
    symbols = []
    for i, s in enumerate(d["symbols"]):
        symbol = {"id": i, "name": s[0], "kind": s[1], "file": s[2], "nameRange": _range(s[3:7]), "range": _range(s[7:11]),
                  "detail": s[11], "container": s[12]}
        if len(s) > 13:
            symbol["doc"] = s[13]
        symbols.append(symbol)
    r = d["references"]
    references = [{"file": r[i], "range": _range(r[i + 1:i + 5]), "symbol": r[i + 5]} for i in range(0, len(r), 6)]
    return dict(d, symbols=symbols, references=references)
