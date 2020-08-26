#ifdef ENABLE_SCRIPTING

extern "C" {
#include "quickjs-libc.h"
#include "quickjs.h"
}

#endif

#include "app.h"
#include "extension.h"
#include "keybinding.h"
#include "scripting.h"
#include "theme.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <streambuf>
#include <string>

static struct scripting_t* scriptingInstance = 0;

#ifdef ENABLE_SCRIPTING
static JSRuntime* rt = 0;
static JSContext* ctx = 0;
#endif

struct scripting_t* scripting_t::instance()
{
    return scriptingInstance;
}

scripting_t::scripting_t()
    : editor(0)
{
}

scripting_t::~scripting_t()
{
#ifdef ENABLE_SCRIPTING
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
#endif
}

#ifdef ENABLE_SCRIPTING
static JSValue js_log(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    int i;
    const char* str;

    for (i = 0; i < argc; i++) {
        str = JS_ToCString(ctx, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        app_t::instance()->log("log: %s", str);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_runFile(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    int i;
    const char* str;

    for (i = 0; i < argc; i++) {
        str = JS_ToCString(ctx, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        scripting_t::instance()->runFile(str);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_command(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    int i;
    const char* str = 0;
    std::string cmdName;
    std::string cmdArgs;

    for (i = 0; i < argc; i++) {
        str = JS_ToCString(ctx, argv[i]);
        if (!str)
            return JS_EXCEPTION;

        if (i == 0)
            cmdName = str;
        if (i == 1)
            cmdArgs = str;

        JS_FreeCString(ctx, str);
    }

    /*
    for (int i = 0;; i++) {
        if (!command_script_map[i].name)
            break;
        if (strcmp(command_script_map[i].name, cmdName.c_str()) == 0) {
            app_t::instance()->log("editor: %s %d", cmdName.c_str(), command_script_map[i].cmd);
            app_t::instance()->commandBuffer.push_back(command_t((command_e)command_script_map[i].cmd, cmdArgs));
            break;
        }
    }
    */

    return JS_UNDEFINED;
}

static JSValue js_theme(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    int i;
    const char* themeName;

    for (i = 0; i < argc; i++) {
        themeName = JS_ToCString(ctx, argv[i]);
        if (!themeName)
            return JS_EXCEPTION;

        struct app_t* app = app_t::instance();
        // app->log("theme %s", themeName);
        theme_ptr tmpTheme = theme_from_name(themeName, app->extensions);
        if (tmpTheme) {
            app->theme = tmpTheme;
            app->setupColors();
            app->applyTheme();
        }

        JS_FreeCString(ctx, themeName);
    }
    return JS_UNDEFINED;
}
#endif

void scripting_t::initialize()
{
#ifdef ENABLE_SCRIPTING
    rt = JS_NewRuntime();
    js_std_init_handlers(rt);
    ctx = JS_NewContextRaw(rt);
    JS_AddIntrinsicBaseObjects(ctx);
    JS_AddIntrinsicEval(ctx);
    js_std_add_helpers(ctx, 0, 0);

    JSValue global_obj, app;

    global_obj = JS_GetGlobalObject(ctx);

    app = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, app, "log", JS_NewCFunction(ctx, js_log, "log", 1));
    JS_SetPropertyStr(ctx, app, "theme", JS_NewCFunction(ctx, js_theme, "theme", 1));
    JS_SetPropertyStr(ctx, app, "command", JS_NewCFunction(ctx, js_command, "command", 1));
    JS_SetPropertyStr(ctx, app, "runFile", JS_NewCFunction(ctx, js_runFile, "runFile", 1));
    JS_SetPropertyStr(ctx, global_obj, "app", app);
    JS_FreeValue(ctx, global_obj);
#endif
    scriptingInstance = this;
}

int scripting_t::runScript(std::string script)
{
#ifdef ENABLE_SCRIPTING
    JSValue ret = JS_Eval(ctx, script.c_str(), script.length(), "<eval>", JS_EVAL_TYPE_GLOBAL);
#endif
    return 0;
}

int scripting_t::runFile(std::string path)
{
#ifdef ENABLE_SCRIPTING
    // js_loadScript(ctx, path.c_str());
    std::ifstream file(path, std::ifstream::in);
    std::string script((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JSValue ret = JS_Eval(ctx, script.c_str(), script.length(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
#endif
    return 0;
}

void scripting_t::update(int frames)
{
#ifdef ENABLE_SCRIPTING
    js_std_loop(ctx);
#endif
}
