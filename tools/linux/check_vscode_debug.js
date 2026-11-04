// Debugs tests/debug/variables.sd like the VS Code extension does (lsp/vscode/debug.js), without VS Code: it builds
// the program, takes the configuration of CodeLLDB that the extension gives, and talks to CodeLLDB's debug adapter
// over the Debug Adapter Protocol, like VS Code. It checks the stops, the calls and the variables.
//
//   node tools/linux/check_vscode_debug.js <codelldb extension folder> [--llvm]
//
// The folder is an unpacked CodeLLDB, e.g of codelldb-linux-x64.vsix (https://github.com/vadimcn/codelldb/releases),
// the 'extension' folder in it. Requirements: node, gcc, and a saad built with ./build.sh
const path = require("path");
const fs = require("fs");
const net = require("net");
const childProcess = require("child_process");

const repository = path.resolve(__dirname, "..", "..");
const debug = require(path.join(repository, "lsp", "vscode", "debug.js"));
const codelldb = process.argv[2];
const backend = process.argv.includes("--llvm") ? "llvm" : "x64";
const program = path.join(repository, "tests", "debug", "variables.sd");
const source = fs.readFileSync(program, "utf8").split("\n");
const lineOf = (text) => source.findIndex((line) => line.includes(text)) + 1;

let failures = 0;
function check(what, condition, shown) {
    if (condition) return;
    failures++;
    console.log(`FAILED: ${what}${shown !== undefined ? `\n    got: ${JSON.stringify(shown)}` : ""}`);
}

// A client of the Debug Adapter Protocol: requests, and waiting for events
class Client {
    constructor(socket) {
        this.socket = socket;
        this.seq = 1;
        this.pending = new Map();
        this.events = [];
        this.waiters = [];
        this.buffer = Buffer.alloc(0);
        socket.on("data", (data) => this.receive(data));
    }

    receive(data) {
        this.buffer = Buffer.concat([this.buffer, data]);
        for (;;) {
            const end = this.buffer.indexOf("\r\n\r\n");
            if (end < 0) return;
            const length = Number(/Content-Length: (\d+)/i.exec(this.buffer.slice(0, end).toString())[1]);
            if (this.buffer.length < end + 4 + length) return;
            const message = JSON.parse(this.buffer.slice(end + 4, end + 4 + length).toString());
            this.buffer = this.buffer.slice(end + 4 + length);
            if (message.type === "response") {
                const resolve = this.pending.get(message.request_seq);
                this.pending.delete(message.request_seq);
                if (resolve) resolve(message);
            } else if (message.type === "event") {
                this.events.push(message);
                this.waiters = this.waiters.filter((waiter) => !waiter(message));
            } else if (message.type === "request") {
                // E.g runInTerminal, which isn't used with terminal: console
                this.send({ type: "response", request_seq: message.seq, command: message.command, success: false });
            }
        }
    }

    send(message) {
        message.seq = this.seq++;
        const body = JSON.stringify(message);
        this.socket.write(`Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`);
        return message.seq;
    }

    request(command, args) {
        return new Promise((resolve) => {
            const seq = this.send({ type: "request", command, arguments: args });
            this.pending.set(seq, resolve);
        });
    }

    event(name, timeout = 30000) {
        return new Promise((resolve, reject) => {
            const index = this.events.findIndex((event) => event.event === name);
            if (index >= 0) { resolve(this.events.splice(index, 1)[0]); return; }
            const timer = setTimeout(() => reject(new Error(`No ${name} event`)), timeout);
            this.waiters.push((event) => {
                if (event.event !== name) return false;
                this.events.splice(this.events.indexOf(event), 1);
                clearTimeout(timer);
                resolve(event);
                return true;
            });
        });
    }
}

// The variables of the top frame: name -> { value, children (name -> value) }
async function variablesOf(client, threadId) {
    const stack = await client.request("stackTrace", { threadId, startFrame: 0, levels: 20 });
    const frame = stack.body.stackFrames[0];
    const scopes = await client.request("scopes", { frameId: frame.id });
    const locals = scopes.body.scopes.find((scope) => /local/i.test(scope.name)) || scopes.body.scopes[0];
    const response = await client.request("variables", { variablesReference: locals.variablesReference });
    const result = {};
    for (const variable of response.body.variables) {
        const entry = { value: variable.value, type: variable.type, children: {} };
        if (variable.variablesReference) {
            const children = await client.request("variables", { variablesReference: variable.variablesReference });
            for (const child of children.body.variables) entry.children[child.name] = child.value;
        }
        result[variable.name] = entry;
    }
    return { frames: stack.body.stackFrames, variables: result };
}

async function main() {
    if (!codelldb || !fs.existsSync(path.join(codelldb, "adapter", "codelldb"))) {
        console.log("Usage: node tools/linux/check_vscode_debug.js <codelldb extension folder> [--llvm]");
        process.exit(2);
    }

    // What the extension does on F5: build, then the configuration of CodeLLDB
    const log = [];
    const config = { type: "saad", request: "launch", name: "Debug", program: "${file}", backend, terminal: "console" };
    const variables = { file: program, fileDirname: path.dirname(program), workspaceFolder: repository };
    const executable = await debug.build(path.join(repository, "bin", "saad"), debug.programOf(config, variables), config, (text) => log.push(text));
    const lldb = debug.lldbConfiguration(config, executable, variables);
    check("the configuration is of CodeLLDB", lldb.type === "lldb" && lldb.request === "launch", lldb);
    check("the formatters are loaded", lldb.initCommands[0].includes("saad_formatters.py"), lldb.initCommands);

    // CodeLLDB's adapter, on a port
    const port = 47000 + Math.floor(Math.random() * 1000);
    const adapter = childProcess.spawn(path.join(codelldb, "adapter", "codelldb"),
        ["--liblldb", path.join(codelldb, "lldb", "lib", "liblldb.so"), "--port", String(port)], { stdio: ["ignore", "pipe", "pipe"] });
    let adapterOutput = "";
    adapter.stdout.on("data", (data) => { adapterOutput += data; });
    adapter.stderr.on("data", (data) => { adapterOutput += data; });
    let socket = null;
    for (let attempt = 0; attempt < 100 && !socket; attempt++) {
        await new Promise((resolve) => setTimeout(resolve, 100));
        socket = await new Promise((resolve) => {
            const s = net.connect(port, "127.0.0.1", () => resolve(s));
            s.on("error", () => resolve(null));
        });
    }
    if (!socket) throw new Error(`CodeLLDB didn't start:\n${adapterOutput}`);
    const client = new Client(socket);

    try {
        const init = await client.request("initialize", { clientID: "check", adapterID: "lldb", linesStartAt1: true, columnsStartAt1: true, pathFormat: "path" });
        check("CodeLLDB starts", init.success, init.message);
        const launched = client.request("launch", lldb);
        await client.event("initialized");
        const describeLine = lineOf("// stop in describe"), breakLine = lineOf("// break here");
        const breakpoints = await client.request("setBreakpoints", {
            source: { path: program },
            breakpoints: [{ line: describeLine, condition: "/nat i == 1" }, { line: breakLine }],
        });
        check("the breakpoints are in the .sd file", breakpoints.success && breakpoints.body.breakpoints.every((b) => b.verified),
            breakpoints.body && breakpoints.body.breakpoints);
        await client.request("configurationDone", {});
        const launch = await launched;
        check("the program is launched", launch.success, launch.message);

        // In describe, in the second time of the loop
        let stopped = await client.event("stopped");
        let state = await variablesOf(client, stopped.body.threadId);
        const top = state.frames[0];
        check("it stops in describe at the line", top.name.startsWith("describe") && top.line === describeLine && top.source && top.source.path === program,
            { name: top.name, line: top.line, source: top.source && top.source.path });
        check("the caller is main, at the call", state.frames[1].name.startsWith("main") && state.frames[1].line === lineOf("r := describe"),
            { name: state.frames[1].name, line: state.frames[1].line });
        const v = state.variables;
        check("i is 1", v.i && v.i.value === "1", v.i);
        check("squared is 1", v.squared && v.squared.value === "1", v.squared);
        check("total is 14", v.total && v.total.value === "14", v.total);
        check("the parameter name is \"saad\"", v.name && v.name.value === "\"saad\"", v.name);
        check("the parameter p has its members", v.p && v.p.children.x === "7", v.p);
        if (process.env.SHOW) console.log(JSON.stringify(state.variables, null, 1));

        // At 'break here' in main
        await client.request("continue", { threadId: stopped.body.threadId });
        stopped = await client.event("stopped");
        state = await variablesOf(client, stopped.body.threadId);
        check("it stops in main at the line", state.frames[0].name.startsWith("main") && state.frames[0].line === breakLine, state.frames[0].name);
        const m = state.variables;
        if (process.env.SHOW) console.log(JSON.stringify(m, null, 1));
        check("count is 42", m.count && m.count.value === "42", m.count);
        check("color is green", m.color && m.color.value === "green", m.color);
        check("name is \"saad\"", m.name && m.name.value === "\"saad\"", m.name);
        check("list is its elements", m.list && m.list.value === "count = 2" && m.list.children["[1]"] === "20", m.list);
        check("slice is its elements", m.slice && m.slice.children["[0]"] === "10", m.slice);
        check("maybe is 5", m.maybe && m.maybe.value === "5", m.maybe);
        check("nothing is none", m.nothing && m.nothing.value === "none", m.nothing);
        check("value is int 7", m.value && m.value.value === "int 7", m.value);
        check("e is .empty", m.e && m.e.value === ".empty", m.e);
        check("l is .label \"hi\"", m.l && m.l.value === ".label \"hi\"", m.l);
        check("numbers is the array", m.numbers && m.numbers.children["[1]"] === "2", m.numbers);
        check("r is 19", m.r && m.r.value === "19", m.r);

        await client.request("continue", { threadId: stopped.body.threadId });
        const exited = await client.event("exited");
        check("the program ends with 0", exited.body.exitCode === 0, exited.body);
    } finally {
        await client.request("disconnect", { terminateDebuggee: true }).catch(() => {});
        socket.destroy();
        adapter.kill();
    }
    if (failures) {
        console.log(`${failures} failed`);
        process.exit(1);
    }
}

main().catch((error) => {
    console.log(`FAILED: ${error.message}`);
    process.exit(1);
});
