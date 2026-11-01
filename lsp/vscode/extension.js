// Starts the Saad language server (lsp/main.sd) for .sd files. The server does the work, see docs/lsp.md
const path = require("path");
const fs = require("fs");
const vscode = require("vscode");
const { LanguageClient } = require("vscode-languageclient/node");

let client = null;

// The compiler as a full path when it is a file, so the server finds it, or as written, e.g "saad" on the PATH
function compilerPath(config) {
    const compiler = config.get("compilerPath") || "saad";
    const folder = vscode.workspace.workspaceFolders ? vscode.workspace.workspaceFolders[0].uri.fsPath : "";
    const relative = folder ? path.resolve(folder, compiler) : compiler;
    return fs.existsSync(relative) ? relative : compiler;
}

// The server: the setting, or lsp/build/out.exe of the repository the compiler is in (bin/saad.exe)
function serverPath(config, compiler) {
    const configured = config.get("serverPath");
    if (configured) return configured;
    const exe = process.platform === "win32" ? "out.exe" : "saad-lsp";
    const nextToCompiler = path.join(path.dirname(compiler), "..", "lsp", "build", exe);
    return fs.existsSync(nextToCompiler) ? nextToCompiler : "saad-lsp";
}

async function start() {
    const config = vscode.workspace.getConfiguration("saad");
    const compiler = compilerPath(config);
    const server = serverPath(config, compiler);

    const serverOptions = {
        command: server,
        args: ["--compiler", compiler],
        options: { windowsHide: true },
    };
    const clientOptions = {
        documentSelector: [{ scheme: "file", language: "saad" }],
        initializationOptions: { compilerPath: compiler },
        outputChannelName: "Saad",
    };

    client = new LanguageClient("saad", "Saad", serverOptions, clientOptions);
    try {
        await client.start();
    } catch (error) {
        vscode.window.showErrorMessage(`The Saad language server (${server}) couldn't start: ${error.message}. Set saad.serverPath and saad.compilerPath.`);
    }
}

async function stop() {
    if (client) {
        await client.stop();
        client = null;
    }
}

function activate(context) {
    context.subscriptions.push(vscode.commands.registerCommand("saad.restartServer", async () => {
        await stop();
        await start();
    }));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(async (event) => {
        if (event.affectsConfiguration("saad.compilerPath") || event.affectsConfiguration("saad.serverPath")) {
            await stop();
            await start();
        }
    }));
    return start();
}

function deactivate() {
    return stop();
}

module.exports = { activate, deactivate };
