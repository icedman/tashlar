#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>

#include "app.h"
#include "editor.h"
#include "util.h"

#include "explorer.h"
#include "gutter.h"
#include "minimap.h"
#include "statusbar.h"
#include "tabbar.h"

#include "extension.h"

#include "json/json.h"
#include "reader.h"

static struct app_t* appInstance = 0;

static std::map<int, int> colorMap;

#define SELECTED_OFFSET 500

void renderEditor(struct editor_t& editor);

int pairForColor(int colorIdx, bool selected)
{
    if (selected && colorIdx == color_pair_e::NORMAL) {
        return color_pair_e::SELECTED;
    }
    return colorMap[colorIdx + (selected ? SELECTED_OFFSET : 0)];
}

static color_info_t color(int r, int g, int b)
{
    color_info_t c(r, g, b);
    c.index = color_info_t::nearest_color_index(c.red, c.green, c.blue);
    return c;
}

static color_info_t lighter(color_info_t p, int x)
{
    color_info_t c;
    c.red = p.red + x;
    c.green = p.green + x;
    c.blue = p.blue + x;
    if (c.red > 255)
        c.red = 255;
    if (c.green > 255)
        c.green = 255;
    if (c.blue > 255)
        c.blue = 255;
    if (c.red < 0)
        c.red = 0;
    if (c.green < 0)
        c.green = 0;
    if (c.blue < 0)
        c.blue = 0;
    c.index = color_info_t::nearest_color_index(c.red, c.green, c.blue);
    return c;
}

static color_info_t darker(color_info_t c, int x)
{
    return lighter(c, -x);
}

void app_t::setupColors()
{
    colorMap.clear();

    style_t s = theme->styles_for_scope("default");
    std::cout << theme->colorIndices.size() << " colors used" << std::endl;

    color_info_t colorFg = color(250, 250, 250);
    color_info_t colorSelBg = color(250, 250, 250);
    color_info_t colorSelFg = color(50, 50, 50);
    if (theme->colorIndices.find(colorFg.index) == theme->colorIndices.end()) {
        theme->colorIndices.emplace(colorFg.index, colorFg);
    }
    if (theme->colorIndices.find(colorSelBg.index) == theme->colorIndices.end()) {
        theme->colorIndices.emplace(colorSelBg.index, colorSelBg);
    }
    if (theme->colorIndices.find(colorSelFg.index) == theme->colorIndices.end()) {
        theme->colorIndices.emplace(colorSelFg.index, colorSelFg);
    }

    bg = -1;
    fg = colorFg.index;
    selBg = colorSelBg.index;
    selFg = colorSelFg.index;
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

    //----------
    // tree
    //----------
    treeFg = fg;
    treeBg = bg;
    theme->theme_color("list.activeSelectionBackground", clr);
    if (!clr.is_blank()) {
        treeBg = clr.index;
    }
    theme->theme_color("list.activeSelectionForeground", clr);
    if (!clr.is_blank()) {
        treeFg = clr.index;
    }

    //----------
    // tab
    //----------
    theme->theme_color("tab.activeBackground", clr);
    if (!clr.is_blank()) {
        tabBg = clr.index;
    }
    theme->theme_color("tab.activeForeground", clr);
    if (!clr.is_blank()) {
        tabFg = clr.index;
    }
    // theme->theme_color("tab.inactiveBackground", clr);
    // theme->theme_color("tab.inactiveForeground", clr);

    tabHoverFg = fg;
    tabHoverBg = bg;
    theme->theme_color("tab.hoverBackground", clr);
    if (!clr.is_blank()) {
        tabHoverFg = clr.index;
    }
    theme->theme_color("tab.hoverForeground", clr);
    if (!clr.is_blank()) {
        tabHoverBg = clr.index;
    }
    tabActiveBorder = fg;
    theme->theme_color("tab.activeBorderTop", clr);
    if (!clr.is_blank()) {
        tabActiveBorder = clr.index;
    }

    //----------
    // statusbar
    //----------
    // theme->theme_color("statusBar.background", clr);
    // theme->theme_color("statusBar.foreground", clr);

    app_t::instance()->log("%d registered colors", theme->colorIndices.size());

    //---------------
    // build the color pairs
    //---------------
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

void app_t::applyColors()
{
    style_t comment = theme->styles_for_scope("comment");
    statusbar->colorPair = pairForColor(comment.foreground.index, false);
    gutter->colorPair = pairForColor(comment.foreground.index, false);
    gutter->colorPairIndicator = pairForColor(tabActiveBorder, false);
    minimap->colorPair = pairForColor(comment.foreground.index, false);
    minimap->colorPairIndicator = pairForColor(tabActiveBorder, false);

    tabbar->colorPair = pairForColor(comment.foreground.index, false);
    tabbar->colorPairIndicator = pairForColor(tabActiveBorder, false);
    explorer->colorPair = pairForColor(comment.foreground.index, false);
    explorer->colorPairIndicator = pairForColor(tabActiveBorder, false);

    popup->colorPair = pairForColor(comment.foreground.index, false);
    popup->colorPairIndicator = pairForColor(tabActiveBorder, false);

    // tabbar->colorPair = pairForColor(tabFg, false);
    // tabbar->colorPairSelected = pairForColor(tabHoverFg, true);
    // tabbar->colorPairIndicator = pairForColor(tabActiveBorder, false);
    // explorer->colorPair = pairForColor(treeFg, false);
    // explorer->colorPairSelected = pairForColor(treeFg, true);
}

app_t::app_t()
    : lineWrap(false)
    , statusbar(0)
    , gutter(0)
    , explorer(0)
    , refreshLoop(8)
{
    appInstance = this;

    // simple by default
    showStatusBar = false;
    showTabbar = false;
    showGutter = false;
    showSidebar = false;
    showMinimap = false;

    initLog();
}

app_t::~app_t()
{
}

struct app_t* app_t::instance()
{
    return appInstance;
}

void app_t::layout()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;
    for (auto window : windows) {
        window->layout(w.ws_col, w.ws_row);
    }
}

void app_t::render()
{
    renderEditor(*currentEditor);
    for (int i = 0; i < windows.size(); i++) {
        windows[i]->render();
    }
}

bool app_t::processCommand(command_e cmd, char ch)
{
    bool handled = false;
    for (auto window : windows) {
        if ((handled = window->processCommand(cmd, ch))) {
            break;
        }
    }
    return handled;
}

void app_t::initLog()
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
    if (settings.isMember("mini_map")) {
        showMinimap = settings["mini_map"].asBool();
    }
    if (settings.isMember("tab_size")) {
        tabSize = settings["tab_size"].asInt();
    }
    if (tabSize < 2) {
        tabSize = 2;
    }

    minimap->theme = theme;
    statusbar->theme = theme;
    gutter->theme = theme;
    explorer->theme = theme;

    //---------------

    Json::Value file_exclude_patterns = settings["file_exclude_patterns"];
    if (file_exclude_patterns.isArray() && file_exclude_patterns.size()) {
        for (int j = 0; j < file_exclude_patterns.size(); j++) {
            std::string pat = file_exclude_patterns[j].asString();
            excludeFiles.push_back(pat);
        }
    }

    Json::Value folder_exclude_patterns = settings["folder_exclude_patterns"];
    if (folder_exclude_patterns.isArray() && folder_exclude_patterns.size()) {
        for (int j = 0; j < folder_exclude_patterns.size(); j++) {
            std::string pat = folder_exclude_patterns[j].asString();
            excludeFolders.push_back(pat);
        }
    }
}

editor_ptr app_t::openEditor(std::string path)
{
    log("open: %s", path.c_str());
    for (auto e : editors) {
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
    while (it != editors.end()) {
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
