"use strict";
/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
const vscode = require("vscode");
function activate(context) {
    context.subscriptions.push(vscode.commands.registerCommand('extension.node-debug2.toggleSkippingFile', toggleSkippingFile));
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('extensionHost', new ExtensionHostDebugConfigurationProvider()));
}
exports.activate = activate;
function deactivate() {
}
exports.deactivate = deactivate;
function toggleSkippingFile(path) {
    if (!path) {
        const activeEditor = vscode.window.activeTextEditor;
        path = activeEditor && activeEditor.document.fileName;
    }
    if (path && vscode.debug.activeDebugSession) {
        const args = typeof path === 'string' ? { path } : { sourceReference: path };
        vscode.debug.activeDebugSession.customRequest('toggleSkipFileStatus', args);
    }
}
class ExtensionHostDebugConfigurationProvider {
    resolveDebugConfiguration(folder, debugConfiguration) {
        if (useV3()) {
            folder = folder || (vscode.workspace.workspaceFolders ? vscode.workspace.workspaceFolders[0] : undefined);
            debugConfiguration['__workspaceFolder'] = folder === null || folder === void 0 ? void 0 : folder.uri.fsPath;
            debugConfiguration.type = 'pwa-extensionHost';
        }
        else {
            annoyingDeprecationNotification();
        }
        return debugConfiguration;
    }
}
const v3Setting = 'debug.javascript.usePreview';
function useV3() {
    var _a;
    return (_a = getWithoutDefault(v3Setting)) !== null && _a !== void 0 ? _a : true;
}
function getWithoutDefault(setting) {
    var _a;
    const info = vscode.workspace.getConfiguration().inspect(setting);
    return (_a = info === null || info === void 0 ? void 0 : info.workspaceValue) !== null && _a !== void 0 ? _a : info === null || info === void 0 ? void 0 : info.globalValue;
}
let hasShownDeprecation = false;
function annoyingDeprecationNotification() {
    return __awaiter(this, void 0, void 0, function* () {
        if (hasShownDeprecation) {
            return;
        }
        const useNewDebugger = 'Upgrade';
        hasShownDeprecation = true;
        const inspect = vscode.workspace.getConfiguration().inspect(v3Setting);
        const isWorkspace = (inspect === null || inspect === void 0 ? void 0 : inspect.workspaceValue) === false;
        const result = yield vscode.window.showWarningMessage(`You're using a ${isWorkspace ? 'workspace' : 'user'} setting to use VS Code's legacy Node.js debugger, which will be removed soon. Please update your settings using the "Upgrade" button to use our modern debugger.`, useNewDebugger);
        if (result !== useNewDebugger) {
            return;
        }
        const config = vscode.workspace.getConfiguration();
        if ((inspect === null || inspect === void 0 ? void 0 : inspect.globalValue) === false) {
            config.update(v3Setting, true, vscode.ConfigurationTarget.Global);
        }
        if ((inspect === null || inspect === void 0 ? void 0 : inspect.workspaceValue) === false) {
            config.update(v3Setting, true, vscode.ConfigurationTarget.Workspace);
        }
    });
}

//# sourceMappingURL=extension.js.map
