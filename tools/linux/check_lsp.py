#!/usr/bin/env python3
"""Talks to the language server (lsp/) like an editor does, and checks its answers.

    tools/linux/check_lsp.py path/to/saad-lsp

The server is built for Linux with tools/linux/build_lsp.sh. It uses bin/saad for the index of tests/index.sd.
"""
import json, os, subprocess, sys, tempfile, threading, queue

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FIXTURE = os.path.join(REPO, "tests", "index.sd")
failures = []

def check(condition, what):
    if not condition:
        failures.append(what)
        print("  FAIL:", what)

class Client:
    def __init__(self, server):
        env = dict(os.environ, TMPDIR=tempfile.mkdtemp())
        self.process = subprocess.Popen([server, "--compiler", os.path.join(REPO, "bin", "saad")],
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        self.messages = queue.Queue()
        self.next_id = 0
        self.log = []
        threading.Thread(target=self.read, daemon=True).start()
        threading.Thread(target=self.read_log, daemon=True).start()

    def read_log(self):
        for line in self.process.stderr:
            self.log.append(line.decode(errors="replace").rstrip())

    def read(self):
        stream = self.process.stdout
        while True:
            length = None
            while True:
                line = stream.readline()
                if not line:
                    self.messages.put(None)
                    return
                line = line.strip()
                if not line:
                    break
                name, _, value = line.partition(b":")
                if name.lower() == b"content-length":
                    length = int(value)
            body = stream.read(length)
            self.messages.put(json.loads(body))

    def send(self, message):
        body = json.dumps(dict(message, jsonrpc="2.0")).encode()
        self.process.stdin.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
        self.process.stdin.flush()

    def notify(self, method, params):
        self.send({"method": method, "params": params})

    def request(self, method, params):
        self.next_id += 1
        self.send({"id": self.next_id, "method": method, "params": params})
        return self.wait(lambda m: m.get("id") == self.next_id)

    def wait(self, wanted, timeout=20):
        """The next message that is wanted. Notifications before it are kept in self.notifications"""
        while True:
            try:
                m = self.messages.get(timeout=timeout)
            except queue.Empty:
                raise RuntimeError("No answer from the server. Its log:\n" + "\n".join(self.log))
            if m is None:
                raise RuntimeError("The server ended. Its log:\n" + "\n".join(self.log))
            if wanted(m):
                return m
            self.notifications.append(m)

    notifications = []

    def diagnostics(self, uri):
        """The diagnostics the server sends next for the file"""
        for i, m in enumerate(self.notifications):
            if m.get("method") == "textDocument/publishDiagnostics" and m["params"]["uri"] == uri:
                del self.notifications[i]
                return m["params"]["diagnostics"]
        m = self.wait(lambda m: m.get("method") == "textDocument/publishDiagnostics" and m["params"]["uri"] == uri)
        return m["params"]["diagnostics"]

def uri_of(path):
    return "file://" + path

def position_of(text, line, needle, nth=0):
    """The position of the needle in the line (counting from 1), in UTF-16 units like an editor"""
    content = text.split("\n")[line - 1]
    at = -1
    for _ in range(nth + 1):
        at = content.index(needle, at + 1)
    return {"line": line - 1, "character": len(content[:at].encode("utf-16-le")) // 2}

def main():
    server = sys.argv[1]
    client = Client(server)
    text = open(FIXTURE).read()
    uri = uri_of(FIXTURE)

    print("initialize")
    result = client.request("initialize", {"processId": None, "rootUri": uri_of(os.path.dirname(FIXTURE)), "capabilities": {}})["result"]
    caps = result["capabilities"]
    for name in ["definitionProvider", "referencesProvider", "hoverProvider", "documentSymbolProvider", "workspaceSymbolProvider"]:
        check(caps.get(name) is True, f"capability {name}")
    client.notify("initialized", {})

    print("didOpen")
    client.notify("textDocument/didOpen", {"textDocument": {"uri": uri, "languageId": "saad", "version": 1, "text": text}})
    check(client.diagnostics(uri) == [], "no diagnostics for tests/index.sd")

    doc = {"uri": uri}

    print("definition")
    # 'c.grow(by: 1.0)' goes to 'grow :: method'
    m = client.request("textDocument/definition", {"textDocument": doc, "position": position_of(text, 52, "grow")})
    loc = m["result"]
    check(loc and loc["uri"] == uri and loc["range"]["start"] == {"line": 37, "character": 0}, f"definition of grow: {loc}")
    # 'geometry.Origin' is in another file
    m = client.request("textDocument/definition", {"textDocument": doc, "position": position_of(text, 62, "Origin")})
    loc = m["result"]
    check(loc and loc["uri"].endswith("/tests/namespaces/geometry/shapes.sd"), f"definition of geometry.Origin: {loc}")
    # Nothing at a keyword
    m = client.request("textDocument/definition", {"textDocument": doc, "position": position_of(text, 43, "return")})
    check(m["result"] is None, f"no definition for a keyword: {m['result']}")

    print("references")
    m = client.request("textDocument/references", {"textDocument": doc, "position": position_of(text, 32, "total"), "context": {"includeDeclaration": True}})
    lines = sorted(r["range"]["start"]["line"] + 1 for r in m["result"])
    check(lines == [32, 57, 65, 66, 69], f"references of total: {lines}")

    print("hover")
    m = client.request("textDocument/hover", {"textDocument": doc, "position": position_of(text, 44, "area")})
    value = m["result"]["contents"]["value"] if m["result"] else ""
    check("area :: method" in value, f"hover of area: {value!r}")
    m = client.request("textDocument/hover", {"textDocument": doc, "position": position_of(text, 53, "Shape")})
    value = m["result"]["contents"]["value"] if m["result"] else ""
    check("Shape :: union" in value and "A shape with a size" in value, f"hover of Shape with its doc: {value!r}")

    print("documentSymbol")
    m = client.request("textDocument/documentSymbol", {"textDocument": doc})
    top = {s["name"]: s for s in m["result"]}
    for name in ["Shape", "Circle", "Status", "Pair", "Count", "Limit", "total", "swapped", "grow", "size", "main"]:
        check(name in top, f"{name} in the outline")
    if "Circle" in top:
        children = [c["name"] for c in top["Circle"]["children"]]
        check(children == ["radius", "area"], f"the children of Circle: {children}")
    check(all(s["kind"] not in (13,) or s["name"] == "total" for s in m["result"]), "no local variables in the outline")

    print("workspace/symbol")
    m = client.request("workspace/symbol", {"query": "swap"})
    names = [s["name"] for s in m["result"]]
    check("swapped" in names, f"workspace symbols for 'swap': {names}")

    print("didChange with an error, and UTF-16 positions")
    broken = text.replace("    n: Count = Limit;", "    n: Count = Limit;\n    letters := \"ø😀\"; x := undefinedName;")
    client.notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 2}, "contentChanges": [{"text": broken}]})
    diagnostics = client.diagnostics(uri)
    check(len(diagnostics) == 1 and "undefinedName" in diagnostics[0]["message"], f"the error of the unsaved text: {diagnostics}")
    if diagnostics:
        want = position_of(broken, 57, "undefinedName")
        check(diagnostics[0]["range"]["start"] == want, f"UTF-16 position {diagnostics[0]['range']['start']} is {want}")

    print("didChange back")
    client.notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 3}, "contentChanges": [{"text": text}]})
    check(client.diagnostics(uri) == [], "the error is cleared")

    print("typing: changes that come one after the other are checked once")
    indexed = sum("Indexed" in line for line in client.log)
    burst = b""
    for version in range(4, 9):
        changed = text.replace("    n: Count = Limit;", f"    n: Count = Limit; typo{version} := oops{version};")
        body = json.dumps({"jsonrpc": "2.0", "method": "textDocument/didChange", "params": {
            "textDocument": {"uri": uri, "version": version}, "contentChanges": [{"text": changed}]}}).encode()
        burst += b"Content-Length: %d\r\n\r\n" % len(body) + body
    client.process.stdin.write(burst)
    client.process.stdin.flush()
    diagnostics = client.diagnostics(uri)
    check(len(diagnostics) == 1 and "oops8" in diagnostics[0]["message"], f"the error of the last change: {diagnostics}")
    m = client.request("textDocument/hover", {"textDocument": doc, "position": position_of(text, 44, "area")})
    runs = sum("Indexed" in line for line in client.log) - indexed
    check(runs == 1, f"checked {runs} times for 5 changes")

    print("didSave")
    client.notify("textDocument/didChange", {"textDocument": {"uri": uri, "version": 9}, "contentChanges": [{"text": text}]})
    client.notify("textDocument/didSave", {"textDocument": {"uri": uri}})
    check(client.diagnostics(uri) == [], "saved without errors")

    print("unknown request")
    m = client.request("textDocument/formatting", {"textDocument": doc, "options": {}})
    check(m.get("error", {}).get("code") == -32601, f"an unknown request is an error: {m}")

    print("shutdown")
    m = client.request("shutdown", None)
    check("result" in m and m["result"] is None, "shutdown")
    client.notify("exit", None)
    code = client.process.wait(timeout=10)
    check(code == 0, f"exit code {code}")

    if failures:
        print("\nThe log of the server:\n" + "\n".join(client.log))
        print(f"\n{len(failures)} checks failed")
        sys.exit(1)
    print("\nAll checks passed")

if __name__ == "__main__":
    main()
