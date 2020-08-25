module.exports =
/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, { enumerable: true, get: getter });
/******/ 		}
/******/ 	};
/******/
/******/ 	// define __esModule on exports
/******/ 	__webpack_require__.r = function(exports) {
/******/ 		if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 			Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 		}
/******/ 		Object.defineProperty(exports, '__esModule', { value: true });
/******/ 	};
/******/
/******/ 	// create a fake namespace object
/******/ 	// mode & 1: value is a module id, require it
/******/ 	// mode & 2: merge all properties of value into the ns
/******/ 	// mode & 4: return value when already ns object
/******/ 	// mode & 8|1: behave like require
/******/ 	__webpack_require__.t = function(value, mode) {
/******/ 		if(mode & 1) value = __webpack_require__(value);
/******/ 		if(mode & 8) return value;
/******/ 		if((mode & 4) && typeof value === 'object' && value && value.__esModule) return value;
/******/ 		var ns = Object.create(null);
/******/ 		__webpack_require__.r(ns);
/******/ 		Object.defineProperty(ns, 'default', { enumerable: true, value: value });
/******/ 		if(mode & 2 && typeof value != 'string') for(var key in value) __webpack_require__.d(ns, key, function(key) { return value[key]; }.bind(null, key));
/******/ 		return ns;
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 10);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports) {

module.exports = require("path");

/***/ }),
/* 1 */
/***/ (function(module, exports) {

module.exports = require("fs");

/***/ }),
/* 2 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __webpack_require__(0);
/**
 * Returns whether the user can access the given file path.
 */
async function canAccess({ access }, file) {
    if (!file) {
        return false;
    }
    try {
        await access(file);
        return true;
    }
    catch (e) {
        return false;
    }
}
exports.canAccess = canAccess;
const regexChars = '/\\.?*()^${}|[]+';
/**
 * Escape regex special characters from the string.
 */
function escapeRegexSpecialChars(str, except) {
    const useRegexChars = regexChars
        .split('')
        .filter(c => !except || except.indexOf(c) < 0)
        .join('')
        .replace(/[\\\]]/g, '\\$&');
    const r = new RegExp(`[${useRegexChars}]`, 'g');
    return str.replace(r, '\\$&');
}
exports.escapeRegexSpecialChars = escapeRegexSpecialChars;
/**
 * Gets the configured Chrome path, if any.
 */
async function preferredChromePath(fs, env) {
    if (await canAccess(fs, env.CHROME_PATH)) {
        return env.CHROME_PATH;
    }
}
exports.preferredChromePath = preferredChromePath;
/**
 * Gets the configured Edge path, if any.
 */
async function preferredEdgePath(fs, env) {
    if (await canAccess(fs, env.EDGE_PATH)) {
        return env.EDGE_PATH;
    }
}
exports.preferredEdgePath = preferredEdgePath;
/**
 * Sorts the set of installations,
 */
function sort(installations, priorities) {
    const defaultPriority = 10;
    return ([...installations]
        .filter(inst => !!inst)
        .map(inst => {
        const priority = priorities.find(p => p.regex.test(inst));
        return priority
            ? { path: inst, weight: priority.weight, quality: priority.quality }
            : { path: inst, weight: defaultPriority, quality: "dev" /* Dev */ };
    })
        // sort based on weight
        .sort((a, b) => b.weight - a.weight)
        // remove weight
        .map(p => ({ path: p.path, quality: p.quality })));
}
exports.sort = sort;
/**
 * Finds binaries for Windows platforms by looking for the given path
 * suffixes in each of the local app data and program files directories
 * on the machine, returning complete absolute paths that match.
 */
async function findWindowsCandidates(env, fs, suffixes) {
    const prefixes = [env.LOCALAPPDATA, env.PROGRAMFILES, env['PROGRAMFILES(X86)']].filter((p) => !!p);
    const todo = [];
    for (const prefix of prefixes) {
        for (const suffix of suffixes) {
            const candidate = path_1.win32.join(prefix, suffix.name);
            todo.push(canAccess(fs, candidate).then(ok => ok ? { path: candidate, quality: suffix.type } : undefined));
        }
    }
    return (await Promise.all(todo)).filter((e) => !!e);
}
exports.findWindowsCandidates = findWindowsCandidates;
//# sourceMappingURL=util.js.map

/***/ }),
/* 3 */
/***/ (function(module, exports) {

module.exports = require("os");

/***/ }),
/* 4 */
/***/ (function(module, exports) {

module.exports = require("child_process");

/***/ }),
/* 5 */
/***/ (function(module, exports) {

module.exports = require("vscode");

/***/ }),
/* 6 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __webpack_require__(0);
const util_1 = __webpack_require__(2);
/**
 * Base class providing utilities for the Darwin browser finders.
 */
class DarwinFinderBase {
    constructor(env, fs, execa) {
        this.env = env;
        this.fs = fs;
        this.execa = execa;
    }
    /**
     * Finds apps matching the given pattern in the launch service register.
     */
    async findLaunchRegisteredApps(pattern, defaultPaths, suffixes) {
        const lsRegister = '/System/Library/Frameworks/CoreServices.framework' +
            '/Versions/A/Frameworks/LaunchServices.framework' +
            '/Versions/A/Support/lsregister';
        const { stdout, } = await this.execa.command(`${lsRegister} -dump | grep -i '${pattern}'| awk '{$1=""; print $0}'`, { shell: true, stdio: 'pipe' });
        const paths = [...defaultPaths, ...stdout.split('\n').map(l => l.trim())].filter(l => !!l);
        const preferred = this.getPreferredPath();
        if (preferred) {
            paths.push(preferred);
        }
        const installations = new Set();
        for (const inst of paths) {
            for (const suffix of suffixes) {
                const execPath = path_1.posix.join(inst.trim(), suffix);
                try {
                    await this.fs.access(execPath);
                    installations.add(execPath);
                }
                catch (e) {
                    // no access => ignored
                }
            }
        }
        return installations;
    }
    /**
     * Creates priorities for the {@link sort} function that places browsers
     * in proper order based on their installed location./
     */
    createPriorities(priorities) {
        const home = this.env.HOME && util_1.escapeRegexSpecialChars(this.env.HOME);
        const preferred = this.getPreferredPath();
        const mapped = priorities.reduce((acc, p) => [
            ...acc,
            {
                regex: new RegExp(`^/Applications/.*${p.name}`),
                weight: p.weight + 100,
                quality: p.quality,
            },
            {
                regex: new RegExp(`^${home}/Applications/.*${p.name}`),
                weight: p.weight,
                quality: p.quality,
            },
            {
                regex: new RegExp(`^/Volumes/.*${p.name}`),
                weight: p.weight - 100,
                quality: p.quality,
            },
        ], []);
        if (preferred) {
            mapped.unshift({
                regex: new RegExp(util_1.escapeRegexSpecialChars(preferred)),
                weight: 151,
                quality: "custom" /* Custom */,
            });
        }
        return mapped;
    }
}
exports.DarwinFinderBase = DarwinFinderBase;
//# sourceMappingURL=darwinFinderBase.js.map

/***/ }),
/* 7 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
Object.defineProperty(exports,"__esModule",{value:true});exports.SIGRTMAX=exports.getRealtimeSignals=void 0;
const getRealtimeSignals=function(){
const length=SIGRTMAX-SIGRTMIN+1;
return Array.from({length},getRealtimeSignal);
};exports.getRealtimeSignals=getRealtimeSignals;

const getRealtimeSignal=function(value,index){
return{
name:`SIGRT${index+1}`,
number:SIGRTMIN+index,
action:"terminate",
description:"Application-specific signal (realtime)",
standard:"posix"};

};

const SIGRTMIN=34;
const SIGRTMAX=64;exports.SIGRTMAX=SIGRTMAX;
//# sourceMappingURL=realtime.js.map

/***/ }),
/* 8 */
/***/ (function(module, exports, __webpack_require__) {

var wrappy = __webpack_require__(57)
module.exports = wrappy(once)
module.exports.strict = wrappy(onceStrict)

once.proto = once(function () {
  Object.defineProperty(Function.prototype, 'once', {
    value: function () {
      return once(this)
    },
    configurable: true
  })

  Object.defineProperty(Function.prototype, 'onceStrict', {
    value: function () {
      return onceStrict(this)
    },
    configurable: true
  })
})

function once (fn) {
  var f = function () {
    if (f.called) return f.value
    f.called = true
    return f.value = fn.apply(this, arguments)
  }
  f.called = false
  return f
}

function onceStrict (fn) {
  var f = function () {
    if (f.called)
      throw new Error(f.onceError)
    f.called = true
    return f.value = fn.apply(this, arguments)
  }
  var name = fn.name || 'Function wrapped with `once`'
  f.onceError = name + " shouldn't be called more than once"
  f.called = false
  return f
}


/***/ }),
/* 9 */
/***/ (function(module, exports) {

module.exports = require("stream");

/***/ }),
/* 10 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const vscode = __importStar(__webpack_require__(5));
const sessionManager_1 = __webpack_require__(11);
const spawn_1 = __webpack_require__(15);
const os_1 = __webpack_require__(3);
let manager;
function activate(context) {
    var _a;
    manager = new sessionManager_1.SessionManager(new spawn_1.BrowserSpawner((_a = context.storagePath, (_a !== null && _a !== void 0 ? _a : os_1.tmpdir()))));
    context.subscriptions.push(vscode.commands.registerCommand('js-debug-companion.launchAndAttach', params => {
        var _a;
        (_a = manager) === null || _a === void 0 ? void 0 : _a.create(params).catch(err => vscode.window.showErrorMessage(err.message));
    }), vscode.commands.registerCommand('js-debug-companion.kill', ({ launchId }) => {
        var _a;
        (_a = manager) === null || _a === void 0 ? void 0 : _a.destroy(launchId);
    }));
}
exports.activate = activate;
function deactivate() {
    var _a;
    (_a = manager) === null || _a === void 0 ? void 0 : _a.dispose();
    manager = undefined;
}
exports.deactivate = deactivate;


/***/ }),
/* 11 */
/***/ (function(module, exports, __webpack_require__) {

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
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const session_1 = __webpack_require__(12);
const net_1 = __webpack_require__(14);
const vscode = __importStar(__webpack_require__(5));
class SessionManager {
    constructor(spawn) {
        this.spawn = spawn;
        this.sessions = new Map();
    }
    /**
     * Creates a session with the set of launch parameters.
     */
    create(params) {
        return __awaiter(this, void 0, void 0, function* () {
            const session = new session_1.Session();
            this.sessions.set(params.launchId, session);
            session.onClose(() => this.sessions.delete(params.launchId));
            session.onError(err => {
                vscode.window.showErrorMessage(`Error running browser: ${err.message || err.stack}`);
                this.sessions.delete(params.launchId);
            });
            yield Promise.all([
                this.addChildSocket(session, params),
                this.addChildBrowser(session, params),
            ]);
        });
    }
    /**
     * Destroys a session with the given launch ID.
     */
    destroy(launchId) {
        var _a;
        const session = this.sessions.get(launchId);
        (_a = session) === null || _a === void 0 ? void 0 : _a.dispose();
        this.sessions.delete(launchId);
    }
    /**
     * @inheritdoc
     */
    dispose() {
        for (const session of this.sessions.values()) {
            session.dispose();
        }
        this.sessions.clear();
    }
    addChildSocket(session, params) {
        const socket = new net_1.Socket();
        const [host, port] = params.proxyUri.split(':');
        socket.connect({ port: Number(port), host });
        session.attachSocket(socket);
    }
    addChildBrowser(session, params) {
        return __awaiter(this, void 0, void 0, function* () {
            const browser = yield this.spawn.launch(params);
            session.attachChild(browser);
        });
    }
}
exports.SessionManager = SessionManager;


/***/ }),
/* 12 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const vscode_1 = __webpack_require__(5);
const zlib_1 = __webpack_require__(13);
/**
 * The Session manages the lifecycle for a single top-level browser debug sesssion.
 */
class Session {
    constructor() {
        this.errorEmitter = new vscode_1.EventEmitter();
        this.onError = this.errorEmitter.event;
        this.closeEmitter = new vscode_1.EventEmitter();
        this.onClose = this.closeEmitter.event;
        this.disposed = false;
        this.onClose(() => this.dispose());
        this.onError(() => this.dispose());
    }
    /**
     * Attaches the socket looping back up to js-debug.
     */
    attachSocket(socket) {
        if (this.disposed) {
            socket.destroy();
            return;
        }
        this.socket = socket;
        socket.on('close', () => this.closeEmitter.fire());
        socket.on('error', err => this.errorEmitter.fire(err));
        this.tryActivate();
    }
    /**
     * Attaches the browser child process.
     */
    attachChild(child) {
        if (this.disposed) {
            child.kill();
            return;
        }
        this.browserProcess = child;
        child.on('close', () => this.closeEmitter.fire());
        child.on('error', err => this.errorEmitter.fire(err));
        this.tryActivate();
    }
    /**
     * @inheritdoc
     */
    dispose() {
        var _a, _b;
        if (!this.disposed) {
            (_a = this.socket) === null || _a === void 0 ? void 0 : _a.destroy();
            (_b = this.browserProcess) === null || _b === void 0 ? void 0 : _b.kill();
            this.disposed = true;
        }
    }
    tryActivate() {
        if (!this.browserProcess || !this.socket) {
            return;
        }
        const cpIn = this.browserProcess.stdio[3];
        const cpOut = this.browserProcess.stdio[4];
        const compressor = zlib_1.createGzip();
        cpOut.pipe(compressor).pipe(this.socket);
        cpOut.on('data', (data) => {
            if (data.includes(0)) {
                compressor.flush(2 /* Z_SYNC_FLUSH */);
            }
        });
        this.socket.pipe(zlib_1.createGunzip()).pipe(cpIn);
    }
}
exports.Session = Session;


/***/ }),
/* 13 */
/***/ (function(module, exports) {

module.exports = require("zlib");

/***/ }),
/* 14 */
/***/ (function(module, exports) {

module.exports = require("net");

/***/ }),
/* 15 */
/***/ (function(module, exports, __webpack_require__) {

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
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const errors_1 = __webpack_require__(16);
const vscode_js_debug_browsers_1 = __webpack_require__(17);
const fs_1 = __webpack_require__(1);
const execa_1 = __importDefault(__webpack_require__(24));
const child_process_1 = __webpack_require__(4);
const nls = __importStar(__webpack_require__(63));
const path_1 = __webpack_require__(0);
const localize = nls.loadMessageBundle();
class BrowserSpawner {
    constructor(storagePath) {
        this.storagePath = storagePath;
        this.finders = {
            edge: new vscode_js_debug_browsers_1.EdgeBrowserFinder(process.env, fs_1.promises, execa_1.default),
            chrome: new vscode_js_debug_browsers_1.ChromeBrowserFinder(process.env, fs_1.promises, execa_1.default),
        };
    }
    findBrowserPath(type, runtimeExecutable) {
        var _a;
        return __awaiter(this, void 0, void 0, function* () {
            if (!vscode_js_debug_browsers_1.isQuality(runtimeExecutable)) {
                return runtimeExecutable;
            }
            if (!this.finders.hasOwnProperty(type)) {
                throw new errors_1.UserError(`Browser type "${type}" is not supported.`);
            }
            const available = yield this.finders[type].findAll();
            const resolved = (_a = available.find(r => r.quality === runtimeExecutable)) === null || _a === void 0 ? void 0 : _a.path;
            if (!resolved) {
                if (runtimeExecutable === "stable" /* Stable */ && !available.length) {
                    throw new errors_1.UserError(localize('noBrowserInstallFound', 'Unable to find a {0} installation on your system. Try installing it, or providing an absolute path to the browser in the "runtimeExecutable" in your launch.json.', type));
                }
                else {
                    throw new errors_1.UserError(localize('browserVersionNotFound', 'Unable to find {0} version {1}. Available auto-discovered versions are: {2}. You can set the "runtimeExecutable" in your launch.json to one of these, or provide an absolute path to the browser executable.', type, runtimeExecutable, JSON.stringify([...new Set(available)])));
                }
            }
            return resolved;
        });
    }
    getUserDataDir(params) {
        var _a;
        const requested = params.params.userDataDir;
        if (requested === false) {
            return;
        }
        if (requested === true) {
            return path_1.join(this.storagePath, ((_a = params.browserArgs) === null || _a === void 0 ? void 0 : _a.includes('--headless')) ? '.headless-profile' : '.profile');
        }
        return requested;
    }
    /**
     * Launches and returns a child process for the browser specified by the
     * given parameters.
     * @throws UserError if the launch fails
     */
    launch(params) {
        return __awaiter(this, void 0, void 0, function* () {
            const binary = yield this.findBrowserPath(params.type, params.params.runtimeExecutable);
            const args = params.browserArgs.slice();
            const userDataDir = this.getUserDataDir(params);
            // prepend args to not interfere with any positional arguments (e.g. url to open)
            args.unshift('--remote-debugging-pipe');
            if (userDataDir !== undefined) {
                args.unshift(`--user-data-dir=${userDataDir}`);
            }
            // The cwd defaults to the working directory of the remote extension, but
            // this probably won't exist on the local host. If it doesn't just set it
            // to the process' cwd.
            let cwd = params.params.cwd || params.params.webRoot;
            try {
                if (!cwd || !(yield fs_1.promises.stat(cwd)).isDirectory()) {
                    cwd = process.cwd();
                }
            }
            catch (_a) {
                cwd = process.cwd(); // catch ENOENT
            }
            return child_process_1.spawn(binary, args, {
                detached: process.platform !== 'win32',
                env: Object.assign({ ELECTRON_RUN_AS_NODE: undefined }, params.params.env),
                stdio: ['ignore', 'ignore', 'ignore', 'pipe', 'pipe'],
                cwd,
            });
        });
    }
}
exports.BrowserSpawner = BrowserSpawner;


/***/ }),
/* 16 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Expected error meant to be shown to users.
 */
class UserError extends Error {
}
exports.UserError = UserError;


/***/ }),
/* 17 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const darwinChrome_1 = __webpack_require__(18);
const windowsChrome_1 = __webpack_require__(19);
const linuxChrome_1 = __webpack_require__(20);
const windowsEdge_1 = __webpack_require__(21);
const darwinEdge_1 = __webpack_require__(22);
const noop_1 = __webpack_require__(23);
// constructing it this way makes sure we can't forget to add a type:
const qualities = {
    ["canary" /* Canary */]: null,
    ["stable" /* Stable */]: null,
    ["beta" /* Beta */]: null,
    ["dev" /* Dev */]: null,
    ["custom" /* Custom */]: null,
};
/**
 * All known qualities.
 */
exports.allQualities = new Set(Object.keys(qualities));
/**
 * Gets whether given string is a known Quality.
 */
exports.isQuality = (input) => exports.allQualities.has(input);
/**
 * Chrome finder class for the current platform.
 */
exports.ChromeBrowserFinder = process.platform === 'win32'
    ? windowsChrome_1.WindowsChromeBrowserFinder
    : process.platform === 'darwin'
        ? darwinChrome_1.DarwinChromeBrowserFinder
        : linuxChrome_1.LinuxChromeBrowserFinder;
/**
 * Edge finder class for the current platform.
 */
exports.EdgeBrowserFinder = process.platform === 'win32'
    ? windowsEdge_1.WindowsEdgeBrowserFinder
    : process.platform === 'darwin'
        ? darwinEdge_1.DarwinEdgeBrowserFinder
        : noop_1.NoopFinder;
//# sourceMappingURL=index.js.map

/***/ }),
/* 18 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const util_1 = __webpack_require__(2);
const darwinFinderBase_1 = __webpack_require__(6);
/**
 * Finds the Chrome browser on OS X.
 */
class DarwinChromeBrowserFinder extends darwinFinderBase_1.DarwinFinderBase {
    async findAll() {
        const suffixes = ['/Contents/MacOS/Google Chrome Canary', '/Contents/MacOS/Google Chrome'];
        const defaultPaths = ['/Applications/Google Chrome.app'];
        const installations = await this.findLaunchRegisteredApps('google chrome\\( canary\\)\\?.app$', defaultPaths, suffixes);
        return util_1.sort(installations, this.createPriorities([
            {
                name: 'Chrome.app',
                weight: 0,
                quality: "stable" /* Stable */,
            },
            {
                name: 'Chrome Canary.app',
                weight: 1,
                quality: "canary" /* Canary */,
            },
        ]));
    }
    getPreferredPath() {
        return this.env.CHROME_PATH;
    }
}
exports.DarwinChromeBrowserFinder = DarwinChromeBrowserFinder;
//# sourceMappingURL=darwinChrome.js.map

/***/ }),
/* 19 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __webpack_require__(0);
const util_1 = __webpack_require__(2);
/**
 * Finds the Chrome browser on Windows.
 */
class WindowsChromeBrowserFinder {
    constructor(env, fs) {
        this.env = env;
        this.fs = fs;
    }
    async findAll() {
        const sep = path_1.win32.sep;
        const suffixes = [
            {
                name: `${sep}Google${sep}Chrome SxS${sep}Application${sep}chrome.exe`,
                type: "canary" /* Canary */,
            },
            {
                name: `${sep}Google${sep}Chrome${sep}Application${sep}chrome.exe`,
                type: "stable" /* Stable */,
            },
        ];
        const installations = await util_1.findWindowsCandidates(this.env, this.fs, suffixes);
        const customChromePath = await util_1.preferredChromePath(this.fs, this.env);
        if (customChromePath) {
            installations.unshift({ path: customChromePath, quality: "custom" /* Custom */ });
        }
        return installations;
    }
}
exports.WindowsChromeBrowserFinder = WindowsChromeBrowserFinder;
//# sourceMappingURL=windowsChrome.js.map

/***/ }),
/* 20 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __webpack_require__(0);
const util_1 = __webpack_require__(2);
const child_process_1 = __webpack_require__(4);
const os_1 = __webpack_require__(3);
const newLineRegex = /\r?\n/;
/**
 * Finds the Chrome browser on Windows.
 */
class LinuxChromeBrowserFinder {
    constructor(env, fs) {
        this.env = env;
        this.fs = fs;
    }
    async findAll() {
        const installations = new Set();
        // 1. Look into CHROME_PATH env variable
        const customChromePath = await util_1.preferredChromePath(this.fs, this.env);
        if (customChromePath) {
            installations.add(customChromePath);
        }
        // 2. Look into the directories where .desktop are saved on gnome based distro's
        const desktopInstallationFolders = [
            path_1.posix.join(os_1.homedir(), '.local/share/applications/'),
            '/usr/share/applications/',
            '/usr/bin',
        ];
        desktopInstallationFolders.forEach(folder => {
            for (const bin in this.findChromeExecutables(folder)) {
                installations.add(bin);
            }
        });
        // 3. Look for google-chrome & chromium executables by using the which command
        const executables = [
            'google-chrome-unstable',
            'google-chrome-stable',
            'google-chrome',
            'chromium-browser',
            'chromium',
        ];
        await Promise.all(executables.map(async (executable) => {
            try {
                const chromePath = child_process_1.execFileSync('which', [executable], { stdio: 'pipe' })
                    .toString()
                    .split(newLineRegex)[0];
                if (await util_1.canAccess(this.fs, chromePath)) {
                    installations.add(chromePath);
                }
            }
            catch (e) {
                // Not installed.
            }
        }));
        if (!installations.size) {
            throw new Error('The environment variable CHROME_PATH must be set to executable of a build of Chromium version 54.0 or later.');
        }
        const priorities = [
            { regex: /chrome-wrapper$/, weight: 52, quality: "custom" /* Custom */ },
            { regex: /google-chrome-unstable$/, weight: 51, quality: "canary" /* Canary */ },
            { regex: /google-chrome-stable$/, weight: 50, quality: "stable" /* Stable */ },
            { regex: /google-chrome$/, weight: 49, quality: "stable" /* Stable */ },
            { regex: /chromium-browser$/, weight: 48, quality: "custom" /* Custom */ },
            { regex: /chromium$/, weight: 47, quality: "custom" /* Custom */ },
        ];
        if (this.env.CHROME_PATH) {
            priorities.unshift({
                regex: new RegExp(util_1.escapeRegexSpecialChars(this.env.CHROME_PATH)),
                weight: 101,
                quality: "custom" /* Custom */,
            });
        }
        return util_1.sort(installations, priorities);
    }
    findChromeExecutables(folder) {
        const argumentsRegex = /(^[^ ]+).*/; // Take everything up to the first space
        const chromeExecRegex = '^Exec=/.*/(google-chrome|chrome|chromium)-.*';
        const installations = [];
        if (util_1.canAccess(this.fs, folder)) {
            // Output of the grep & print looks like:
            //    /opt/google/chrome/google-chrome --profile-directory
            //    /home/user/Downloads/chrome-linux/chrome-wrapper %U
            // Some systems do not support grep -R so fallback to -r.
            // See https://github.com/GoogleChrome/chrome-launcher/issues/46 for more context.
            let execResult;
            try {
                execResult = child_process_1.execSync(`grep -ER "${chromeExecRegex}" ${folder} | awk -F '=' '{print $2}'`);
            }
            catch (e) {
                execResult = child_process_1.execSync(`grep -Er "${chromeExecRegex}" ${folder} | awk -F '=' '{print $2}'`);
            }
            const execPaths = execResult
                .toString()
                .split(newLineRegex)
                .map(execPath => execPath.replace(argumentsRegex, '$1'));
            execPaths.forEach(execPath => util_1.canAccess(this.fs, execPath) && installations.push(execPath));
        }
        return installations;
    }
}
exports.LinuxChromeBrowserFinder = LinuxChromeBrowserFinder;
//# sourceMappingURL=linuxChrome.js.map

/***/ }),
/* 21 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const path_1 = __webpack_require__(0);
const util_1 = __webpack_require__(2);
/**
 * Finds the Chrome browser on Windows.
 */
class WindowsEdgeBrowserFinder {
    constructor(env, fs) {
        this.env = env;
        this.fs = fs;
    }
    async findAll() {
        const suffixes = [
            {
                name: `${path_1.sep}Microsoft${path_1.sep}Edge SxS${path_1.sep}Application${path_1.sep}msedge.exe`,
                type: "canary" /* Canary */,
            },
            {
                name: `${path_1.sep}Microsoft${path_1.sep}Edge Dev${path_1.sep}Application${path_1.sep}msedge.exe`,
                type: "dev" /* Dev */,
            },
            {
                name: `${path_1.sep}Microsoft${path_1.sep}Edge Beta${path_1.sep}Application${path_1.sep}msedge.exe`,
                type: "beta" /* Beta */,
            },
            {
                name: `${path_1.sep}Microsoft${path_1.sep}Edge${path_1.sep}Application${path_1.sep}msedge.exe`,
                type: "stable" /* Stable */,
            },
        ];
        const installations = await util_1.findWindowsCandidates(this.env, this.fs, suffixes);
        const customEdgePath = await util_1.preferredEdgePath(this.fs, this.env);
        if (customEdgePath) {
            installations.unshift({ path: customEdgePath, quality: "custom" /* Custom */ });
        }
        return installations;
    }
}
exports.WindowsEdgeBrowserFinder = WindowsEdgeBrowserFinder;
//# sourceMappingURL=windowsEdge.js.map

/***/ }),
/* 22 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
const util_1 = __webpack_require__(2);
const darwinFinderBase_1 = __webpack_require__(6);
/**
 * Finds the Edege browser on OS X.
 */
class DarwinEdgeBrowserFinder extends darwinFinderBase_1.DarwinFinderBase {
    async findAll() {
        const suffixes = [
            '/Contents/MacOS/Microsoft Edge Canary',
            '/Contents/MacOS/Microsoft Edge Beta',
            '/Contents/MacOS/Microsoft Edge Dev',
            '/Contents/MacOS/Microsoft Edge',
        ];
        const defaultPaths = ['/Applications/Microsoft Edge.app'];
        const installations = await this.findLaunchRegisteredApps('Microsoft Edge[A-Za-z ]*.app$', defaultPaths, suffixes);
        return util_1.sort(installations, this.createPriorities([
            {
                name: 'Microsoft Edge.app',
                weight: 0,
                quality: "stable" /* Stable */,
            },
            {
                name: 'Microsoft Edge Canary.app',
                weight: 1,
                quality: "canary" /* Canary */,
            },
            {
                name: 'Microsoft Edge Beta.app',
                weight: 2,
                quality: "beta" /* Beta */,
            },
            {
                name: 'Microsoft Edge Dev.app',
                weight: 3,
                quality: "dev" /* Dev */,
            },
        ]));
    }
    getPreferredPath() {
        return this.env.EDGE_PATH;
    }
}
exports.DarwinEdgeBrowserFinder = DarwinEdgeBrowserFinder;
//# sourceMappingURL=darwinEdge.js.map

/***/ }),
/* 23 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * A no-op browser finder used to stub unsupported browsers/platforms (like
 * Edge on Linux).
 */
class NoopFinder {
    findAll() {
        return Promise.resolve([]);
    }
}
exports.NoopFinder = NoopFinder;
//# sourceMappingURL=noop.js.map

/***/ }),
/* 24 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const path = __webpack_require__(0);
const childProcess = __webpack_require__(4);
const crossSpawn = __webpack_require__(25);
const stripFinalNewline = __webpack_require__(38);
const npmRunPath = __webpack_require__(39);
const onetime = __webpack_require__(41);
const makeError = __webpack_require__(43);
const normalizeStdio = __webpack_require__(47);
const {spawnedKill, spawnedCancel, setupTimeout, setExitHandler} = __webpack_require__(48);
const {handleInput, getSpawnedResult, makeAllStream, validateInputSync} = __webpack_require__(53);
const {mergePromise, getSpawnedPromise} = __webpack_require__(61);
const {joinCommand, parseCommand} = __webpack_require__(62);

const DEFAULT_MAX_BUFFER = 1000 * 1000 * 100;

const getEnv = ({env: envOption, extendEnv, preferLocal, localDir, execPath}) => {
	const env = extendEnv ? {...process.env, ...envOption} : envOption;

	if (preferLocal) {
		return npmRunPath.env({env, cwd: localDir, execPath});
	}

	return env;
};

const handleArgs = (file, args, options = {}) => {
	const parsed = crossSpawn._parse(file, args, options);
	file = parsed.command;
	args = parsed.args;
	options = parsed.options;

	options = {
		maxBuffer: DEFAULT_MAX_BUFFER,
		buffer: true,
		stripFinalNewline: true,
		extendEnv: true,
		preferLocal: false,
		localDir: options.cwd || process.cwd(),
		execPath: process.execPath,
		encoding: 'utf8',
		reject: true,
		cleanup: true,
		all: false,
		windowsHide: true,
		...options
	};

	options.env = getEnv(options);

	options.stdio = normalizeStdio(options);

	if (process.platform === 'win32' && path.basename(file, '.exe') === 'cmd') {
		// #116
		args.unshift('/q');
	}

	return {file, args, options, parsed};
};

const handleOutput = (options, value, error) => {
	if (typeof value !== 'string' && !Buffer.isBuffer(value)) {
		// When `execa.sync()` errors, we normalize it to '' to mimic `execa()`
		return error === undefined ? undefined : '';
	}

	if (options.stripFinalNewline) {
		return stripFinalNewline(value);
	}

	return value;
};

const execa = (file, args, options) => {
	const parsed = handleArgs(file, args, options);
	const command = joinCommand(file, args);

	let spawned;
	try {
		spawned = childProcess.spawn(parsed.file, parsed.args, parsed.options);
	} catch (error) {
		// Ensure the returned error is always both a promise and a child process
		const dummySpawned = new childProcess.ChildProcess();
		const errorPromise = Promise.reject(makeError({
			error,
			stdout: '',
			stderr: '',
			all: '',
			command,
			parsed,
			timedOut: false,
			isCanceled: false,
			killed: false
		}));
		return mergePromise(dummySpawned, errorPromise);
	}

	const spawnedPromise = getSpawnedPromise(spawned);
	const timedPromise = setupTimeout(spawned, parsed.options, spawnedPromise);
	const processDone = setExitHandler(spawned, parsed.options, timedPromise);

	const context = {isCanceled: false};

	spawned.kill = spawnedKill.bind(null, spawned.kill.bind(spawned));
	spawned.cancel = spawnedCancel.bind(null, spawned, context);

	const handlePromise = async () => {
		const [{error, exitCode, signal, timedOut}, stdoutResult, stderrResult, allResult] = await getSpawnedResult(spawned, parsed.options, processDone);
		const stdout = handleOutput(parsed.options, stdoutResult);
		const stderr = handleOutput(parsed.options, stderrResult);
		const all = handleOutput(parsed.options, allResult);

		if (error || exitCode !== 0 || signal !== null) {
			const returnedError = makeError({
				error,
				exitCode,
				signal,
				stdout,
				stderr,
				all,
				command,
				parsed,
				timedOut,
				isCanceled: context.isCanceled,
				killed: spawned.killed
			});

			if (!parsed.options.reject) {
				return returnedError;
			}

			throw returnedError;
		}

		return {
			command,
			exitCode: 0,
			stdout,
			stderr,
			all,
			failed: false,
			timedOut: false,
			isCanceled: false,
			killed: false
		};
	};

	const handlePromiseOnce = onetime(handlePromise);

	crossSpawn._enoent.hookChildProcess(spawned, parsed.parsed);

	handleInput(spawned, parsed.options.input);

	spawned.all = makeAllStream(spawned, parsed.options);

	return mergePromise(spawned, handlePromiseOnce);
};

module.exports = execa;

module.exports.sync = (file, args, options) => {
	const parsed = handleArgs(file, args, options);
	const command = joinCommand(file, args);

	validateInputSync(parsed.options);

	let result;
	try {
		result = childProcess.spawnSync(parsed.file, parsed.args, parsed.options);
	} catch (error) {
		throw makeError({
			error,
			stdout: '',
			stderr: '',
			all: '',
			command,
			parsed,
			timedOut: false,
			isCanceled: false,
			killed: false
		});
	}

	const stdout = handleOutput(parsed.options, result.stdout, result.error);
	const stderr = handleOutput(parsed.options, result.stderr, result.error);

	if (result.error || result.status !== 0 || result.signal !== null) {
		const error = makeError({
			stdout,
			stderr,
			error: result.error,
			signal: result.signal,
			exitCode: result.status,
			command,
			parsed,
			timedOut: result.error && result.error.code === 'ETIMEDOUT',
			isCanceled: false,
			killed: result.signal !== null
		});

		if (!parsed.options.reject) {
			return error;
		}

		throw error;
	}

	return {
		command,
		exitCode: 0,
		stdout,
		stderr,
		failed: false,
		timedOut: false,
		isCanceled: false,
		killed: false
	};
};

module.exports.command = (command, options) => {
	const [file, ...args] = parseCommand(command);
	return execa(file, args, options);
};

module.exports.commandSync = (command, options) => {
	const [file, ...args] = parseCommand(command);
	return execa.sync(file, args, options);
};

module.exports.node = (scriptPath, args, options = {}) => {
	if (args && !Array.isArray(args) && typeof args === 'object') {
		options = args;
		args = [];
	}

	const stdio = normalizeStdio.node(options);

	const {nodePath = process.execPath, nodeOptions = process.execArgv} = options;

	return execa(
		nodePath,
		[
			...nodeOptions,
			scriptPath,
			...(Array.isArray(args) ? args : [])
		],
		{
			...options,
			stdin: undefined,
			stdout: undefined,
			stderr: undefined,
			stdio,
			shell: false
		}
	);
};


/***/ }),
/* 25 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const cp = __webpack_require__(4);
const parse = __webpack_require__(26);
const enoent = __webpack_require__(37);

function spawn(command, args, options) {
    // Parse the arguments
    const parsed = parse(command, args, options);

    // Spawn the child process
    const spawned = cp.spawn(parsed.command, parsed.args, parsed.options);

    // Hook into child process "exit" event to emit an error if the command
    // does not exists, see: https://github.com/IndigoUnited/node-cross-spawn/issues/16
    enoent.hookChildProcess(spawned, parsed);

    return spawned;
}

function spawnSync(command, args, options) {
    // Parse the arguments
    const parsed = parse(command, args, options);

    // Spawn the child process
    const result = cp.spawnSync(parsed.command, parsed.args, parsed.options);

    // Analyze if the command does not exist, see: https://github.com/IndigoUnited/node-cross-spawn/issues/16
    result.error = result.error || enoent.verifyENOENTSync(result.status, parsed);

    return result;
}

module.exports = spawn;
module.exports.spawn = spawn;
module.exports.sync = spawnSync;

module.exports._parse = parse;
module.exports._enoent = enoent;


/***/ }),
/* 26 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const path = __webpack_require__(0);
const resolveCommand = __webpack_require__(27);
const escape = __webpack_require__(33);
const readShebang = __webpack_require__(34);

const isWin = process.platform === 'win32';
const isExecutableRegExp = /\.(?:com|exe)$/i;
const isCmdShimRegExp = /node_modules[\\/].bin[\\/][^\\/]+\.cmd$/i;

function detectShebang(parsed) {
    parsed.file = resolveCommand(parsed);

    const shebang = parsed.file && readShebang(parsed.file);

    if (shebang) {
        parsed.args.unshift(parsed.file);
        parsed.command = shebang;

        return resolveCommand(parsed);
    }

    return parsed.file;
}

function parseNonShell(parsed) {
    if (!isWin) {
        return parsed;
    }

    // Detect & add support for shebangs
    const commandFile = detectShebang(parsed);

    // We don't need a shell if the command filename is an executable
    const needsShell = !isExecutableRegExp.test(commandFile);

    // If a shell is required, use cmd.exe and take care of escaping everything correctly
    // Note that `forceShell` is an hidden option used only in tests
    if (parsed.options.forceShell || needsShell) {
        // Need to double escape meta chars if the command is a cmd-shim located in `node_modules/.bin/`
        // The cmd-shim simply calls execute the package bin file with NodeJS, proxying any argument
        // Because the escape of metachars with ^ gets interpreted when the cmd.exe is first called,
        // we need to double escape them
        const needsDoubleEscapeMetaChars = isCmdShimRegExp.test(commandFile);

        // Normalize posix paths into OS compatible paths (e.g.: foo/bar -> foo\bar)
        // This is necessary otherwise it will always fail with ENOENT in those cases
        parsed.command = path.normalize(parsed.command);

        // Escape command & arguments
        parsed.command = escape.command(parsed.command);
        parsed.args = parsed.args.map((arg) => escape.argument(arg, needsDoubleEscapeMetaChars));

        const shellCommand = [parsed.command].concat(parsed.args).join(' ');

        parsed.args = ['/d', '/s', '/c', `"${shellCommand}"`];
        parsed.command = process.env.comspec || 'cmd.exe';
        parsed.options.windowsVerbatimArguments = true; // Tell node's spawn that the arguments are already escaped
    }

    return parsed;
}

function parse(command, args, options) {
    // Normalize arguments, similar to nodejs
    if (args && !Array.isArray(args)) {
        options = args;
        args = null;
    }

    args = args ? args.slice(0) : []; // Clone array to avoid changing the original
    options = Object.assign({}, options); // Clone object to avoid changing the original

    // Build our parsed object
    const parsed = {
        command,
        args,
        options,
        file: undefined,
        original: {
            command,
            args,
        },
    };

    // Delegate further parsing to shell or non-shell
    return options.shell ? parsed : parseNonShell(parsed);
}

module.exports = parse;


/***/ }),
/* 27 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const path = __webpack_require__(0);
const which = __webpack_require__(28);
const pathKey = __webpack_require__(32)();

function resolveCommandAttempt(parsed, withoutPathExt) {
    const cwd = process.cwd();
    const hasCustomCwd = parsed.options.cwd != null;
    // Worker threads do not have process.chdir()
    const shouldSwitchCwd = hasCustomCwd && process.chdir !== undefined;

    // If a custom `cwd` was specified, we need to change the process cwd
    // because `which` will do stat calls but does not support a custom cwd
    if (shouldSwitchCwd) {
        try {
            process.chdir(parsed.options.cwd);
        } catch (err) {
            /* Empty */
        }
    }

    let resolved;

    try {
        resolved = which.sync(parsed.command, {
            path: (parsed.options.env || process.env)[pathKey],
            pathExt: withoutPathExt ? path.delimiter : undefined,
        });
    } catch (e) {
        /* Empty */
    } finally {
        if (shouldSwitchCwd) {
            process.chdir(cwd);
        }
    }

    // If we successfully resolved, ensure that an absolute path is returned
    // Note that when a custom `cwd` was used, we need to resolve to an absolute path based on it
    if (resolved) {
        resolved = path.resolve(hasCustomCwd ? parsed.options.cwd : '', resolved);
    }

    return resolved;
}

function resolveCommand(parsed) {
    return resolveCommandAttempt(parsed) || resolveCommandAttempt(parsed, true);
}

module.exports = resolveCommand;


/***/ }),
/* 28 */
/***/ (function(module, exports, __webpack_require__) {

const isWindows = process.platform === 'win32' ||
    process.env.OSTYPE === 'cygwin' ||
    process.env.OSTYPE === 'msys'

const path = __webpack_require__(0)
const COLON = isWindows ? ';' : ':'
const isexe = __webpack_require__(29)

const getNotFoundError = (cmd) =>
  Object.assign(new Error(`not found: ${cmd}`), { code: 'ENOENT' })

const getPathInfo = (cmd, opt) => {
  const colon = opt.colon || COLON

  // If it has a slash, then we don't bother searching the pathenv.
  // just check the file itself, and that's it.
  const pathEnv = cmd.match(/\//) || isWindows && cmd.match(/\\/) ? ['']
    : (
      [
        // windows always checks the cwd first
        ...(isWindows ? [process.cwd()] : []),
        ...(opt.path || process.env.PATH ||
          /* istanbul ignore next: very unusual */ '').split(colon),
      ]
    )
  const pathExtExe = isWindows
    ? opt.pathExt || process.env.PATHEXT || '.EXE;.CMD;.BAT;.COM'
    : ''
  const pathExt = isWindows ? pathExtExe.split(colon) : ['']

  if (isWindows) {
    if (cmd.indexOf('.') !== -1 && pathExt[0] !== '')
      pathExt.unshift('')
  }

  return {
    pathEnv,
    pathExt,
    pathExtExe,
  }
}

const which = (cmd, opt, cb) => {
  if (typeof opt === 'function') {
    cb = opt
    opt = {}
  }
  if (!opt)
    opt = {}

  const { pathEnv, pathExt, pathExtExe } = getPathInfo(cmd, opt)
  const found = []

  const step = i => new Promise((resolve, reject) => {
    if (i === pathEnv.length)
      return opt.all && found.length ? resolve(found)
        : reject(getNotFoundError(cmd))

    const ppRaw = pathEnv[i]
    const pathPart = /^".*"$/.test(ppRaw) ? ppRaw.slice(1, -1) : ppRaw

    const pCmd = path.join(pathPart, cmd)
    const p = !pathPart && /^\.[\\\/]/.test(cmd) ? cmd.slice(0, 2) + pCmd
      : pCmd

    resolve(subStep(p, i, 0))
  })

  const subStep = (p, i, ii) => new Promise((resolve, reject) => {
    if (ii === pathExt.length)
      return resolve(step(i + 1))
    const ext = pathExt[ii]
    isexe(p + ext, { pathExt: pathExtExe }, (er, is) => {
      if (!er && is) {
        if (opt.all)
          found.push(p + ext)
        else
          return resolve(p + ext)
      }
      return resolve(subStep(p, i, ii + 1))
    })
  })

  return cb ? step(0).then(res => cb(null, res), cb) : step(0)
}

const whichSync = (cmd, opt) => {
  opt = opt || {}

  const { pathEnv, pathExt, pathExtExe } = getPathInfo(cmd, opt)
  const found = []

  for (let i = 0; i < pathEnv.length; i ++) {
    const ppRaw = pathEnv[i]
    const pathPart = /^".*"$/.test(ppRaw) ? ppRaw.slice(1, -1) : ppRaw

    const pCmd = path.join(pathPart, cmd)
    const p = !pathPart && /^\.[\\\/]/.test(cmd) ? cmd.slice(0, 2) + pCmd
      : pCmd

    for (let j = 0; j < pathExt.length; j ++) {
      const cur = p + pathExt[j]
      try {
        const is = isexe.sync(cur, { pathExt: pathExtExe })
        if (is) {
          if (opt.all)
            found.push(cur)
          else
            return cur
        }
      } catch (ex) {}
    }
  }

  if (opt.all && found.length)
    return found

  if (opt.nothrow)
    return null

  throw getNotFoundError(cmd)
}

module.exports = which
which.sync = whichSync


/***/ }),
/* 29 */
/***/ (function(module, exports, __webpack_require__) {

var fs = __webpack_require__(1)
var core
if (process.platform === 'win32' || global.TESTING_WINDOWS) {
  core = __webpack_require__(30)
} else {
  core = __webpack_require__(31)
}

module.exports = isexe
isexe.sync = sync

function isexe (path, options, cb) {
  if (typeof options === 'function') {
    cb = options
    options = {}
  }

  if (!cb) {
    if (typeof Promise !== 'function') {
      throw new TypeError('callback not provided')
    }

    return new Promise(function (resolve, reject) {
      isexe(path, options || {}, function (er, is) {
        if (er) {
          reject(er)
        } else {
          resolve(is)
        }
      })
    })
  }

  core(path, options || {}, function (er, is) {
    // ignore EACCES because that just means we aren't allowed to run it
    if (er) {
      if (er.code === 'EACCES' || options && options.ignoreErrors) {
        er = null
        is = false
      }
    }
    cb(er, is)
  })
}

function sync (path, options) {
  // my kingdom for a filtered catch
  try {
    return core.sync(path, options || {})
  } catch (er) {
    if (options && options.ignoreErrors || er.code === 'EACCES') {
      return false
    } else {
      throw er
    }
  }
}


/***/ }),
/* 30 */
/***/ (function(module, exports, __webpack_require__) {

module.exports = isexe
isexe.sync = sync

var fs = __webpack_require__(1)

function checkPathExt (path, options) {
  var pathext = options.pathExt !== undefined ?
    options.pathExt : process.env.PATHEXT

  if (!pathext) {
    return true
  }

  pathext = pathext.split(';')
  if (pathext.indexOf('') !== -1) {
    return true
  }
  for (var i = 0; i < pathext.length; i++) {
    var p = pathext[i].toLowerCase()
    if (p && path.substr(-p.length).toLowerCase() === p) {
      return true
    }
  }
  return false
}

function checkStat (stat, path, options) {
  if (!stat.isSymbolicLink() && !stat.isFile()) {
    return false
  }
  return checkPathExt(path, options)
}

function isexe (path, options, cb) {
  fs.stat(path, function (er, stat) {
    cb(er, er ? false : checkStat(stat, path, options))
  })
}

function sync (path, options) {
  return checkStat(fs.statSync(path), path, options)
}


/***/ }),
/* 31 */
/***/ (function(module, exports, __webpack_require__) {

module.exports = isexe
isexe.sync = sync

var fs = __webpack_require__(1)

function isexe (path, options, cb) {
  fs.stat(path, function (er, stat) {
    cb(er, er ? false : checkStat(stat, options))
  })
}

function sync (path, options) {
  return checkStat(fs.statSync(path), options)
}

function checkStat (stat, options) {
  return stat.isFile() && checkMode(stat, options)
}

function checkMode (stat, options) {
  var mod = stat.mode
  var uid = stat.uid
  var gid = stat.gid

  var myUid = options.uid !== undefined ?
    options.uid : process.getuid && process.getuid()
  var myGid = options.gid !== undefined ?
    options.gid : process.getgid && process.getgid()

  var u = parseInt('100', 8)
  var g = parseInt('010', 8)
  var o = parseInt('001', 8)
  var ug = u | g

  var ret = (mod & o) ||
    (mod & g) && gid === myGid ||
    (mod & u) && uid === myUid ||
    (mod & ug) && myUid === 0

  return ret
}


/***/ }),
/* 32 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const pathKey = (options = {}) => {
	const environment = options.env || process.env;
	const platform = options.platform || process.platform;

	if (platform !== 'win32') {
		return 'PATH';
	}

	return Object.keys(environment).reverse().find(key => key.toUpperCase() === 'PATH') || 'Path';
};

module.exports = pathKey;
// TODO: Remove this for the next major release
module.exports.default = pathKey;


/***/ }),
/* 33 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


// See http://www.robvanderwoude.com/escapechars.php
const metaCharsRegExp = /([()\][%!^"`<>&|;, *?])/g;

function escapeCommand(arg) {
    // Escape meta chars
    arg = arg.replace(metaCharsRegExp, '^$1');

    return arg;
}

function escapeArgument(arg, doubleEscapeMetaChars) {
    // Convert to string
    arg = `${arg}`;

    // Algorithm below is based on https://qntm.org/cmd

    // Sequence of backslashes followed by a double quote:
    // double up all the backslashes and escape the double quote
    arg = arg.replace(/(\\*)"/g, '$1$1\\"');

    // Sequence of backslashes followed by the end of the string
    // (which will become a double quote later):
    // double up all the backslashes
    arg = arg.replace(/(\\*)$/, '$1$1');

    // All other backslashes occur literally

    // Quote the whole thing:
    arg = `"${arg}"`;

    // Escape meta chars
    arg = arg.replace(metaCharsRegExp, '^$1');

    // Double escape meta chars if necessary
    if (doubleEscapeMetaChars) {
        arg = arg.replace(metaCharsRegExp, '^$1');
    }

    return arg;
}

module.exports.command = escapeCommand;
module.exports.argument = escapeArgument;


/***/ }),
/* 34 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const fs = __webpack_require__(1);
const shebangCommand = __webpack_require__(35);

function readShebang(command) {
    // Read the first 150 bytes from the file
    const size = 150;
    const buffer = Buffer.alloc(size);

    let fd;

    try {
        fd = fs.openSync(command, 'r');
        fs.readSync(fd, buffer, 0, size, 0);
        fs.closeSync(fd);
    } catch (e) { /* Empty */ }

    // Attempt to extract shebang (null is returned if not a shebang)
    return shebangCommand(buffer.toString());
}

module.exports = readShebang;


/***/ }),
/* 35 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const shebangRegex = __webpack_require__(36);

module.exports = (string = '') => {
	const match = string.match(shebangRegex);

	if (!match) {
		return null;
	}

	const [path, argument] = match[0].replace(/#! ?/, '').split(' ');
	const binary = path.split('/').pop();

	if (binary === 'env') {
		return argument;
	}

	return argument ? `${binary} ${argument}` : binary;
};


/***/ }),
/* 36 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

module.exports = /^#!(.*)/;


/***/ }),
/* 37 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const isWin = process.platform === 'win32';

function notFoundError(original, syscall) {
    return Object.assign(new Error(`${syscall} ${original.command} ENOENT`), {
        code: 'ENOENT',
        errno: 'ENOENT',
        syscall: `${syscall} ${original.command}`,
        path: original.command,
        spawnargs: original.args,
    });
}

function hookChildProcess(cp, parsed) {
    if (!isWin) {
        return;
    }

    const originalEmit = cp.emit;

    cp.emit = function (name, arg1) {
        // If emitting "exit" event and exit code is 1, we need to check if
        // the command exists and emit an "error" instead
        // See https://github.com/IndigoUnited/node-cross-spawn/issues/16
        if (name === 'exit') {
            const err = verifyENOENT(arg1, parsed, 'spawn');

            if (err) {
                return originalEmit.call(cp, 'error', err);
            }
        }

        return originalEmit.apply(cp, arguments); // eslint-disable-line prefer-rest-params
    };
}

function verifyENOENT(status, parsed) {
    if (isWin && status === 1 && !parsed.file) {
        return notFoundError(parsed.original, 'spawn');
    }

    return null;
}

function verifyENOENTSync(status, parsed) {
    if (isWin && status === 1 && !parsed.file) {
        return notFoundError(parsed.original, 'spawnSync');
    }

    return null;
}

module.exports = {
    hookChildProcess,
    verifyENOENT,
    verifyENOENTSync,
    notFoundError,
};


/***/ }),
/* 38 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


module.exports = input => {
	const LF = typeof input === 'string' ? '\n' : '\n'.charCodeAt();
	const CR = typeof input === 'string' ? '\r' : '\r'.charCodeAt();

	if (input[input.length - 1] === LF) {
		input = input.slice(0, input.length - 1);
	}

	if (input[input.length - 1] === CR) {
		input = input.slice(0, input.length - 1);
	}

	return input;
};


/***/ }),
/* 39 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const path = __webpack_require__(0);
const pathKey = __webpack_require__(40);

const npmRunPath = options => {
	options = {
		cwd: process.cwd(),
		path: process.env[pathKey()],
		execPath: process.execPath,
		...options
	};

	let previous;
	let cwdPath = path.resolve(options.cwd);
	const result = [];

	while (previous !== cwdPath) {
		result.push(path.join(cwdPath, 'node_modules/.bin'));
		previous = cwdPath;
		cwdPath = path.resolve(cwdPath, '..');
	}

	// Ensure the running `node` binary is used
	const execPathDir = path.resolve(options.cwd, options.execPath, '..');
	result.push(execPathDir);

	return result.concat(options.path).join(path.delimiter);
};

module.exports = npmRunPath;
// TODO: Remove this for the next major release
module.exports.default = npmRunPath;

module.exports.env = options => {
	options = {
		env: process.env,
		...options
	};

	const env = {...options.env};
	const path = pathKey({env});

	options.path = env[path];
	env[path] = module.exports(options);

	return env;
};


/***/ }),
/* 40 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const pathKey = (options = {}) => {
	const environment = options.env || process.env;
	const platform = options.platform || process.platform;

	if (platform !== 'win32') {
		return 'PATH';
	}

	return Object.keys(environment).reverse().find(key => key.toUpperCase() === 'PATH') || 'Path';
};

module.exports = pathKey;
// TODO: Remove this for the next major release
module.exports.default = pathKey;


/***/ }),
/* 41 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const mimicFn = __webpack_require__(42);

const calledFunctions = new WeakMap();

const oneTime = (fn, options = {}) => {
	if (typeof fn !== 'function') {
		throw new TypeError('Expected a function');
	}

	let ret;
	let isCalled = false;
	let callCount = 0;
	const functionName = fn.displayName || fn.name || '<anonymous>';

	const onetime = function (...args) {
		calledFunctions.set(onetime, ++callCount);

		if (isCalled) {
			if (options.throw === true) {
				throw new Error(`Function \`${functionName}\` can only be called once`);
			}

			return ret;
		}

		isCalled = true;
		ret = fn.apply(this, args);
		fn = null;

		return ret;
	};

	mimicFn(onetime, fn);
	calledFunctions.set(onetime, callCount);

	return onetime;
};

module.exports = oneTime;
// TODO: Remove this for the next major release
module.exports.default = oneTime;

module.exports.callCount = fn => {
	if (!calledFunctions.has(fn)) {
		throw new Error(`The given function \`${fn.name}\` is not wrapped by the \`onetime\` package`);
	}

	return calledFunctions.get(fn);
};


/***/ }),
/* 42 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const mimicFn = (to, from) => {
	for (const prop of Reflect.ownKeys(from)) {
		Object.defineProperty(to, prop, Object.getOwnPropertyDescriptor(from, prop));
	}

	return to;
};

module.exports = mimicFn;
// TODO: Remove this for the next major release
module.exports.default = mimicFn;


/***/ }),
/* 43 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const {signalsByName} = __webpack_require__(44);

const getErrorPrefix = ({timedOut, timeout, errorCode, signal, signalDescription, exitCode, isCanceled}) => {
	if (timedOut) {
		return `timed out after ${timeout} milliseconds`;
	}

	if (isCanceled) {
		return 'was canceled';
	}

	if (errorCode !== undefined) {
		return `failed with ${errorCode}`;
	}

	if (signal !== undefined) {
		return `was killed with ${signal} (${signalDescription})`;
	}

	if (exitCode !== undefined) {
		return `failed with exit code ${exitCode}`;
	}

	return 'failed';
};

const makeError = ({
	stdout,
	stderr,
	all,
	error,
	signal,
	exitCode,
	command,
	timedOut,
	isCanceled,
	killed,
	parsed: {options: {timeout}}
}) => {
	// `signal` and `exitCode` emitted on `spawned.on('exit')` event can be `null`.
	// We normalize them to `undefined`
	exitCode = exitCode === null ? undefined : exitCode;
	signal = signal === null ? undefined : signal;
	const signalDescription = signal === undefined ? undefined : signalsByName[signal].description;

	const errorCode = error && error.code;

	const prefix = getErrorPrefix({timedOut, timeout, errorCode, signal, signalDescription, exitCode, isCanceled});
	const execaMessage = `Command ${prefix}: ${command}`;
	const shortMessage = error instanceof Error ? `${execaMessage}\n${error.message}` : execaMessage;
	const message = [shortMessage, stderr, stdout].filter(Boolean).join('\n');

	if (error instanceof Error) {
		error.originalMessage = error.message;
		error.message = message;
	} else {
		error = new Error(message);
	}

	error.shortMessage = shortMessage;
	error.command = command;
	error.exitCode = exitCode;
	error.signal = signal;
	error.signalDescription = signalDescription;
	error.stdout = stdout;
	error.stderr = stderr;

	if (all !== undefined) {
		error.all = all;
	}

	if ('bufferedData' in error) {
		delete error.bufferedData;
	}

	error.failed = true;
	error.timedOut = Boolean(timedOut);
	error.isCanceled = isCanceled;
	error.killed = killed && !timedOut;

	return error;
};

module.exports = makeError;


/***/ }),
/* 44 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
Object.defineProperty(exports,"__esModule",{value:true});exports.signalsByNumber=exports.signalsByName=void 0;var _os=__webpack_require__(3);

var _signals=__webpack_require__(45);
var _realtime=__webpack_require__(7);



const getSignalsByName=function(){
const signals=(0,_signals.getSignals)();
return signals.reduce(getSignalByName,{});
};

const getSignalByName=function(
signalByNameMemo,
{name,number,description,supported,action,forced,standard})
{
return{
...signalByNameMemo,
[name]:{name,number,description,supported,action,forced,standard}};

};

const signalsByName=getSignalsByName();exports.signalsByName=signalsByName;




const getSignalsByNumber=function(){
const signals=(0,_signals.getSignals)();
const length=_realtime.SIGRTMAX+1;
const signalsA=Array.from({length},(value,number)=>
getSignalByNumber(number,signals));

return Object.assign({},...signalsA);
};

const getSignalByNumber=function(number,signals){
const signal=findSignalByNumber(number,signals);

if(signal===undefined){
return{};
}

const{name,description,supported,action,forced,standard}=signal;
return{
[number]:{
name,
number,
description,
supported,
action,
forced,
standard}};


};



const findSignalByNumber=function(number,signals){
const signal=signals.find(({name})=>_os.constants.signals[name]===number);

if(signal!==undefined){
return signal;
}

return signals.find(signalA=>signalA.number===number);
};

const signalsByNumber=getSignalsByNumber();exports.signalsByNumber=signalsByNumber;
//# sourceMappingURL=main.js.map

/***/ }),
/* 45 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
Object.defineProperty(exports,"__esModule",{value:true});exports.getSignals=void 0;var _os=__webpack_require__(3);

var _core=__webpack_require__(46);
var _realtime=__webpack_require__(7);



const getSignals=function(){
const realtimeSignals=(0,_realtime.getRealtimeSignals)();
const signals=[..._core.SIGNALS,...realtimeSignals].map(normalizeSignal);
return signals;
};exports.getSignals=getSignals;







const normalizeSignal=function({
name,
number:defaultNumber,
description,
action,
forced=false,
standard})
{
const{
signals:{[name]:constantSignal}}=
_os.constants;
const supported=constantSignal!==undefined;
const number=supported?constantSignal:defaultNumber;
return{name,number,description,supported,action,forced,standard};
};
//# sourceMappingURL=signals.js.map

/***/ }),
/* 46 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
Object.defineProperty(exports,"__esModule",{value:true});exports.SIGNALS=void 0;

const SIGNALS=[
{
name:"SIGHUP",
number:1,
action:"terminate",
description:"Terminal closed",
standard:"posix"},

{
name:"SIGINT",
number:2,
action:"terminate",
description:"User interruption with CTRL-C",
standard:"ansi"},

{
name:"SIGQUIT",
number:3,
action:"core",
description:"User interruption with CTRL-\\",
standard:"posix"},

{
name:"SIGILL",
number:4,
action:"core",
description:"Invalid machine instruction",
standard:"ansi"},

{
name:"SIGTRAP",
number:5,
action:"core",
description:"Debugger breakpoint",
standard:"posix"},

{
name:"SIGABRT",
number:6,
action:"core",
description:"Aborted",
standard:"ansi"},

{
name:"SIGIOT",
number:6,
action:"core",
description:"Aborted",
standard:"bsd"},

{
name:"SIGBUS",
number:7,
action:"core",
description:
"Bus error due to misaligned, non-existing address or paging error",
standard:"bsd"},

{
name:"SIGEMT",
number:7,
action:"terminate",
description:"Command should be emulated but is not implemented",
standard:"other"},

{
name:"SIGFPE",
number:8,
action:"core",
description:"Floating point arithmetic error",
standard:"ansi"},

{
name:"SIGKILL",
number:9,
action:"terminate",
description:"Forced termination",
standard:"posix",
forced:true},

{
name:"SIGUSR1",
number:10,
action:"terminate",
description:"Application-specific signal",
standard:"posix"},

{
name:"SIGSEGV",
number:11,
action:"core",
description:"Segmentation fault",
standard:"ansi"},

{
name:"SIGUSR2",
number:12,
action:"terminate",
description:"Application-specific signal",
standard:"posix"},

{
name:"SIGPIPE",
number:13,
action:"terminate",
description:"Broken pipe or socket",
standard:"posix"},

{
name:"SIGALRM",
number:14,
action:"terminate",
description:"Timeout or timer",
standard:"posix"},

{
name:"SIGTERM",
number:15,
action:"terminate",
description:"Termination",
standard:"ansi"},

{
name:"SIGSTKFLT",
number:16,
action:"terminate",
description:"Stack is empty or overflowed",
standard:"other"},

{
name:"SIGCHLD",
number:17,
action:"ignore",
description:"Child process terminated, paused or unpaused",
standard:"posix"},

{
name:"SIGCLD",
number:17,
action:"ignore",
description:"Child process terminated, paused or unpaused",
standard:"other"},

{
name:"SIGCONT",
number:18,
action:"unpause",
description:"Unpaused",
standard:"posix",
forced:true},

{
name:"SIGSTOP",
number:19,
action:"pause",
description:"Paused",
standard:"posix",
forced:true},

{
name:"SIGTSTP",
number:20,
action:"pause",
description:"Paused using CTRL-Z or \"suspend\"",
standard:"posix"},

{
name:"SIGTTIN",
number:21,
action:"pause",
description:"Background process cannot read terminal input",
standard:"posix"},

{
name:"SIGBREAK",
number:21,
action:"terminate",
description:"User interruption with CTRL-BREAK",
standard:"other"},

{
name:"SIGTTOU",
number:22,
action:"pause",
description:"Background process cannot write to terminal output",
standard:"posix"},

{
name:"SIGURG",
number:23,
action:"ignore",
description:"Socket received out-of-band data",
standard:"bsd"},

{
name:"SIGXCPU",
number:24,
action:"core",
description:"Process timed out",
standard:"bsd"},

{
name:"SIGXFSZ",
number:25,
action:"core",
description:"File too big",
standard:"bsd"},

{
name:"SIGVTALRM",
number:26,
action:"terminate",
description:"Timeout or timer",
standard:"bsd"},

{
name:"SIGPROF",
number:27,
action:"terminate",
description:"Timeout or timer",
standard:"bsd"},

{
name:"SIGWINCH",
number:28,
action:"ignore",
description:"Terminal window size changed",
standard:"bsd"},

{
name:"SIGIO",
number:29,
action:"terminate",
description:"I/O is available",
standard:"other"},

{
name:"SIGPOLL",
number:29,
action:"terminate",
description:"Watched event",
standard:"other"},

{
name:"SIGINFO",
number:29,
action:"ignore",
description:"Request for process information",
standard:"other"},

{
name:"SIGPWR",
number:30,
action:"terminate",
description:"Device running out of power",
standard:"systemv"},

{
name:"SIGSYS",
number:31,
action:"core",
description:"Invalid system call",
standard:"other"},

{
name:"SIGUNUSED",
number:31,
action:"terminate",
description:"Invalid system call",
standard:"other"}];exports.SIGNALS=SIGNALS;
//# sourceMappingURL=core.js.map

/***/ }),
/* 47 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const aliases = ['stdin', 'stdout', 'stderr'];

const hasAlias = opts => aliases.some(alias => opts[alias] !== undefined);

const normalizeStdio = opts => {
	if (!opts) {
		return;
	}

	const {stdio} = opts;

	if (stdio === undefined) {
		return aliases.map(alias => opts[alias]);
	}

	if (hasAlias(opts)) {
		throw new Error(`It's not possible to provide \`stdio\` in combination with one of ${aliases.map(alias => `\`${alias}\``).join(', ')}`);
	}

	if (typeof stdio === 'string') {
		return stdio;
	}

	if (!Array.isArray(stdio)) {
		throw new TypeError(`Expected \`stdio\` to be of type \`string\` or \`Array\`, got \`${typeof stdio}\``);
	}

	const length = Math.max(stdio.length, aliases.length);
	return Array.from({length}, (value, index) => stdio[index]);
};

module.exports = normalizeStdio;

// `ipc` is pushed unless it is already present
module.exports.node = opts => {
	const stdio = normalizeStdio(opts);

	if (stdio === 'ipc') {
		return 'ipc';
	}

	if (stdio === undefined || typeof stdio === 'string') {
		return [stdio, stdio, stdio, 'ipc'];
	}

	if (stdio.includes('ipc')) {
		return stdio;
	}

	return [...stdio, 'ipc'];
};


/***/ }),
/* 48 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const os = __webpack_require__(3);
const onExit = __webpack_require__(49);

const DEFAULT_FORCE_KILL_TIMEOUT = 1000 * 5;

// Monkey-patches `childProcess.kill()` to add `forceKillAfterTimeout` behavior
const spawnedKill = (kill, signal = 'SIGTERM', options = {}) => {
	const killResult = kill(signal);
	setKillTimeout(kill, signal, options, killResult);
	return killResult;
};

const setKillTimeout = (kill, signal, options, killResult) => {
	if (!shouldForceKill(signal, options, killResult)) {
		return;
	}

	const timeout = getForceKillAfterTimeout(options);
	const t = setTimeout(() => {
		kill('SIGKILL');
	}, timeout);

	// Guarded because there's no `.unref()` when `execa` is used in the renderer
	// process in Electron. This cannot be tested since we don't run tests in
	// Electron.
	// istanbul ignore else
	if (t.unref) {
		t.unref();
	}
};

const shouldForceKill = (signal, {forceKillAfterTimeout}, killResult) => {
	return isSigterm(signal) && forceKillAfterTimeout !== false && killResult;
};

const isSigterm = signal => {
	return signal === os.constants.signals.SIGTERM ||
		(typeof signal === 'string' && signal.toUpperCase() === 'SIGTERM');
};

const getForceKillAfterTimeout = ({forceKillAfterTimeout = true}) => {
	if (forceKillAfterTimeout === true) {
		return DEFAULT_FORCE_KILL_TIMEOUT;
	}

	if (!Number.isInteger(forceKillAfterTimeout) || forceKillAfterTimeout < 0) {
		throw new TypeError(`Expected the \`forceKillAfterTimeout\` option to be a non-negative integer, got \`${forceKillAfterTimeout}\` (${typeof forceKillAfterTimeout})`);
	}

	return forceKillAfterTimeout;
};

// `childProcess.cancel()`
const spawnedCancel = (spawned, context) => {
	const killResult = spawned.kill();

	if (killResult) {
		context.isCanceled = true;
	}
};

const timeoutKill = (spawned, signal, reject) => {
	spawned.kill(signal);
	reject(Object.assign(new Error('Timed out'), {timedOut: true, signal}));
};

// `timeout` option handling
const setupTimeout = (spawned, {timeout, killSignal = 'SIGTERM'}, spawnedPromise) => {
	if (timeout === 0 || timeout === undefined) {
		return spawnedPromise;
	}

	if (!Number.isInteger(timeout) || timeout < 0) {
		throw new TypeError(`Expected the \`timeout\` option to be a non-negative integer, got \`${timeout}\` (${typeof timeout})`);
	}

	let timeoutId;
	const timeoutPromise = new Promise((resolve, reject) => {
		timeoutId = setTimeout(() => {
			timeoutKill(spawned, killSignal, reject);
		}, timeout);
	});

	const safeSpawnedPromise = spawnedPromise.finally(() => {
		clearTimeout(timeoutId);
	});

	return Promise.race([timeoutPromise, safeSpawnedPromise]);
};

// `cleanup` option handling
const setExitHandler = async (spawned, {cleanup, detached}, timedPromise) => {
	if (!cleanup || detached) {
		return timedPromise;
	}

	const removeExitHandler = onExit(() => {
		spawned.kill();
	});

	return timedPromise.finally(() => {
		removeExitHandler();
	});
};

module.exports = {
	spawnedKill,
	spawnedCancel,
	setupTimeout,
	setExitHandler
};


/***/ }),
/* 49 */
/***/ (function(module, exports, __webpack_require__) {

// Note: since nyc uses this module to output coverage, any lines
// that are in the direct sync flow of nyc's outputCoverage are
// ignored, since we can never get coverage for them.
var assert = __webpack_require__(50)
var signals = __webpack_require__(51)

var EE = __webpack_require__(52)
/* istanbul ignore if */
if (typeof EE !== 'function') {
  EE = EE.EventEmitter
}

var emitter
if (process.__signal_exit_emitter__) {
  emitter = process.__signal_exit_emitter__
} else {
  emitter = process.__signal_exit_emitter__ = new EE()
  emitter.count = 0
  emitter.emitted = {}
}

// Because this emitter is a global, we have to check to see if a
// previous version of this library failed to enable infinite listeners.
// I know what you're about to say.  But literally everything about
// signal-exit is a compromise with evil.  Get used to it.
if (!emitter.infinite) {
  emitter.setMaxListeners(Infinity)
  emitter.infinite = true
}

module.exports = function (cb, opts) {
  assert.equal(typeof cb, 'function', 'a callback must be provided for exit handler')

  if (loaded === false) {
    load()
  }

  var ev = 'exit'
  if (opts && opts.alwaysLast) {
    ev = 'afterexit'
  }

  var remove = function () {
    emitter.removeListener(ev, cb)
    if (emitter.listeners('exit').length === 0 &&
        emitter.listeners('afterexit').length === 0) {
      unload()
    }
  }
  emitter.on(ev, cb)

  return remove
}

module.exports.unload = unload
function unload () {
  if (!loaded) {
    return
  }
  loaded = false

  signals.forEach(function (sig) {
    try {
      process.removeListener(sig, sigListeners[sig])
    } catch (er) {}
  })
  process.emit = originalProcessEmit
  process.reallyExit = originalProcessReallyExit
  emitter.count -= 1
}

function emit (event, code, signal) {
  if (emitter.emitted[event]) {
    return
  }
  emitter.emitted[event] = true
  emitter.emit(event, code, signal)
}

// { <signal>: <listener fn>, ... }
var sigListeners = {}
signals.forEach(function (sig) {
  sigListeners[sig] = function listener () {
    // If there are no other listeners, an exit is coming!
    // Simplest way: remove us and then re-send the signal.
    // We know that this will kill the process, so we can
    // safely emit now.
    var listeners = process.listeners(sig)
    if (listeners.length === emitter.count) {
      unload()
      emit('exit', null, sig)
      /* istanbul ignore next */
      emit('afterexit', null, sig)
      /* istanbul ignore next */
      process.kill(process.pid, sig)
    }
  }
})

module.exports.signals = function () {
  return signals
}

module.exports.load = load

var loaded = false

function load () {
  if (loaded) {
    return
  }
  loaded = true

  // This is the number of onSignalExit's that are in play.
  // It's important so that we can count the correct number of
  // listeners on signals, and don't wait for the other one to
  // handle it instead of us.
  emitter.count += 1

  signals = signals.filter(function (sig) {
    try {
      process.on(sig, sigListeners[sig])
      return true
    } catch (er) {
      return false
    }
  })

  process.emit = processEmit
  process.reallyExit = processReallyExit
}

var originalProcessReallyExit = process.reallyExit
function processReallyExit (code) {
  process.exitCode = code || 0
  emit('exit', process.exitCode, null)
  /* istanbul ignore next */
  emit('afterexit', process.exitCode, null)
  /* istanbul ignore next */
  originalProcessReallyExit.call(process, process.exitCode)
}

var originalProcessEmit = process.emit
function processEmit (ev, arg) {
  if (ev === 'exit') {
    if (arg !== undefined) {
      process.exitCode = arg
    }
    var ret = originalProcessEmit.apply(this, arguments)
    emit('exit', process.exitCode, null)
    /* istanbul ignore next */
    emit('afterexit', process.exitCode, null)
    return ret
  } else {
    return originalProcessEmit.apply(this, arguments)
  }
}


/***/ }),
/* 50 */
/***/ (function(module, exports) {

module.exports = require("assert");

/***/ }),
/* 51 */
/***/ (function(module, exports) {

// This is not the set of all possible signals.
//
// It IS, however, the set of all signals that trigger
// an exit on either Linux or BSD systems.  Linux is a
// superset of the signal names supported on BSD, and
// the unknown signals just fail to register, so we can
// catch that easily enough.
//
// Don't bother with SIGKILL.  It's uncatchable, which
// means that we can't fire any callbacks anyway.
//
// If a user does happen to register a handler on a non-
// fatal signal like SIGWINCH or something, and then
// exit, it'll end up firing `process.emit('exit')`, so
// the handler will be fired anyway.
//
// SIGBUS, SIGFPE, SIGSEGV and SIGILL, when not raised
// artificially, inherently leave the process in a
// state from which it is not safe to try and enter JS
// listeners.
module.exports = [
  'SIGABRT',
  'SIGALRM',
  'SIGHUP',
  'SIGINT',
  'SIGTERM'
]

if (process.platform !== 'win32') {
  module.exports.push(
    'SIGVTALRM',
    'SIGXCPU',
    'SIGXFSZ',
    'SIGUSR2',
    'SIGTRAP',
    'SIGSYS',
    'SIGQUIT',
    'SIGIOT'
    // should detect profiler and enable/disable accordingly.
    // see #21
    // 'SIGPROF'
  )
}

if (process.platform === 'linux') {
  module.exports.push(
    'SIGIO',
    'SIGPOLL',
    'SIGPWR',
    'SIGSTKFLT',
    'SIGUNUSED'
  )
}


/***/ }),
/* 52 */
/***/ (function(module, exports) {

module.exports = require("events");

/***/ }),
/* 53 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const isStream = __webpack_require__(54);
const getStream = __webpack_require__(55);
const mergeStream = __webpack_require__(60);

// `input` option
const handleInput = (spawned, input) => {
	// Checking for stdin is workaround for https://github.com/nodejs/node/issues/26852
	// TODO: Remove `|| spawned.stdin === undefined` once we drop support for Node.js <=12.2.0
	if (input === undefined || spawned.stdin === undefined) {
		return;
	}

	if (isStream(input)) {
		input.pipe(spawned.stdin);
	} else {
		spawned.stdin.end(input);
	}
};

// `all` interleaves `stdout` and `stderr`
const makeAllStream = (spawned, {all}) => {
	if (!all || (!spawned.stdout && !spawned.stderr)) {
		return;
	}

	const mixed = mergeStream();

	if (spawned.stdout) {
		mixed.add(spawned.stdout);
	}

	if (spawned.stderr) {
		mixed.add(spawned.stderr);
	}

	return mixed;
};

// On failure, `result.stdout|stderr|all` should contain the currently buffered stream
const getBufferedData = async (stream, streamPromise) => {
	if (!stream) {
		return;
	}

	stream.destroy();

	try {
		return await streamPromise;
	} catch (error) {
		return error.bufferedData;
	}
};

const getStreamPromise = (stream, {encoding, buffer, maxBuffer}) => {
	if (!stream || !buffer) {
		return;
	}

	if (encoding) {
		return getStream(stream, {encoding, maxBuffer});
	}

	return getStream.buffer(stream, {maxBuffer});
};

// Retrieve result of child process: exit code, signal, error, streams (stdout/stderr/all)
const getSpawnedResult = async ({stdout, stderr, all}, {encoding, buffer, maxBuffer}, processDone) => {
	const stdoutPromise = getStreamPromise(stdout, {encoding, buffer, maxBuffer});
	const stderrPromise = getStreamPromise(stderr, {encoding, buffer, maxBuffer});
	const allPromise = getStreamPromise(all, {encoding, buffer, maxBuffer: maxBuffer * 2});

	try {
		return await Promise.all([processDone, stdoutPromise, stderrPromise, allPromise]);
	} catch (error) {
		return Promise.all([
			{error, signal: error.signal, timedOut: error.timedOut},
			getBufferedData(stdout, stdoutPromise),
			getBufferedData(stderr, stderrPromise),
			getBufferedData(all, allPromise)
		]);
	}
};

const validateInputSync = ({input}) => {
	if (isStream(input)) {
		throw new TypeError('The `input` option cannot be a stream in sync mode');
	}
};

module.exports = {
	handleInput,
	makeAllStream,
	getSpawnedResult,
	validateInputSync
};



/***/ }),
/* 54 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const isStream = stream =>
	stream !== null &&
	typeof stream === 'object' &&
	typeof stream.pipe === 'function';

isStream.writable = stream =>
	isStream(stream) &&
	stream.writable !== false &&
	typeof stream._write === 'function' &&
	typeof stream._writableState === 'object';

isStream.readable = stream =>
	isStream(stream) &&
	stream.readable !== false &&
	typeof stream._read === 'function' &&
	typeof stream._readableState === 'object';

isStream.duplex = stream =>
	isStream.writable(stream) &&
	isStream.readable(stream);

isStream.transform = stream =>
	isStream.duplex(stream) &&
	typeof stream._transform === 'function' &&
	typeof stream._transformState === 'object';

module.exports = isStream;


/***/ }),
/* 55 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const pump = __webpack_require__(56);
const bufferStream = __webpack_require__(59);

class MaxBufferError extends Error {
	constructor() {
		super('maxBuffer exceeded');
		this.name = 'MaxBufferError';
	}
}

async function getStream(inputStream, options) {
	if (!inputStream) {
		return Promise.reject(new Error('Expected a stream'));
	}

	options = {
		maxBuffer: Infinity,
		...options
	};

	const {maxBuffer} = options;

	let stream;
	await new Promise((resolve, reject) => {
		const rejectPromise = error => {
			if (error) { // A null check
				error.bufferedData = stream.getBufferedValue();
			}

			reject(error);
		};

		stream = pump(inputStream, bufferStream(options), error => {
			if (error) {
				rejectPromise(error);
				return;
			}

			resolve();
		});

		stream.on('data', () => {
			if (stream.getBufferedLength() > maxBuffer) {
				rejectPromise(new MaxBufferError());
			}
		});
	});

	return stream.getBufferedValue();
}

module.exports = getStream;
// TODO: Remove this for the next major release
module.exports.default = getStream;
module.exports.buffer = (stream, options) => getStream(stream, {...options, encoding: 'buffer'});
module.exports.array = (stream, options) => getStream(stream, {...options, array: true});
module.exports.MaxBufferError = MaxBufferError;


/***/ }),
/* 56 */
/***/ (function(module, exports, __webpack_require__) {

var once = __webpack_require__(8)
var eos = __webpack_require__(58)
var fs = __webpack_require__(1) // we only need fs to get the ReadStream and WriteStream prototypes

var noop = function () {}
var ancient = /^v?\.0/.test(process.version)

var isFn = function (fn) {
  return typeof fn === 'function'
}

var isFS = function (stream) {
  if (!ancient) return false // newer node version do not need to care about fs is a special way
  if (!fs) return false // browser
  return (stream instanceof (fs.ReadStream || noop) || stream instanceof (fs.WriteStream || noop)) && isFn(stream.close)
}

var isRequest = function (stream) {
  return stream.setHeader && isFn(stream.abort)
}

var destroyer = function (stream, reading, writing, callback) {
  callback = once(callback)

  var closed = false
  stream.on('close', function () {
    closed = true
  })

  eos(stream, {readable: reading, writable: writing}, function (err) {
    if (err) return callback(err)
    closed = true
    callback()
  })

  var destroyed = false
  return function (err) {
    if (closed) return
    if (destroyed) return
    destroyed = true

    if (isFS(stream)) return stream.close(noop) // use close for fs streams to avoid fd leaks
    if (isRequest(stream)) return stream.abort() // request.destroy just do .end - .abort is what we want

    if (isFn(stream.destroy)) return stream.destroy()

    callback(err || new Error('stream was destroyed'))
  }
}

var call = function (fn) {
  fn()
}

var pipe = function (from, to) {
  return from.pipe(to)
}

var pump = function () {
  var streams = Array.prototype.slice.call(arguments)
  var callback = isFn(streams[streams.length - 1] || noop) && streams.pop() || noop

  if (Array.isArray(streams[0])) streams = streams[0]
  if (streams.length < 2) throw new Error('pump requires two streams per minimum')

  var error
  var destroys = streams.map(function (stream, i) {
    var reading = i < streams.length - 1
    var writing = i > 0
    return destroyer(stream, reading, writing, function (err) {
      if (!error) error = err
      if (err) destroys.forEach(call)
      if (reading) return
      destroys.forEach(call)
      callback(error)
    })
  })

  return streams.reduce(pipe)
}

module.exports = pump


/***/ }),
/* 57 */
/***/ (function(module, exports) {

// Returns a wrapper function that returns a wrapped callback
// The wrapper function should do some stuff, and return a
// presumably different callback function.
// This makes sure that own properties are retained, so that
// decorations and such are not lost along the way.
module.exports = wrappy
function wrappy (fn, cb) {
  if (fn && cb) return wrappy(fn)(cb)

  if (typeof fn !== 'function')
    throw new TypeError('need wrapper function')

  Object.keys(fn).forEach(function (k) {
    wrapper[k] = fn[k]
  })

  return wrapper

  function wrapper() {
    var args = new Array(arguments.length)
    for (var i = 0; i < args.length; i++) {
      args[i] = arguments[i]
    }
    var ret = fn.apply(this, args)
    var cb = args[args.length-1]
    if (typeof ret === 'function' && ret !== cb) {
      Object.keys(cb).forEach(function (k) {
        ret[k] = cb[k]
      })
    }
    return ret
  }
}


/***/ }),
/* 58 */
/***/ (function(module, exports, __webpack_require__) {

var once = __webpack_require__(8);

var noop = function() {};

var isRequest = function(stream) {
	return stream.setHeader && typeof stream.abort === 'function';
};

var isChildProcess = function(stream) {
	return stream.stdio && Array.isArray(stream.stdio) && stream.stdio.length === 3
};

var eos = function(stream, opts, callback) {
	if (typeof opts === 'function') return eos(stream, null, opts);
	if (!opts) opts = {};

	callback = once(callback || noop);

	var ws = stream._writableState;
	var rs = stream._readableState;
	var readable = opts.readable || (opts.readable !== false && stream.readable);
	var writable = opts.writable || (opts.writable !== false && stream.writable);
	var cancelled = false;

	var onlegacyfinish = function() {
		if (!stream.writable) onfinish();
	};

	var onfinish = function() {
		writable = false;
		if (!readable) callback.call(stream);
	};

	var onend = function() {
		readable = false;
		if (!writable) callback.call(stream);
	};

	var onexit = function(exitCode) {
		callback.call(stream, exitCode ? new Error('exited with error code: ' + exitCode) : null);
	};

	var onerror = function(err) {
		callback.call(stream, err);
	};

	var onclose = function() {
		process.nextTick(onclosenexttick);
	};

	var onclosenexttick = function() {
		if (cancelled) return;
		if (readable && !(rs && (rs.ended && !rs.destroyed))) return callback.call(stream, new Error('premature close'));
		if (writable && !(ws && (ws.ended && !ws.destroyed))) return callback.call(stream, new Error('premature close'));
	};

	var onrequest = function() {
		stream.req.on('finish', onfinish);
	};

	if (isRequest(stream)) {
		stream.on('complete', onfinish);
		stream.on('abort', onclose);
		if (stream.req) onrequest();
		else stream.on('request', onrequest);
	} else if (writable && !ws) { // legacy streams
		stream.on('end', onlegacyfinish);
		stream.on('close', onlegacyfinish);
	}

	if (isChildProcess(stream)) stream.on('exit', onexit);

	stream.on('end', onend);
	stream.on('finish', onfinish);
	if (opts.error !== false) stream.on('error', onerror);
	stream.on('close', onclose);

	return function() {
		cancelled = true;
		stream.removeListener('complete', onfinish);
		stream.removeListener('abort', onclose);
		stream.removeListener('request', onrequest);
		if (stream.req) stream.req.removeListener('finish', onfinish);
		stream.removeListener('end', onlegacyfinish);
		stream.removeListener('close', onlegacyfinish);
		stream.removeListener('finish', onfinish);
		stream.removeListener('exit', onexit);
		stream.removeListener('end', onend);
		stream.removeListener('error', onerror);
		stream.removeListener('close', onclose);
	};
};

module.exports = eos;


/***/ }),
/* 59 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const {PassThrough: PassThroughStream} = __webpack_require__(9);

module.exports = options => {
	options = {...options};

	const {array} = options;
	let {encoding} = options;
	const isBuffer = encoding === 'buffer';
	let objectMode = false;

	if (array) {
		objectMode = !(encoding || isBuffer);
	} else {
		encoding = encoding || 'utf8';
	}

	if (isBuffer) {
		encoding = null;
	}

	const stream = new PassThroughStream({objectMode});

	if (encoding) {
		stream.setEncoding(encoding);
	}

	let length = 0;
	const chunks = [];

	stream.on('data', chunk => {
		chunks.push(chunk);

		if (objectMode) {
			length = chunks.length;
		} else {
			length += chunk.length;
		}
	});

	stream.getBufferedValue = () => {
		if (array) {
			return chunks;
		}

		return isBuffer ? Buffer.concat(chunks, length) : chunks.join('');
	};

	stream.getBufferedLength = () => length;

	return stream;
};


/***/ }),
/* 60 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


const { PassThrough } = __webpack_require__(9);

module.exports = function (/*streams...*/) {
  var sources = []
  var output  = new PassThrough({objectMode: true})

  output.setMaxListeners(0)

  output.add = add
  output.isEmpty = isEmpty

  output.on('unpipe', remove)

  Array.prototype.slice.call(arguments).forEach(add)

  return output

  function add (source) {
    if (Array.isArray(source)) {
      source.forEach(add)
      return this
    }

    sources.push(source);
    source.once('end', remove.bind(null, source))
    source.once('error', output.emit.bind(output, 'error'))
    source.pipe(output, {end: false})
    return this
  }

  function isEmpty () {
    return sources.length == 0;
  }

  function remove (source) {
    sources = sources.filter(function (it) { return it !== source })
    if (!sources.length && output.readable) { output.end() }
  }
}


/***/ }),
/* 61 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const mergePromiseProperty = (spawned, promise, property) => {
	// Starting the main `promise` is deferred to avoid consuming streams
	const value = typeof promise === 'function' ?
		(...args) => promise()[property](...args) :
		promise[property].bind(promise);

	Object.defineProperty(spawned, property, {
		value,
		writable: true,
		enumerable: false,
		configurable: true
	});
};

// The return value is a mixin of `childProcess` and `Promise`
const mergePromise = (spawned, promise) => {
	mergePromiseProperty(spawned, promise, 'then');
	mergePromiseProperty(spawned, promise, 'catch');
	mergePromiseProperty(spawned, promise, 'finally');
	return spawned;
};

// Use promises instead of `child_process` events
const getSpawnedPromise = spawned => {
	return new Promise((resolve, reject) => {
		spawned.on('exit', (exitCode, signal) => {
			resolve({exitCode, signal});
		});

		spawned.on('error', error => {
			reject(error);
		});

		if (spawned.stdin) {
			spawned.stdin.on('error', error => {
				reject(error);
			});
		}
	});
};

module.exports = {
	mergePromise,
	getSpawnedPromise
};



/***/ }),
/* 62 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

const SPACES_REGEXP = / +/g;

const joinCommand = (file, args = []) => {
	if (!Array.isArray(args)) {
		return file;
	}

	return [file, ...args].join(' ');
};

// Allow spaces to be escaped by a backslash if not meant as a delimiter
const handleEscaping = (tokens, token, index) => {
	if (index === 0) {
		return [token];
	}

	const previousToken = tokens[tokens.length - 1];

	if (previousToken.endsWith('\\')) {
		return [...tokens.slice(0, -1), `${previousToken.slice(0, -1)} ${token}`];
	}

	return [...tokens, token];
};

// Handle `execa.command()`
const parseCommand = command => {
	return command
		.trim()
		.split(SPACES_REGEXP)
		.reduce(handleEscaping, []);
};

module.exports = {
	joinCommand,
	parseCommand
};


/***/ }),
/* 63 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */

Object.defineProperty(exports, "__esModule", { value: true });
var path = __webpack_require__(0);
var fs = __webpack_require__(1);
var toString = Object.prototype.toString;
function isDefined(value) {
    return typeof value !== 'undefined';
}
function isNumber(value) {
    return toString.call(value) === '[object Number]';
}
function isString(value) {
    return toString.call(value) === '[object String]';
}
function isBoolean(value) {
    return value === true || value === false;
}
function readJsonFileSync(filename) {
    return JSON.parse(fs.readFileSync(filename, 'utf8'));
}
var MessageFormat;
(function (MessageFormat) {
    MessageFormat["file"] = "file";
    MessageFormat["bundle"] = "bundle";
    MessageFormat["both"] = "both";
})(MessageFormat = exports.MessageFormat || (exports.MessageFormat = {}));
var BundleFormat;
(function (BundleFormat) {
    // the nls.bundle format
    BundleFormat["standalone"] = "standalone";
    BundleFormat["languagePack"] = "languagePack";
})(BundleFormat = exports.BundleFormat || (exports.BundleFormat = {}));
var LocalizeInfo;
(function (LocalizeInfo) {
    function is(value) {
        var candidate = value;
        return candidate && isDefined(candidate.key) && isDefined(candidate.comment);
    }
    LocalizeInfo.is = is;
})(LocalizeInfo || (LocalizeInfo = {}));
var resolvedLanguage;
var resolvedBundles;
var options;
var isPseudo;
function initializeSettings() {
    options = { locale: undefined, language: undefined, languagePackSupport: false, cacheLanguageResolution: true, messageFormat: MessageFormat.bundle };
    if (isString(process.env.VSCODE_NLS_CONFIG)) {
        try {
            var vscodeOptions = JSON.parse(process.env.VSCODE_NLS_CONFIG);
            var language = void 0;
            var locale = void 0;
            if (vscodeOptions.availableLanguages) {
                var value = vscodeOptions.availableLanguages['*'];
                if (isString(value)) {
                    language = value;
                }
            }
            if (isString(vscodeOptions.locale)) {
                options.locale = vscodeOptions.locale.toLowerCase();
            }
            if (language === undefined) {
                options.language = options.locale;
            }
            else if (language !== 'en') {
                options.language = language;
            }
            if (isBoolean(vscodeOptions._languagePackSupport)) {
                options.languagePackSupport = vscodeOptions._languagePackSupport;
            }
            if (isString(vscodeOptions._cacheRoot)) {
                options.cacheRoot = vscodeOptions._cacheRoot;
            }
            if (isString(vscodeOptions._languagePackId)) {
                options.languagePackId = vscodeOptions._languagePackId;
            }
            if (isString(vscodeOptions._translationsConfigFile)) {
                options.translationsConfigFile = vscodeOptions._translationsConfigFile;
                try {
                    options.translationsConfig = readJsonFileSync(options.translationsConfigFile);
                }
                catch (error) {
                    // We can't read the translation config file. Mark the cache as corrupted.
                    if (vscodeOptions._corruptedFile) {
                        fs.writeFile(vscodeOptions._corruptedFile, 'corrupted', 'utf8', function (err) {
                            console.error(err);
                        });
                    }
                }
            }
        }
        catch (_a) {
            // Do nothing.
        }
    }
    isPseudo = options.locale === 'pseudo';
    resolvedLanguage = undefined;
    resolvedBundles = Object.create(null);
}
initializeSettings();
function supportsLanguagePack() {
    return options.languagePackSupport === true && options.cacheRoot !== undefined && options.languagePackId !== undefined && options.translationsConfigFile !== undefined
        && options.translationsConfig !== undefined;
}
function format(message, args) {
    var result;
    if (isPseudo) {
        // FF3B and FF3D is the Unicode zenkaku representation for [ and ]
        message = '\uFF3B' + message.replace(/[aouei]/g, '$&$&') + '\uFF3D';
    }
    if (args.length === 0) {
        result = message;
    }
    else {
        result = message.replace(/\{(\d+)\}/g, function (match, rest) {
            var index = rest[0];
            var arg = args[index];
            var replacement = match;
            if (typeof arg === 'string') {
                replacement = arg;
            }
            else if (typeof arg === 'number' || typeof arg === 'boolean' || arg === void 0 || arg === null) {
                replacement = String(arg);
            }
            return replacement;
        });
    }
    return result;
}
function createScopedLocalizeFunction(messages) {
    return function (key, message) {
        var args = [];
        for (var _i = 2; _i < arguments.length; _i++) {
            args[_i - 2] = arguments[_i];
        }
        if (isNumber(key)) {
            if (key >= messages.length) {
                console.error("Broken localize call found. Index out of bounds. Stacktrace is\n: " + new Error('').stack);
                return;
            }
            return format(messages[key], args);
        }
        else {
            if (isString(message)) {
                console.warn("Message " + message + " didn't get externalized correctly.");
                return format(message, args);
            }
            else {
                console.error("Broken localize call found. Stacktrace is\n: " + new Error('').stack);
            }
        }
    };
}
function localize(key, message) {
    var args = [];
    for (var _i = 2; _i < arguments.length; _i++) {
        args[_i - 2] = arguments[_i];
    }
    return format(message, args);
}
function resolveLanguage(file) {
    var resolvedLanguage;
    if (options.cacheLanguageResolution && resolvedLanguage) {
        resolvedLanguage = resolvedLanguage;
    }
    else {
        if (isPseudo || !options.language) {
            resolvedLanguage = '.nls.json';
        }
        else {
            var locale = options.language;
            while (locale) {
                var candidate = '.nls.' + locale + '.json';
                if (fs.existsSync(file + candidate)) {
                    resolvedLanguage = candidate;
                    break;
                }
                else {
                    var index = locale.lastIndexOf('-');
                    if (index > 0) {
                        locale = locale.substring(0, index);
                    }
                    else {
                        resolvedLanguage = '.nls.json';
                        locale = null;
                    }
                }
            }
        }
        if (options.cacheLanguageResolution) {
            resolvedLanguage = resolvedLanguage;
        }
    }
    return file + resolvedLanguage;
}
function findInTheBoxBundle(root) {
    var language = options.language;
    while (language) {
        var candidate = path.join(root, "nls.bundle." + language + ".json");
        if (fs.existsSync(candidate)) {
            return candidate;
        }
        else {
            var index = language.lastIndexOf('-');
            if (index > 0) {
                language = language.substring(0, index);
            }
            else {
                language = undefined;
            }
        }
    }
    // Test if we can reslove the default bundle.
    if (language === undefined) {
        var candidate = path.join(root, 'nls.bundle.json');
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }
    return undefined;
}
function mkdir(directory) {
    try {
        fs.mkdirSync(directory);
    }
    catch (err) {
        if (err.code === 'EEXIST') {
            return;
        }
        else if (err.code === 'ENOENT') {
            var parent = path.dirname(directory);
            if (parent !== directory) {
                mkdir(parent);
                fs.mkdirSync(directory);
            }
        }
        else {
            throw err;
        }
    }
}
function createDefaultNlsBundle(folder) {
    var metaData = readJsonFileSync(path.join(folder, 'nls.metadata.json'));
    var result = Object.create(null);
    for (var module_1 in metaData) {
        var entry = metaData[module_1];
        result[module_1] = entry.messages;
    }
    return result;
}
function createNLSBundle(header, metaDataPath) {
    var languagePackLocation = options.translationsConfig[header.id];
    if (!languagePackLocation) {
        return undefined;
    }
    var languagePack = readJsonFileSync(languagePackLocation).contents;
    var metaData = readJsonFileSync(path.join(metaDataPath, 'nls.metadata.json'));
    var result = Object.create(null);
    for (var module_2 in metaData) {
        var entry = metaData[module_2];
        var translations = languagePack[header.outDir + "/" + module_2];
        if (translations) {
            var resultMessages = [];
            for (var i = 0; i < entry.keys.length; i++) {
                var messageKey = entry.keys[i];
                var key = isString(messageKey) ? messageKey : messageKey.key;
                var translatedMessage = translations[key];
                if (translatedMessage === undefined) {
                    translatedMessage = entry.messages[i];
                }
                resultMessages.push(translatedMessage);
            }
            result[module_2] = resultMessages;
        }
        else {
            result[module_2] = entry.messages;
        }
    }
    return result;
}
function touch(file) {
    var d = new Date();
    fs.utimes(file, d, d, function () {
        // Do nothing. Ignore
    });
}
function cacheBundle(key, bundle) {
    resolvedBundles[key] = bundle;
    return bundle;
}
function loadNlsBundleOrCreateFromI18n(header, bundlePath) {
    var result;
    var bundle = path.join(options.cacheRoot, header.id + "-" + header.hash + ".json");
    var useMemoryOnly = false;
    var writeBundle = false;
    try {
        result = JSON.parse(fs.readFileSync(bundle, { encoding: 'utf8', flag: 'r' }));
        touch(bundle);
        return result;
    }
    catch (err) {
        if (err.code === 'ENOENT') {
            writeBundle = true;
        }
        else if (err instanceof SyntaxError) {
            // We have a syntax error. So no valid JSON. Use
            console.log("Syntax error parsing message bundle: " + err.message + ".");
            fs.unlink(bundle, function (err) {
                if (err) {
                    console.error("Deleting corrupted bundle " + bundle + " failed.");
                }
            });
            useMemoryOnly = true;
        }
        else {
            throw err;
        }
    }
    result = createNLSBundle(header, bundlePath);
    if (!result || useMemoryOnly) {
        return result;
    }
    if (writeBundle) {
        try {
            fs.writeFileSync(bundle, JSON.stringify(result), { encoding: 'utf8', flag: 'wx' });
        }
        catch (err) {
            if (err.code === 'EEXIST') {
                return result;
            }
            throw err;
        }
    }
    return result;
}
function loadDefaultNlsBundle(bundlePath) {
    try {
        return createDefaultNlsBundle(bundlePath);
    }
    catch (err) {
        console.log("Generating default bundle from meta data failed.", err);
        return undefined;
    }
}
function loadNlsBundle(header, bundlePath) {
    var result;
    // Core decided to use a language pack. Do the same in the extension
    if (supportsLanguagePack()) {
        try {
            result = loadNlsBundleOrCreateFromI18n(header, bundlePath);
        }
        catch (err) {
            console.log("Load or create bundle failed ", err);
        }
    }
    if (!result) {
        // No language pack found, but core is running in language pack mode
        // Don't try to use old in the box bundles since the might be stale
        // Fall right back to the default bundle.
        if (options.languagePackSupport) {
            return loadDefaultNlsBundle(bundlePath);
        }
        var candidate = findInTheBoxBundle(bundlePath);
        if (candidate) {
            try {
                return readJsonFileSync(candidate);
            }
            catch (err) {
                console.log("Loading in the box message bundle failed.", err);
            }
        }
        result = loadDefaultNlsBundle(bundlePath);
    }
    return result;
}
function tryFindMetaDataHeaderFile(file) {
    var result;
    var dirname = path.dirname(file);
    while (true) {
        result = path.join(dirname, 'nls.metadata.header.json');
        if (fs.existsSync(result)) {
            break;
        }
        var parent = path.dirname(dirname);
        if (parent === dirname) {
            result = undefined;
            break;
        }
        else {
            dirname = parent;
        }
    }
    return result;
}
function loadMessageBundle(file) {
    if (!file) {
        // No file. We are in dev mode. Return the default
        // localize function.
        return localize;
    }
    // Remove extension since we load json files.
    var ext = path.extname(file);
    if (ext) {
        file = file.substr(0, file.length - ext.length);
    }
    if (options.messageFormat === MessageFormat.both || options.messageFormat === MessageFormat.bundle) {
        var headerFile = tryFindMetaDataHeaderFile(file);
        if (headerFile) {
            var bundlePath = path.dirname(headerFile);
            var bundle = resolvedBundles[bundlePath];
            if (bundle === undefined) {
                try {
                    var header = JSON.parse(fs.readFileSync(headerFile, 'utf8'));
                    try {
                        var nlsBundle = loadNlsBundle(header, bundlePath);
                        bundle = cacheBundle(bundlePath, nlsBundle ? { header: header, nlsBundle: nlsBundle } : null);
                    }
                    catch (err) {
                        console.error('Failed to load nls bundle', err);
                        bundle = cacheBundle(bundlePath, null);
                    }
                }
                catch (err) {
                    console.error('Failed to read header file', err);
                    bundle = cacheBundle(bundlePath, null);
                }
            }
            if (bundle) {
                var module_3 = file.substr(bundlePath.length + 1).replace(/\\/g, '/');
                var messages = bundle.nlsBundle[module_3];
                if (messages === undefined) {
                    console.error("Messages for file " + file + " not found. See console for details.");
                    return function () {
                        return 'Messages not found.';
                    };
                }
                return createScopedLocalizeFunction(messages);
            }
        }
    }
    if (options.messageFormat === MessageFormat.both || options.messageFormat === MessageFormat.file) {
        // Try to load a single file bundle
        try {
            var json = readJsonFileSync(resolveLanguage(file));
            if (Array.isArray(json)) {
                return createScopedLocalizeFunction(json);
            }
            else {
                if (isDefined(json.messages) && isDefined(json.keys)) {
                    return createScopedLocalizeFunction(json.messages);
                }
                else {
                    console.error("String bundle '" + file + "' uses an unsupported format.");
                    return function () {
                        return 'File bundle has unsupported format. See console for details';
                    };
                }
            }
        }
        catch (err) {
            if (err.code !== 'ENOENT') {
                console.error('Failed to load single file bundle', err);
            }
        }
    }
    console.error("Failed to load message bundle for file " + file);
    return function () {
        return 'Failed to load message bundle. See console for details.';
    };
}
exports.loadMessageBundle = loadMessageBundle;
function config(opts) {
    if (opts) {
        if (isString(opts.locale)) {
            options.locale = opts.locale.toLowerCase();
            options.language = options.locale;
            resolvedLanguage = undefined;
            resolvedBundles = Object.create(null);
        }
        if (opts.messageFormat !== undefined) {
            options.messageFormat = opts.messageFormat;
        }
        if (opts.bundleFormat === BundleFormat.standalone && options.languagePackSupport === true) {
            options.languagePackSupport = false;
        }
    }
    isPseudo = options.locale === 'pseudo';
    return loadMessageBundle;
}
exports.config = config;
//# sourceMappingURL=main.js.map

/***/ })
/******/ ]);
//# sourceMappingURL=extension.js.map