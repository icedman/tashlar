#include <iostream>
#include <cstring>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include "app.h"
#include "extension.h"
#include "scripting.h"
#include "theme.h"

static struct scripting_t* scriptingInstance = 0;

static lua_State* L = 0;

typedef int command_exec(lua_State* L);

struct command_lua_map_t {
    char *name;
    int cmd;
} command_lua_map[] = {
    // CMD_UNKNOWN,
    { "cancel", CMD_CANCEL },
    { "cut", CMD_CUT },
    { "copy", CMD_COPY },
    { "paste", CMD_PASTE },
    { "select_word", CMD_SELECT_WORD },

    { "cmd_tab_0", CMD_TAB_0 } ,
    { "cmd_tab_1", CMD_TAB_1 } ,
    { "cmd_tab_2", CMD_TAB_2 } ,
    { "cmd_tab_3", CMD_TAB_3 } ,
    { "cmd_tab_4", CMD_TAB_4 } ,
    { "cmd_tab_5", CMD_TAB_5 } ,
    { "cmd_tab_6", CMD_TAB_6 } ,
    { "cmd_tab_7", CMD_TAB_7 } ,
    { "cmd_tab_8", CMD_TAB_8 } ,
    { "cmd_tab_9", CMD_TAB_9 } ,

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

    // { "open", CMD_OPEN },
    { "save", CMD_SAVE },
    // { "save_as", CMD_SAVE_AS },
    // { "save_copy", CMD_SAVE_COPY },
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


static int command_theme(lua_State* L)
{
    int n = lua_gettop(L);
    char* themeName = 0;
    if (n > 0) {
        themeName = (char*)lua_tostring(L, 1);
    }

    if (themeName) {
        struct app_t* app = app_t::instance();
        // app->log("theme %s", themeName);
        theme_ptr tmpTheme = theme_from_name(themeName, app->extensions);
        if (tmpTheme) {
            app->theme = tmpTheme;
            app->setupColors();
            app->applyColors();
            app->refresh();
        }
    }

    return 0;
}

static int command_app_open(lua_State* L)
{
    int n = lua_gettop(L);
    char* fileName = 0;
    if (n > 0) {
        fileName = (char*)lua_tostring(L, 1);
    }

    if (fileName) {
        struct app_t* app = app_t::instance();
        app->openEditor(fileName);
    }

    return 0;
}

static int command_app_save_as(lua_State* L)
{
    int n = lua_gettop(L);
    char* fileName = 0;
    if (n > 0) {
        fileName = (char*)lua_tostring(L, 1);
    }

    if (fileName) {
        struct app_t* app = app_t::instance();
        app->saveAs(fileName, true);
    }

    return 0;
}


static int command_app_save_copy(lua_State* L)
{
    int n = lua_gettop(L);
    char* fileName = 0;
    if (n > 0) {
        fileName = (char*)lua_tostring(L, 1);
    }

    if (fileName) {
        struct app_t* app = app_t::instance();
        app->saveAs(fileName);
    }

    return 0;
}
    
static int command_editor(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 0) {
        char* cmdName = (char*)lua_tostring(L, 1);
        std::string cmdArgs;
        if (n > 1) {
            char* args = (char*)lua_tostring(L, 2);
            if (args) {
                cmdArgs = args;
            }
        }
    
        for(int i=0;;i++) {
            if (!command_lua_map[i].name) break;
            if (strcmp(command_lua_map[i].name, cmdName) == 0) {
                app_t::instance()->log("editor: %s %d", cmdName, command_lua_map[i].cmd);
                app_t::instance()->commandBuffer.push_back(cmd_t((command_e)command_lua_map[i].cmd, cmdArgs));
            }
        }
    }
    return 0;
}
    
struct scripting_t* scripting_t::instance()
{
    return scriptingInstance;
}

scripting_t::scripting_t()
{
}

scripting_t::~scripting_t()
{
    lua_close(L);
}

void scripting_t::initialize()
{
    L = luaL_newstate();
    lua_register(L, "theme", command_theme);
    lua_register(L, "editor", command_editor);
    lua_register(L, "open", command_app_open);
    lua_register(L, "save_as", command_app_save_as);
    lua_register(L, "save_copy", command_app_save_copy);
    scriptingInstance = this;
}

int scripting_t::runScript(std::string script)
{
    luaL_dostring(L, script.c_str());
    return 0;
}

int scripting_t::runFile(std::string path)
{
    luaL_dofile(L, path.c_str());
    return 0;
}
