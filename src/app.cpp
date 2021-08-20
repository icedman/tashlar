#include "app.h"
#include "search.h"
#include "util.h"
#include "explorer.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

static struct app_t* appInstance = 0;

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

struct app_t* app_t::instance()
{
    return appInstance;
}

app_t::app_t()
    : end(false)
    , refreshCount(0)
{
    appInstance = this;
}

app_t::~app_t()
{
}

void app_t::refresh()
{
    refreshCount = 8;
}

void app_t::setClipboard(std::string text)
{
    clipText = text;
}

std::string app_t::clipboard()
{
    return clipText;
}

void app_t::configure(int argc, char** argv)
{
    //-------------------
    // defaults
    //-------------------
    enablePopup = true;
    markup = "";

    const char* argTheme = 0;
    const char* argScript = 0;
    const char* defaultTheme = "Monokai";

    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            argTheme = argv[i + 1];
        }
        // if (strcmp(argv[i], "-s") == 0) {
        //     argScript = argv[i + 1];
        // }
        if (strcmp(argv[i], "-m") == 0) {
            markup = argv[i + 1];
        }
    }

    if (argScript) {
        scriptPath = argScript;
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
                log("extension dir: %s", path.asString().c_str());
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
        config_t::instance()->lineWrap = settings["word_wrap"].asBool();
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
    tabSize = 4;
    if (settings.isMember("tab_size")) {
        tabSize = settings["tab_size"].asInt();
    }
    if (tabSize < 2) {
        tabSize = 2;
    }
    if (tabSize > 8) {
        tabSize = 8;
    }

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

void app_t::setupColors()
{
    style_t s = theme->styles_for_scope("default");
    // std::cout << theme->colorIndices.size() << " colors used" << std::endl;

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
        bgApp = clr.index;
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

    log("%d registered colors", theme->colorIndices.size());
    // getRenderer()->updateColors();

    // applyTheme();
}

editor_ptr app_t::openEditor(std::string path)
{
    log("open: %s", path.c_str());

    // for (auto gem : editors) {
    //     if (!gem->split)
    //         gem->setVisible(false);
    // }

    for (auto e : editors) {
        if (e->document.fullPath == path) {
            log("reopening existing tab");
            currentEditor = e;
            // view_t::setFocus(currentEditor.get());
            // gem->setVisible(true);
            // focused = currentEditor.get();
            return e;
        }
    }

    const char* filename = path.c_str();

    editor_ptr editor = std::make_shared<editor_t>();
    editor->highlighter.lang = language_from_file(filename, extensions);
    editor->highlighter.theme = theme;

    editor->pushOp("OPEN", filename);
    editor->runAllOps();

    currentEditor = editor;
    editor->name = "editor:";
    editor->name += path;

    editors.emplace_back(editor);
    log(">%d", editors.size());

    // gem->applyTheme();
    // view_t::setFocus(currentEditor.get());
    // editor->highlighter.run(editor.get());
    return editor;
}
