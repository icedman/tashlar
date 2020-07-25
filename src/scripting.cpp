
extern "C" {
#include "quickjs-libc.h"
#include "quickjs.h"
}

#include "app.h"
#include "extension.h"
#include "scripting.h"
#include "theme.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <streambuf>
#include <string>

static struct scripting_t* scriptingInstance = 0;

static JSRuntime* rt = 0;
static JSContext* ctx = 0;

struct command_script_map_t {
    char* name;
    int cmd;
} command_script_map[] = {
    // CMD_UNKNOWN,
    { "cancel", CMD_CANCEL },
    { "cut", CMD_CUT },
    { "copy", CMD_COPY },
    { "paste", CMD_PASTE },
    { "select_word", CMD_SELECT_WORD },

    { "cmd_tab_0", CMD_TAB_0 },
    { "cmd_tab_1", CMD_TAB_1 },
    { "cmd_tab_2", CMD_TAB_2 },
    { "cmd_tab_3", CMD_TAB_3 },
    { "cmd_tab_4", CMD_TAB_4 },
    { "cmd_tab_5", CMD_TAB_5 },
    { "cmd_tab_6", CMD_TAB_6 },
    { "cmd_tab_7", CMD_TAB_7 },
    { "cmd_tab_8", CMD_TAB_8 },
    { "cmd_tab_9", CMD_TAB_9 },

    { "indent", CMD_INDENT },
    { "unindent", CMD_UNINDENT },

    { "select_all", CMD_SELECT_ALL },
    { "select_line", CMD_SELECT_LINE },
    { "duplicate_line", CMD_DUPLICATE_LINE },
    { "delete_line", CMD_DELETE_LINE },
    { "move_line_up", CMD_MOVE_LINE_UP },
    { "move_line_down", CMD_MOVE_LINE_DOWN },
    { "split_line", CMD_SPLIT_LINE },

    // CMD_TOGGLE_FOLD,

    { "clear_selection", CMD_CLEAR_SELECTION },
    { "duplicate_selection", CMD_DUPLICATE_SELECTION },
    { "delete_selection", CMD_DELETE_SELECTION },
    { "add_cursor_and_move_up", CMD_ADD_CURSOR_AND_MOVE_UP },
    { "add_cursor_and_move_down", CMD_ADD_CURSOR_AND_MOVE_DOWN },
    { "add_cursor_for_selected_word", CMD_ADD_CURSOR_FOR_SELECTED_WORD },
    { "clear_cursors", CMD_CLEAR_CURSORS },

    { "move_cursor_left", CMD_MOVE_CURSOR_LEFT },
    { "move_cursor_right", CMD_MOVE_CURSOR_RIGHT },
    { "move_cursor_up", CMD_MOVE_CURSOR_UP },
    { "move_cursor_down", CMD_MOVE_CURSOR_DOWN },
    { "move_cursor_next_word", CMD_MOVE_CURSOR_NEXT_WORD },
    { "move_cursor_previous_word", CMD_MOVE_CURSOR_PREVIOUS_WORD },
    { "move_cursor_start_of_line", CMD_MOVE_CURSOR_START_OF_LINE },
    { "move_cursor_end_of_line", CMD_MOVE_CURSOR_END_OF_LINE },
    { "move_cursor_start_of_document", CMD_MOVE_CURSOR_START_OF_DOCUMENT },
    { "move_cursor_end_of_document", CMD_MOVE_CURSOR_END_OF_DOCUMENT },
    { "move_cursor_next_page", CMD_MOVE_CURSOR_NEXT_PAGE },
    { "move_cursor_previous_page", CMD_MOVE_CURSOR_PREVIOUS_PAGE },

    { "move_cursor_left_anchored", CMD_MOVE_CURSOR_LEFT_ANCHORED },
    { "move_cursor_right_anchored", CMD_MOVE_CURSOR_RIGHT_ANCHORED },
    { "move_cursor_up_anchored", CMD_MOVE_CURSOR_UP_ANCHORED },
    { "move_cursor_down_anchored", CMD_MOVE_CURSOR_DOWN_ANCHORED },
    { "move_cursor_next_word_anchored", CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED },
    { "move_cursor_previous_word_anchored", CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED },
    { "move_cursor_start_of_line_anchored", CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED },
    { "move_cursor_end_of_line_anchored", CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED },
    { "move_cursor_start_of_document_anchored", CMD_MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED },
    { "move_cursor_end_of_document_anchored", CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED },
    { "move_cursor_next_page_anchored", CMD_MOVE_CURSOR_NEXT_PAGE_ANCHORED },
    { "move_cursor_previous_page_anchored", CMD_MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED },

    // CMD_FOCUS_WINDOW_LEFT,
    // CMD_FOCUS_WINDOW_RIGHT,
    // CMD_FOCUS_WINDOW_UP,
    // CMD_FOCUS_WINDOW_DOWN,

    // CMD_NEW_TAB,
    { "close_tab", CMD_CLOSE_TAB },

    { "open", CMD_OPEN },
    { "save", CMD_SAVE },
    { "save_as", CMD_SAVE_AS },
    { "save_copy", CMD_SAVE_COPY },
    { "quit", CMD_QUIT },
    { "undo", CMD_UNDO },
    // { "", CMD_REDO },

    // CMD_RESIZE,
    { "tab", CMD_TAB },
    { "enter", CMD_ENTER },
    { "delete", CMD_DELETE },
    { "backspace", CMD_BACKSPACE },
    { "insert", CMD_INSERT },

    // CMD_CYCLE_FOCUS,
    // CMD_TOGGLE_EXPLORER,

    // CMD_POPUP_SEARCH,
    // CMD_POPUP_SEARCH_LINE,
    // CMD_POPUP_COMMANDS,
    // CMD_POPUP_FILES,
    // CMD_POPUP_COMPLETION,

    { "history_snapshot", CMD_HISTORY_SNAPSHOT },

    { NULL, CMD_UNKNOWN }
};

struct scripting_t* scripting_t::instance()
{
    return scriptingInstance;
}

scripting_t::scripting_t()
{
}

scripting_t::~scripting_t()
{
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

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

    for (int i = 0;; i++) {
        if (!command_script_map[i].name)
            break;
        if (strcmp(command_script_map[i].name, cmdName.c_str()) == 0) {
            app_t::instance()->log("editor: %s %d", cmdName.c_str(), command_script_map[i].cmd);
            app_t::instance()->commandBuffer.push_back(command_t((command_e)command_script_map[i].cmd, cmdArgs));
            break;
        }
    }

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
            app->applyColors();
            app->refresh();
        }

        JS_FreeCString(ctx, themeName);
    }
    return JS_UNDEFINED;
}

void scripting_t::initialize()
{
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

    scriptingInstance = this;
}

int scripting_t::runScript(std::string script)
{
    JSValue ret = JS_Eval(ctx, script.c_str(), script.length(), "<eval>", JS_EVAL_TYPE_GLOBAL);
    return 0;
}

int scripting_t::runFile(std::string path)
{
    // js_loadScript(ctx, path.c_str());
    std::ifstream file(path, std::ifstream::in);
    std::string script((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    JSValue ret = JS_Eval(ctx, script.c_str(), script.length(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
    return 0;
}

void scripting_t::update(int frames)
{
    js_std_loop(ctx);
}
