// Debugging of Saad programs: builds the program with debug information ('saad build --debug') and gives VS Code a
// configuration of CodeLLDB (vadimcn.vscode-lldb) to run it, with the formatters of debug/saad_formatters.py, which
// show strings, lists, optionals, unions and any like Saad writes them.
//
// It doesn't use the vscode module, so it can be tried without VS Code (tools/linux/check_vscode_debug.js)
const path = require("path");
const fs = require("fs");
const childProcess = require("child_process");

const FORMATTERS = path.join(__dirname, "debug", "saad_formatters.py");

function isFile(file) {
    try {
        return fs.statSync(file).isFile();
    } catch {
        return false;
    }
}

function newer(file, than) {
    try {
        return fs.statSync(file).mtimeMs >= fs.statSync(than).mtimeMs;
    } catch {
        return false;
    }
}

// Runs a program, and gives its output to log. Resolves to whether it succeeded
function run(command, args, cwd, log) {
    return new Promise((resolve) => {
        log(`${command} ${args.join(" ")}`);
        childProcess.execFile(command, args, { cwd, windowsHide: true, maxBuffer: 16 * 1024 * 1024 }, (error, stdout, stderr) => {
            if (stdout) log(stdout.trimEnd());
            if (stderr) log(stderr.trimEnd());
            if (error && error.code === "ENOENT") log(`${command} wasn't found`);
            resolve(!error);
        });
    });
}

// The variables of VS Code in a value, e.g ${file}, as far as they are known before the launch
function substitute(value, variables) {
    if (typeof value !== "string") return value;
    return value.replace(/\$\{(\w+)\}/g, (whole, name) => (name in variables && variables[name] ? variables[name] : whole));
}

// The program of the configuration: a .sd file, absolute or in the workspace folder
function programOf(config, variables) {
    let program = substitute(config.program || "${file}", variables);
    if (!program || program.includes("${")) return null;
    if (!path.isAbsolute(program) && variables.workspaceFolder) program = path.resolve(variables.workspaceFolder, program);
    return program;
}

// On Linux, a program of Saad is the object file of 'saad build --elf' with the C functions of tools/linux/shim.c,
// linked with gcc (see tools/linux). The shim is next to the compiler, in the Saad repository
async function linkForLinux(compiler, object, log) {
    const repository = path.resolve(path.dirname(compiler), "..");
    const shim = path.join(repository, "tools", "linux", "shim.c");
    if (!isFile(shim)) throw new Error(`Debugging on Linux needs tools/linux/shim.c of the Saad repository, which isn't at ${shim}`);
    const build = path.dirname(object);
    const shimObject = path.join(build, "shim.o");
    if (!newer(shimObject, shim) && !await run("gcc", ["-O0", "-w", "-c", `-I${path.join(repository, "src")}`, shim, "-o", shimObject], build, log)) {
        throw new Error("gcc couldn't compile tools/linux/shim.c");
    }
    const args = ["-no-pie", "-z", "noexecstack", object, shimObject];
    // libpq (std/dbms/postgres) only for the programs that call it
    if (fs.readFileSync(object).includes("ms_PQ")) {
        const pq = path.join(repository, "tools", "linux", "shim_pq.c");
        const pqObject = path.join(build, "shim_pq.o");
        if (!newer(pqObject, pq) && !await run("gcc", ["-O0", "-w", "-c", "-I/usr/include/postgresql", pq, "-o", pqObject], build, log)) {
            throw new Error("gcc couldn't compile tools/linux/shim_pq.c (it needs libpq-dev)");
        }
        args.push(pqObject, "-lpq");
    }
    const executable = path.join(build, "out");
    args.push("-o", executable, "-lm", "-lpthread");
    if (!await run("gcc", args, build, log)) throw new Error("gcc couldn't link the program");
    return executable;
}

// Builds the program with debug information. Resolves to the executable, or throws an Error that says why not
async function build(compiler, program, config, log) {
    if (!isFile(program)) throw new Error(`The program ${program} doesn't exist`);
    if (path.extname(program) !== ".sd") throw new Error(`The program to debug is a .sd file, not ${program}`);
    const directory = path.dirname(program);
    const args = ["build", program, "--debug"];
    if (config.backend === "llvm") args.push("--llvm");
    if (Number.isInteger(config.optimization)) args.push(`-O${config.optimization}`);

    if (process.platform === "win32") {
        if (!await run(compiler, args, directory, log)) throw new Error("The program couldn't be built. See the output of Saad");
        return path.join(directory, "build", "out.exe");
    }
    if (process.platform === "linux") {
        args.push("--elf");
        if (!await run(compiler, args, directory, log)) throw new Error("The program couldn't be built. See the output of Saad");
        return linkForLinux(compiler, path.join(directory, "build", "out.o"), log);
    }
    throw new Error("Saad programs run on Windows, and on Linux with tools/linux");
}

// The configuration of CodeLLDB for a configuration of type saad, after the program is built
function lldbConfiguration(config, executable, variables) {
    const formatters = FORMATTERS.replace(/\\/g, "/");
    const lldb = {
        type: "lldb",
        request: "launch",
        name: config.name || "Debug",
        program: executable,
        args: (config.args || []).map((arg) => substitute(arg, variables)),
        cwd: substitute(config.cwd, variables) || variables.workspaceFolder || path.dirname(executable),
        stopOnEntry: !!config.stopOnEntry,
        initCommands: [`command script import "${formatters}"`, ...(config.initCommands || [])],
    };
    if (config.env) lldb.env = config.env;
    if (config.terminal) lldb.terminal = config.terminal;
    return lldb;
}

module.exports = { build, lldbConfiguration, programOf, substitute, FORMATTERS };
