#include <iostream>

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

struct scripting_t* scripting_t::instance()
{
    return scriptingInstance;
}

static int command_theme(lua_State* L)
{
    int n = lua_gettop(L);
    char* themeName = 0;
    if (n > 0) {
        themeName = (char*)lua_tostring(L, 1);
    }

    //app_t::instance()->log(">>>%d %d", n, themeName);

    if (themeName) {
        struct app_t* app = app_t::instance();
        // app->log("theme %s", themeName);
        theme_ptr tmpTheme = theme_from_name(themeName, app->extensions);
        if (tmpTheme) {
            app->theme = tmpTheme;
            app->applyColors();
            app->setupColors();
            app->refresh();
        }
    }

    return 0;
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
    ;
    lua_register(L, "theme", command_theme);

    scriptingInstance = this;
}

int scripting_t::runScript(std::string script)
{
    luaL_dostring(L, script.c_str());
    //luaL_dostring(L, "avg, sum = average(10, 20, 30, 40, 50)");
    //luaL_dostring(L, "avg = (2 + 2 + 3 + 5)/4");
    //lua_getglobal(L, "avg");
    //int i = lua_tointeger(L, -1);
    //app_t::instance()->log(">>%d", i);
    // lua_State* L = luaL_newstate();

    //luaL_dostring(L, "a = 10 + 5");
    //lua_getglobal(L, "a");
    //int i = lua_tointeger(L, -1);
    //app_t::instance()->log("%d\n", i);

    //lua_close(L);
    return 0;
}
