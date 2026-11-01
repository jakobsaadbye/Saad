// Makes the GIFs of lsp/README.md: completion, hover and values from their context.
//
//     tools/linux/build_lsp.sh && node tools/linux/lsp_demo_gifs.js
//
// It starts the language server (build/linux/saad-lsp), opens a small program in it, and "types" in it. For every
// frame it asks the server, like an editor does, for the completions, the signature, the hover and the semantic tokens
// (the colors of the names), and draws the editor with them in Chromium, in the colors of VS Code's dark theme. So what
// the GIFs show is what the server answers; only the editor around it is drawn here. ffmpeg makes the GIFs.
// Needs Playwright with Chromium, and ffmpeg.

const { spawn, execFileSync } = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { chromium } = require("playwright");

const REPO = path.resolve(__dirname, "..", "..");
const SERVER = path.join(REPO, "build", "linux", "saad-lsp");
const OUT = path.join(REPO, "lsp", "images");
const WORK = fs.mkdtempSync(path.join(os.tmpdir(), "saad-gifs-"));
const DEMO = path.join(WORK, "main.sd");
const URI = "file://" + DEMO;

const PROGRAM = `import fmt;

// A player of the game
Player :: struct {
    name:   string;
    health: int = 100;
    status: Status;
}

Status :: enum { alive, poisoned, dead }

// Takes the damage, and returns if the player is still alive
hit :: method (p: *Player, damage: int) -> bool {
    p.health -= damage;
    if p.health <= 0 {
        p.status = .dead;
    }
    return p.status != .dead;
}

main :: () {
    player := Player{name: "Ada"};
    alive := player.hit(25);
    fmt.print("% has % health", player.name, player.health);
@
}
`;

//
// A language server client
//

class Client {
    constructor() {
        this.process = spawn(SERVER, ["--compiler", path.join(REPO, "bin", "saad")], { env: { ...process.env, TMPDIR: WORK } });
        this.buffer = Buffer.alloc(0);
        this.waiting = new Map();
        this.nextId = 0;
        this.version = 0;
        this.process.stdout.on("data", (data) => this.read(data));
        this.process.stderr.on("data", () => {});
    }
    read(data) {
        this.buffer = Buffer.concat([this.buffer, data]);
        while (true) {
            const end = this.buffer.indexOf("\r\n\r\n");
            if (end < 0) return;
            const length = Number(/Content-Length: (\d+)/i.exec(this.buffer.slice(0, end).toString())[1]);
            if (this.buffer.length < end + 4 + length) return;
            const message = JSON.parse(this.buffer.slice(end + 4, end + 4 + length).toString());
            this.buffer = this.buffer.slice(end + 4 + length);
            if (message.id !== undefined && this.waiting.has(message.id)) {
                this.waiting.get(message.id)(message.result);
                this.waiting.delete(message.id);
            }
        }
    }
    send(message) {
        const body = Buffer.from(JSON.stringify({ jsonrpc: "2.0", ...message }));
        this.process.stdin.write(`Content-Length: ${body.length}\r\n\r\n`);
        this.process.stdin.write(body);
    }
    request(method, params) {
        const id = ++this.nextId;
        return new Promise((resolve) => { this.waiting.set(id, resolve); this.send({ id, method, params }); });
    }
    notify(method, params) { this.send({ method, params }); }
    change(text) {
        this.notify("textDocument/didChange", { textDocument: { uri: URI, version: ++this.version }, contentChanges: [{ text }] });
    }
}

//
// Drawing the editor
//

const COLORS = {
    text: "#d4d4d4", keyword: "#c586c0", declaration: "#569cd6", type: "#4ec9b0", function: "#dcdcaa", variable: "#9cdcfe",
    enumMember: "#4fc1ff", string: "#ce9178", number: "#b5cea8", comment: "#6a9955", constant: "#569cd6",
};
const TOKEN_COLORS = {
    namespace: COLORS.type, type: COLORS.type, struct: COLORS.type, enum: COLORS.type, enumMember: COLORS.enumMember,
    function: COLORS.function, method: COLORS.function, property: COLORS.variable, variable: COLORS.variable,
    parameter: COLORS.variable, typeParameter: COLORS.type,
};

// The colors of a line: the grammar of lsp/vscode for keywords, strings, numbers and comments, then the semantic tokens
function colorLine(line, tokens) {
    const colors = new Array(line.length).fill(COLORS.text);
    const paint = (re, color) => { for (const m of line.matchAll(re)) for (let i = m.index; i < m.index + m[0].length; i++) colors[i] = color; };
    paint(/\b(if|else|for|while|in|switch|case|return|break|continue)\b/g, COLORS.keyword);
    paint(/\b(struct|union|enum|method|extern|namespace|import|use|as|int|bool|string|f32|f64|u8|i64)\b/g, COLORS.declaration);
    paint(/\b(true|false|null)\b/g, COLORS.constant);
    paint(/\b\d+(\.\d+)?\b/g, COLORS.number);
    for (const t of tokens) for (let i = t.start; i < t.start + t.length && i < line.length; i++) colors[i] = TOKEN_COLORS[t.type] || COLORS.text;
    paint(/"([^"\\]|\\.)*"?/g, COLORS.string);
    paint(/\/\/.*$/g, COLORS.comment);
    return colors;
}

function escape(text) {
    return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function lineHtml(line, colors) {
    let html = "", i = 0;
    while (i < line.length) {
        let j = i;
        while (j < line.length && colors[j] === colors[i]) j++;
        html += `<span style="color:${colors[i]}">${escape(line.slice(i, j))}</span>`;
        i = j;
    }
    return html;
}

// Code in a widget, e.g the signature in a hover: the grammar, and the names of types and of the declaration
function codeHtml(code) {
    return code.split("\n").map((l) => {
        const tokens = [];
        for (const m of l.matchAll(/\b[A-Z]\w*/g)) tokens.push({ start: m.index, length: m[0].length, type: "type" });
        const declared = /^(\w+) :: (method|<|\()/.exec(l);
        if (declared) tokens.push({ start: 0, length: declared[1].length, type: "function" });
        return lineHtml(l, colorLine(l, tokens));
    }).join("<br>");
}

const KIND_ICONS = {
    2: ["#b180d7", "M3 5l5-3 5 3v6l-5 3-5-3z"],          // method
    3: ["#b180d7", "M3 5l5-3 5 3v6l-5 3-5-3z"],          // function
    5: ["#75beff", "M3 4h10v8H3z"],                      // field
    6: ["#75beff", "M2 6h12v4H2z"],                      // variable
    9: ["#d4d4d4", "M3 3h10v10H3z"],                     // namespace
    13: ["#ee9d28", "M3 3h10v10H3zM5 6h6M5 9h6"],        // enum
    14: ["#d4d4d4", "M2 7h12v2H2z"],                     // keyword
    20: ["#75beff", "M3 3h10v10H3zM5 7h6"],              // enum member
    22: ["#ee9d28", "M2 4h12v8H2zM5 4v8M11 4v8"],        // struct
};

function icon(kind) {
    const [color, d] = KIND_ICONS[kind] || ["#d4d4d4", "M4 4h8v8H4z"];
    return `<svg width="16" height="16" viewBox="0 0 16 16"><path d="${d}" fill="none" stroke="${color}" stroke-width="1.4"/></svg>`;
}

// The letters of the label that the typed text matched
function highlight(label, typed) {
    let html = "", t = 0;
    for (const c of label) {
        if (t < typed.length && c.toLowerCase() === typed[t].toLowerCase()) { html += `<b>${escape(c)}</b>`; t++; }
        else html += escape(c);
    }
    return html;
}

const CHAR_WIDTH = 8.4, LINE_HEIGHT = 20, GUTTER = 48;

function frameHtml(frame) {
    const lines = frame.text.split("\n");
    const first = frame.firstLine || 0;
    const shown = lines.slice(first, first + frame.lineCount);
    let code = "";
    shown.forEach((line, i) => {
        const n = first + i;
        const current = frame.cursor && frame.cursor.line === n;
        code += `<div class="row${current ? " current" : ""}"><span class="number">${n + 1}</span><span class="code">${lineHtml(line, colorLine(line, frame.tokens[n] || []))}</span></div>`;
    });

    const x = (col) => GUTTER + 12 + col * CHAR_WIDTH;
    const y = (line) => (line - first) * LINE_HEIGHT;
    let widgets = "";
    if (frame.cursor && frame.showCursor !== false) {
        widgets += `<div class="cursor" style="left:${x(frame.cursor.character)}px;top:${y(frame.cursor.line) + 2}px"></div>`;
    }
    if (frame.suggest) {
        const { items, selected, typed, at } = frame.suggest;
        const rows = items.slice(0, 8).map((item, i) =>
            `<div class="item${i === selected ? " selected" : ""}">${icon(item.kind)}<span class="label">${highlight(item.label, typed)}</span>` +
            `${i === selected && item.detail ? `<span class="detail">${escape(item.detail)}</span>` : ""}</div>`).join("");
        widgets += `<div class="suggest" style="left:${x(at.character) - 22}px;top:${y(at.line) + LINE_HEIGHT + 2}px">${rows}</div>`;
    }
    if (frame.signature) {
        const s = frame.signature;
        const active = s.parameters[s.active] || "";
        const label = escape(s.label).replace(escape(active), `<b class="active">${escape(active)}</b>`);
        widgets += `<div class="widget signature above" style="left:${x(frame.cursor.character) - 60}px;top:${y(frame.cursor.line) - 2}px"><span class="code">${label}</span></div>`;
    }
    if (frame.hover) {
        const { value, at } = frame.hover;
        const match = /```saad\n([\s\S]*?)\n```([\s\S]*)/.exec(value);
        const code = match ? codeHtml(match[1]) : escape(value);
        const doc = match && match[2].trim() ? `<div class="doc">${escape(match[2].trim())}</div>` : "";
        widgets += `<div class="widget hover above" style="left:${x(at.character) - 6}px;top:${y(at.line) - 2}px"><div class="code">${code}</div>${doc}</div>`;
        widgets += `<svg class="pointer" style="left:${x(at.character + 2)}px;top:${y(at.line) + 8}px" width="14" height="20" viewBox="0 0 14 20">
            <path d="M1 1v15l4-4 3 7 2-1-3-7h5z" fill="white" stroke="black" stroke-width="1"/></svg>`;
    }

    return `<!doctype html><html><head><style>
        body { margin: 0; background: #1e1e1e; }
        #editor { position: relative; width: 760px; height: ${frame.lineCount * LINE_HEIGHT + 24}px; background: #1e1e1e; overflow: hidden;
                  font: 14px/${LINE_HEIGHT}px "JetBrains Mono", "DejaVu Sans Mono", monospace; color: #d4d4d4;
                  font-variant-ligatures: none; font-feature-settings: "liga" 0, "calt" 0; }
        #tabs { height: 24px; background: #252526; color: #cccccc; font: 12px/24px sans-serif; padding-left: 12px; border-bottom: 1px solid #2b2b2b; }
        #tabs b { color: #6fa8dc; font-weight: bold; margin-right: 6px; }
        #lines { position: relative; }
        .row { height: ${LINE_HEIGHT}px; white-space: pre; }
        .row.current { background: #282828; }
        .number { display: inline-block; width: ${GUTTER}px; text-align: right; color: #6e7681; margin-right: 12px; }
        .row.current .number { color: #cccccc; }
        .cursor { position: absolute; width: 2px; height: 16px; background: #aeafad; }
        .suggest { position: absolute; width: 420px; background: #252526; border: 1px solid #454545; box-shadow: 0 2px 8px #000a; }
        .item { height: 22px; display: flex; align-items: center; padding: 0 6px; gap: 6px; white-space: pre; }
        .item.selected { background: #04395e; outline: 1px solid #2b79d1; }
        .item b { color: #18a3ff; font-weight: bold; }
        .label { flex: 1; }
        .detail { color: #9d9d9d; font-size: 12px; overflow: hidden; text-overflow: ellipsis; max-width: 250px; }
        .widget { position: absolute; background: #252526; border: 1px solid #454545; padding: 4px 8px; box-shadow: 0 2px 8px #000a; white-space: pre; max-width: 600px; }
        .above { transform: translateY(-100%); }
        .signature .active { color: #18a3ff; font-weight: bold; text-decoration: underline; }
        .hover .doc { font: 13px/18px sans-serif; color: #cccccc; border-top: 1px solid #454545; margin-top: 6px; padding-top: 6px; white-space: normal; }
        .pointer { position: absolute; }
    </style></head><body><div id="editor"><div id="tabs"><b>S</b>main.sd</div><div id="lines">${code}${widgets}</div></div></body></html>`;
}

//
// The scenes
//

async function tokensOf(client, text) {
    const result = await client.request("textDocument/semanticTokens/full", { textDocument: { uri: URI } });
    const legend = client.legend;
    const tokens = {};
    let line = 0, start = 0;
    const data = result ? result.data : [];
    for (let i = 0; i < data.length; i += 5) {
        line += data[i];
        start = data[i] ? data[i + 1] : start + data[i + 1];
        (tokens[line] = tokens[line] || []).push({ start, length: data[i + 2], type: legend[data[i + 3]] });
    }
    return tokens;
}

function filterItems(items, typed) {
    const matches = (label) => { let t = 0; for (const c of label.toLowerCase()) if (t < typed.length && c === typed[t].toLowerCase()) t++; return t === typed.length; };
    return items.filter((item) => matches(item.label));
}

// The text with what is typed at the '@' of the program, and the cursor after it
function program(typed) {
    const line = PROGRAM.split("\n").indexOf("@");
    return { text: PROGRAM.replace("@", "    " + typed), cursor: { line, character: 4 + typed.length } };
}

async function typingScene(client, steps) {
    const frames = [];
    for (const step of steps) {
        const { text, cursor } = program(step.typed);
        client.change(text);
        const frame = { text, cursor, lineCount: 14, firstLine: 18, tokens: {}, duration: step.duration || 110 };
        frame.tokens = await tokensOf(client, text);
        if (step.complete) {
            const items = await client.request("textDocument/completion", { textDocument: { uri: URI }, position: step.complete.at || cursor });
            const typed = step.complete.typed || "";
            frame.suggest = { items: filterItems(items, typed), selected: step.complete.selected || 0, typed, at: { line: cursor.line, character: cursor.character - typed.length } };
        }
        if (step.signature) {
            const help = await client.request("textDocument/signatureHelp", { textDocument: { uri: URI }, position: cursor });
            if (help) frame.signature = { label: help.signatures[0].label, parameters: help.signatures[0].parameters.map((p) => p.label), active: help.activeParameter };
        }
        frames.push(frame);
    }
    return frames;
}

async function hoverScene(client, targets) {
    const text = PROGRAM.replace("@\n", "");
    client.change(text);
    const tokens = await tokensOf(client, text);
    const lines = text.split("\n");
    const frames = [{ text, tokens, lineCount: 15, firstLine: 11, duration: 700 }];
    for (const [lineText, word] of targets) {
        const line = lines.findIndex((l) => l.includes(lineText));
        const character = lines[line].indexOf(word, lines[line].indexOf(lineText)) + 1;
        const hover = await client.request("textDocument/hover", { textDocument: { uri: URI }, position: { line, character } });
        frames.push({ text, tokens, lineCount: 15, firstLine: 11, hover: { value: hover.contents.value, at: { line, character: character - 1 } }, duration: 2600 });
        frames.push({ text, tokens, lineCount: 15, firstLine: 11, duration: 400 });
    }
    return frames;
}

async function render(page, frames, name) {
    const dir = fs.mkdtempSync(path.join(WORK, name + "-"));
    const list = [];
    for (let i = 0; i < frames.length; i++) {
        await page.setContent(frameHtml(frames[i]));
        const file = path.join(dir, `${String(i).padStart(3, "0")}.png`);
        await (await page.$("#editor")).screenshot({ path: file });
        list.push(`file '${file}'\nduration ${(frames[i].duration / 1000).toFixed(3)}`);
    }
    list.push(`file '${path.join(dir, `${String(frames.length - 1).padStart(3, "0")}.png`)}'`);
    fs.writeFileSync(path.join(dir, "frames.txt"), list.join("\n") + "\n");
    const gif = path.join(OUT, name + ".gif");
    execFileSync("ffmpeg", ["-y", "-loglevel", "error", "-f", "concat", "-safe", "0", "-i", path.join(dir, "frames.txt"),
        "-vf", "split[a][b];[a]palettegen=max_colors=96:stats_mode=full[p];[b][p]paletteuse=dither=none", "-loop", "0", gif]);
    console.log(`${gif}: ${frames.length} frames, ${Math.round(fs.statSync(gif).size / 1024)} KB`);
}

async function main() {
    fs.mkdirSync(OUT, { recursive: true });
    fs.writeFileSync(DEMO, PROGRAM.replace("@\n", ""));
    const client = new Client();
    const init = await client.request("initialize", { processId: null, rootUri: "file://" + WORK, capabilities: {} });
    client.legend = init.capabilities.semanticTokensProvider.legend.tokenTypes;
    client.notify("initialized", {});
    client.notify("textDocument/didOpen", { textDocument: { uri: URI, languageId: "saad", version: 0, text: PROGRAM.replace("@\n", "") } });

    const browser = await chromium.launch();
    const page = await browser.newPage({ viewport: { width: 760, height: 400 } });

    // Completion: the members of a value, then the signature of the method
    const typing = [];
    for (const typed of ["", "i", "if", "if ", "if p", "if pl", "if pla", "if play", "if playe", "if player"]) typing.push({ typed });
    typing.push({ typed: "if player.", complete: {}, duration: 1300 });
    typing.push({ typed: "if player.h", complete: { typed: "h" }, duration: 500 });
    typing.push({ typed: "if player.h", complete: { typed: "h", selected: 1 }, duration: 1200 });
    typing.push({ typed: "if player.hit", duration: 300 });
    typing.push({ typed: "if player.hit(", signature: true, duration: 1500 });
    for (const typed of ["if player.hit(1", "if player.hit(10"]) typing.push({ typed, signature: true });
    typing.push({ typed: "if player.hit(10)", duration: 300 });
    typing.push({ typed: "if player.hit(10) {", duration: 2000 });
    await render(page, await typingScene(client, typing), "completion");

    // Values from their context: '.' after == gives the values of the enum
    const context = [];
    for (const typed of ["", "i", "if", "if ", "if p", "if pl", "if pla", "if play", "if playe", "if player", "if player.", "if player.s",
                         "if player.st", "if player.sta", "if player.stat", "if player.statu", "if player.status", "if player.status ",
                         "if player.status =", "if player.status =="]) context.push({ typed });
    context.push({ typed: "if player.status == ", duration: 200 });
    context.push({ typed: "if player.status == .", complete: {}, duration: 1300 });
    context.push({ typed: "if player.status == .", complete: { selected: 1 }, duration: 400 });
    context.push({ typed: "if player.status == .", complete: { selected: 2 }, duration: 1000 });
    context.push({ typed: "if player.status == .dead", duration: 300 });
    context.push({ typed: "if player.status == .dead {", duration: 2000 });
    await render(page, await typingScene(client, context), "context");

    // Hover: the signature and the doc comment of a declaration
    await render(page, await hoverScene(client, [["alive := player.hit(25)", "hit"], ["player := Player{", "Player"], ["fmt.print(", "print"]]), "hover");

    await browser.close();
    client.process.kill();
    fs.rmSync(WORK, { recursive: true, force: true });
}

main().catch((error) => { console.error(error); process.exit(1); });
