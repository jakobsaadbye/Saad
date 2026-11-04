// Starts the Saad language server (lsp/main.sd) for .sd files. The server does the work, see docs/lsp.md
// And debugs Saad programs with CodeLLDB, see debug.js
const path = require("path");
const fs = require("fs");
const childProcess = require("child_process");
const vscode = require("vscode");
const { LanguageClient, State } = require("vscode-languageclient/node");
const debug = require("./debug");

const isWindows = process.platform === "win32";
let client = null;
let output = null;

function isFile(file) {
    try {
        return fs.statSync(file).isFile();
    } catch {
        return false;
    }
}

function workspaceFolders() {
    return (vscode.workspace.workspaceFolders || []).map((folder) => folder.uri.fsPath);
}

// A program on the PATH, e.g "saad" is C:/dev/Saad/bin/saad.exe
function findOnPath(name) {
    const extensions = isWindows ? ["", ...(process.env.PATHEXT || ".EXE;.CMD;.BAT").split(";")] : [""];
    for (const folder of (process.env.PATH || "").split(path.delimiter)) {
        if (!folder) continue;
        for (const extension of extensions) {
            const candidate = path.join(folder, name + extension);
            if (isFile(candidate)) return candidate;
        }
    }
    return null;
}

// A setting that is a path: as it is when absolute, else in a workspace folder, else on the PATH
function resolveSetting(value) {
    if (!value) return null;
    if (path.isAbsolute(value)) return isFile(value) ? value : null;
    for (const folder of workspaceFolders()) {
        const candidate = path.resolve(folder, value);
        if (isFile(candidate)) return candidate;
        if (isWindows && isFile(candidate + ".exe")) return candidate + ".exe";
    }
    return findOnPath(value);
}

function executable(name) {
    return isWindows ? name + ".exe" : name;
}

// The compiler: the setting, else bin/saad.exe of a workspace folder (the Saad repository), else saad on the PATH
function findCompiler(config) {
    const configured = config.get("compilerPath");
    if (configured && configured !== "saad") {
        return { path: resolveSetting(configured), looked: [configured] };
    }
    const candidates = workspaceFolders().map((folder) => path.join(folder, "bin", executable("saad")));
    for (const candidate of candidates) {
        if (isFile(candidate)) return { path: candidate, looked: candidates };
    }
    return { path: findOnPath("saad"), looked: [...candidates, "saad on the PATH"] };
}

// The folders that can be the Saad repository: the one the compiler is in (bin/saad.exe), and the workspace folders
function repositories(compiler) {
    const folders = [];
    if (compiler) folders.push(path.resolve(path.dirname(compiler), ".."));
    for (const folder of workspaceFolders()) {
        if (!folders.includes(folder)) folders.push(folder);
    }
    return folders;
}

// The server: the setting, else lsp/build/out.exe of the repository, else saad-lsp on the PATH
function findServer(config, compiler) {
    const configured = config.get("serverPath");
    if (configured) {
        return { path: resolveSetting(configured), looked: [configured] };
    }
    const candidates = [];
    for (const repository of repositories(compiler)) {
        candidates.push(path.join(repository, "lsp", "build", executable("out")));
        candidates.push(path.join(repository, "lsp", "build", executable("saad-lsp")));
        if (!isWindows) candidates.push(path.join(repository, "build", "linux", "saad-lsp")); // tools/linux/build_lsp.sh
    }
    for (const candidate of candidates) {
        if (isFile(candidate)) return { path: candidate, looked: candidates };
    }
    return { path: findOnPath("saad-lsp"), looked: [...candidates, "saad-lsp on the PATH"] };
}

// lsp/main.sd of the repository, to build the server from
function findServerSource(compiler) {
    for (const repository of repositories(compiler)) {
        const main = path.join(repository, "lsp", "main.sd");
        if (isFile(main)) return main;
    }
    return null;
}

function log(text) {
    output.appendLine(`[extension] ${text}`);
}

// Builds the server with 'saad build lsp/main.sd', which writes lsp/build/out.exe
function buildServer(compiler, main) {
    return new Promise((resolve) => {
        log(`Building the server: ${compiler} build ${main}`);
        childProcess.execFile(compiler, ["build", main], { cwd: path.dirname(main), windowsHide: true }, (error, stdout, stderr) => {
            if (stdout) output.append(stdout);
            if (stderr) output.append(stderr);
            if (error) {
                log(`The build failed: ${error.message}`);
                output.show(true);
                vscode.window.showErrorMessage("The Saad language server couldn't be built. See the output of Saad.");
                resolve(false);
            } else {
                resolve(true);
            }
        });
    });
}

async function start() {
    const config = vscode.workspace.getConfiguration("saad");

    const compiler = findCompiler(config);
    if (!compiler.path) {
        log(`The compiler wasn't found. Looked for: ${compiler.looked.join(", ")}`);
        const choice = await vscode.window.showErrorMessage(
            "The Saad compiler wasn't found. Set saad.compilerPath to saad.exe, e.g C:/dev/Saad/bin/saad.exe.", "Open settings");
        if (choice) vscode.commands.executeCommand("workbench.action.openSettings", "saad.compilerPath");
        return;
    }

    let server = findServer(config, compiler.path);
    if (!server.path) {
        log(`The server wasn't found. Looked for: ${server.looked.join(", ")}`);
        const main = findServerSource(compiler.path);
        if (main) {
            const choice = await vscode.window.showWarningMessage(
                `The Saad language server isn't built yet. Build it with '${path.basename(compiler.path)} build ${main}'?`, "Build it", "Open settings");
            if (choice === "Build it" && await buildServer(compiler.path, main)) {
                server = findServer(config, compiler.path);
            } else if (choice === "Open settings") {
                vscode.commands.executeCommand("workbench.action.openSettings", "saad.serverPath");
            }
        } else {
            const choice = await vscode.window.showErrorMessage(
                "The Saad language server wasn't found. Build it with 'saad build lsp/main.sd' and set saad.serverPath to lsp/build/out.exe.", "Open settings");
            if (choice) vscode.commands.executeCommand("workbench.action.openSettings", "saad.serverPath");
        }
        if (!server.path) return;
    }

    log(`Starting ${server.path} with the compiler ${compiler.path}`);
    const serverOptions = {
        command: server.path,
        args: ["--compiler", compiler.path],
        options: { windowsHide: true },
    };
    const clientOptions = {
        documentSelector: [{ scheme: "file", language: "saad" }],
        initializationOptions: { compilerPath: compiler.path },
        outputChannel: output,
    };

    client = new LanguageClient("saad", "Saad", serverOptions, clientOptions);
    try {
        await client.start();
    } catch (error) {
        const message = (error && error.message) || String(error);
        log(`The server couldn't start: ${message}`);
        output.show(true);
        vscode.window.showErrorMessage(`The Saad language server (${server.path}) couldn't start: ${message}`);
        client = null;
    }
}

async function stop() {
    const stopping = client;
    client = null;
    if (!stopping) return;
    try {
        // A client that failed to start can't be stopped
        if (stopping.state === State.Running || stopping.state === State.Starting) await stopping.stop();
    } catch (error) {
        log(`Stopping the server: ${(error && error.message) || error}`);
    }
}

async function restart() {
    await stop();
    await start();
}

// The variables of VS Code that a configuration of type saad can use before it is launched
function debugVariables(folder) {
    const editor = vscode.window.activeTextEditor;
    const file = editor && editor.document.uri.scheme === "file" ? editor.document.uri.fsPath : "";
    const workspace = folder ? folder.uri.fsPath : (workspaceFolders()[0] || "");
    return { file, fileDirname: file ? path.dirname(file) : "", workspaceFolder: workspace, workspaceRoot: workspace };
}

// A configuration of type saad (launch.json, or none for the current file) is built with 'saad build --debug', and
// becomes a configuration of CodeLLDB, which VS Code then launches
const debugProvider = {
    provideDebugConfigurations() {
        return [{ type: "saad", request: "launch", name: "Debug the Saad program", program: "${file}" }];
    },

    async resolveDebugConfiguration(folder, config) {
        // F5 without a launch.json: the current file
        if (!config.type && !config.request && !config.name) {
            const editor = vscode.window.activeTextEditor;
            if (!editor || editor.document.languageId !== "saad") return config;   // Not ours: VS Code asks for one
            config = { type: "saad", request: "launch", name: "Debug the Saad program", program: "${file}" };
        }
        if (config.type !== "saad") return config;

        const variables = debugVariables(folder);
        const program = debug.programOf(config, variables);
        if (!program) {
            vscode.window.showErrorMessage("The Saad program to debug wasn't found. Open a .sd file, or set 'program' in launch.json.");
            return undefined;
        }
        if (!vscode.extensions.getExtension("vadimcn.vscode-lldb")) {
            const choice = await vscode.window.showErrorMessage("Debugging Saad programs needs the extension CodeLLDB.", "Install CodeLLDB");
            if (choice) vscode.commands.executeCommand("workbench.extensions.installExtension", "vadimcn.vscode-lldb");
            return undefined;
        }
        const compiler = findCompiler(vscode.workspace.getConfiguration("saad"));
        if (!compiler.path) {
            vscode.window.showErrorMessage("The Saad compiler wasn't found. Set saad.compilerPath to saad.exe.");
            return undefined;
        }

        // Unsaved changes are built too
        await vscode.workspace.saveAll(false);
        try {
            const executable = await vscode.window.withProgress(
                { location: vscode.ProgressLocation.Window, title: `Building ${path.basename(program)} for debugging` },
                () => debug.build(compiler.path, program, config, log));
            return debug.lldbConfiguration(config, executable, variables);
        } catch (error) {
            output.show(true);
            vscode.window.showErrorMessage(error.message);
            return undefined;
        }
    },
};

function activate(context) {
    output = vscode.window.createOutputChannel("Saad");
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider("saad", debugProvider));
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider("saad", {
        provideDebugConfigurations: debugProvider.provideDebugConfigurations,
    }, vscode.DebugConfigurationProviderTriggerKind.Dynamic));
    context.subscriptions.push(vscode.commands.registerCommand("saad.debugFile", (uri) => {
        const file = uri && uri.fsPath ? uri.fsPath : (vscode.window.activeTextEditor && vscode.window.activeTextEditor.document.uri.fsPath);
        if (!file) return;
        const folder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(file));
        vscode.debug.startDebugging(folder, { type: "saad", request: "launch", name: `Debug ${path.basename(file)}`, program: file });
    }));
    context.subscriptions.push(output);
    context.subscriptions.push(vscode.commands.registerCommand("saad.restartServer", restart));
    context.subscriptions.push(vscode.commands.registerCommand("saad.buildServer", async () => {
        const compiler = findCompiler(vscode.workspace.getConfiguration("saad"));
        const main = compiler.path ? findServerSource(compiler.path) : null;
        if (!main) {
            vscode.window.showErrorMessage("lsp/main.sd wasn't found next to the compiler or in the workspace.");
            return;
        }
        await stop(); // The running server can't be replaced on Windows
        if (await buildServer(compiler.path, main)) await start();
    }));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration((event) => {
        if (event.affectsConfiguration("saad.compilerPath") || event.affectsConfiguration("saad.serverPath")) restart();
    }));
    return start();
}

function deactivate() {
    return stop();
}

module.exports = { activate, deactivate, findOnPath, findCompiler, findServer, findServerSource, debugProvider };
