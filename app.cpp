#include <curses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "editor.h"
#include "util.h"

#include "explorer.h"
#include "gutter.h"
#include "tabbar.h"
#include "statusbar.h"

#include "extension.h"

#include "json/json.h"
#include "reader.h"

static struct app_t* appInstance = 0;

static std::map<int, int> colorMap;

#define SELECTED_OFFSET 1000

int pairForColor(int colorIdx, bool selected)
{
    if (selected && colorIdx == color_pair_e::NORMAL) {
        return color_pair_e::SELECTED;
    }
    return colorMap[colorIdx + (selected ? SELECTED_OFFSET : 0)];
}

void app_t::setupColors()
{
    colorMap.clear();

    style_t s = theme->styles_for_scope("default");
    std::cout << theme->colorIndices.size() << " colors used" << std::endl;

    int bg = -1;
    int fg = color_info_t::nearest_color_index(250, 250, 250);
    int selBg = color_info_t::nearest_color_index(250, 250, 250);
    int selFg = color_info_t::nearest_color_index(50, 50, 50);
    color_info_t clr;
    theme->theme_color("editor.background", clr);
    if (!clr.is_blank()) {
        // bg = clr.index;
    }

    theme->theme_color("editor.foreground", clr);
    if (!clr.is_blank()) {
        fg = clr.index;
    }
    if (!s.foreground.is_blank()) {
        fg = s.foreground.index;
    }
    theme->theme_color("editor.selectionBackground", clr);
    if (!clr.is_blank()) {
        selBg = clr.index;
    }

    use_default_colors();
    start_color();

    int idx = 32;
    init_pair(color_pair_e::NORMAL, fg, bg);
    init_pair(color_pair_e::SELECTED, selFg, selBg);

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first] = idx;
        init_pair(idx++, it->first, bg);
        it++;
    }

    it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first + SELECTED_OFFSET] = idx;
        init_pair(idx++, it->first, selBg);
        it++;
    }

    applyColors();
}

app_t::app_t()
    : lineWrap(false)
    , statusbar(0)
    , gutter(0)
    , explorer(0)
    , refreshLoop(0)
{
    appInstance = this;

    // simple by default
    showStatusBar = false;
    showTabbar = false;
    showGutter = false;
    showSidebar = false;
}

struct app_t* app_t::instance()
{
    return appInstance;
}

void app_t::iniLog()
{
    FILE* log_file = fopen("/tmp/ashlar.log", "w");
    fclose(log_file);
}

void app_t::log(const char* format, ...)
{
    char string[512] = "";

    va_list args;
    va_start(args, format);
    vsnprintf(string, 255, format, args);
    va_end(args);

    FILE* log_file = fopen("/tmp/ashlar.log", "a");
    if (!log_file) {
        return;
    }
    char* token = strtok(string, "\n");
    while (token != NULL) {
        fprintf(log_file, token);
        fprintf(log_file, "\n");

        token = strtok(NULL, "\n");
    }
    fclose(log_file);
}

void app_t::configure(int argc, char** argv)
{
    //-------------------
    // defaults
    //-------------------
    const char* argTheme = 0;
    const char* defaultTheme = "Monokai";
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            argTheme = argv[i + 1];
        }
    }

    std::string _path = "~/.ashlar/settings.json";

    char* cpath = (char*)malloc(_path.length() + 1 * sizeof(char));
    strcpy(cpath, _path.c_str());
    expand_path((char**)(&cpath));
    const std::string path(cpath);
    free(cpath);

    // Json::
    Json::Value settings = parse::loadJson(path);

    //-------------------
    // initialize extensions
    //-------------------
    if (settings.isMember("extensions_paths")) {
        Json::Value exts = settings["extensions_paths"];
        if (exts.isArray()) {
            for (auto path : exts) {
                load_extensions(path.asString().c_str(), extensions);
            }
        }
    }
    load_extensions("~/.ashlar/extensions/", extensions);

    //-------------------
    // theme
    //-------------------
    if (argTheme) {
        themeName = argTheme;
        theme_ptr tmpTheme = theme_from_name(argTheme, extensions);
        if (tmpTheme && tmpTheme->colorIndices.size()) {
            theme = tmpTheme;
        }
    }
    if (!theme && settings.isMember("theme")) {
        themeName = settings["theme"].asString();
        theme_ptr tmpTheme = theme_from_name(themeName, extensions);
        if (tmpTheme && tmpTheme->colorIndices.size()) {
            theme = tmpTheme;
        }
    }
    if (!theme) {
        themeName = defaultTheme;
        theme = theme_from_name(defaultTheme, extensions);
    }

    //-------------------
    // editor settings
    //-------------------
    if (settings.isMember("word_wrap")) {
        lineWrap = settings["word_wrap"].asBool();
    }
    if (settings.isMember("statusbar")) {
        showStatusBar = settings["statusbar"].asBool();
    }
    if (settings.isMember("gutter")) {
        showGutter = settings["gutter"].asBool();
    }
    if (settings.isMember("sidebar")) {
        showSidebar = settings["sidebar"].asBool();
    }
    if (settings.isMember("tabbar")) {
        showTabbar = settings["tabbar"].asBool();
    }

    statusbar->theme = theme;
    gutter->theme = theme;
    explorer->theme = theme;
}

void app_t::applyColors()
{
    style_t s = theme->styles_for_scope("comment");
    statusbar->colorPair = pairForColor(s.foreground.index, false);
    gutter->colorPair = pairForColor(s.foreground.index, false);
}

editor_ptr app_t::openEditor(std::string path)
{
    log("open: %s", path.c_str());
    for(auto e : editors) {
        if (e->document.fullPath == path) {
            log("reopening existing tab");
            currentEditor = e;
            focused = currentEditor.get();
            return e;
        }
    }
    
    const char* filename = path.c_str();
    editor_ptr editor = std::make_shared<struct editor_t>();
    editor->lang = language_from_file(filename, extensions);
    editor->theme = theme;
    editor->document.open(filename);
    editors.emplace_back(editor);

    currentEditor = editor;
    focused = currentEditor.get();

    tabbar->tabs.clear();
    refresh();
    return editor;
}

void app_t::refresh()
{
    refreshLoop = 2;
}

void app_t::close()
{
    std::vector<editor_ptr>::iterator it = editors.begin();
    while(it != editors.end()) {
        if ((*it)->id == currentEditor->id) {
            editors.erase(it);
            break;
        }
        it++;
    }
    if (editors.size()) {
        currentEditor = editors.front();
        focused = currentEditor.get();
    }
    
    tabbar->tabs.clear();
    refresh();
}