#include "app.h"
#include "render.h"
#include "search.h"
#include "util.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#define LOG_FILE "/tmp/ashlar.log"

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

static void initLog()
{
    FILE* log_file = fopen(LOG_FILE, "w");
    fclose(log_file);
}

app_t::app_t()
    : view_t("app")
    , end(false)
    , refreshCount(0)
{
    appInstance = this;
    initLog();

    viewLayout = LAYOUT_VERTICAL;
    addView(&mainView);
    /*
    addView(&bottomBar);
    bottomBar.name = "bottom";
    bottomBar.preferredHeight = 1;
    bottomBar.addView(&statusBar);
    */
    addView(&statusBar);

    mainView.addView(&explorer);
    mainView.addView(&tabView);

    tabView.viewLayout = LAYOUT_VERTICAL;
    tabView.addView(&tabBar);
    tabView.addView(&tabContent);

    addView(&popup);
}

app_t::~app_t()
{
}

void app_t::refresh()
{
    refreshCount = 8;
}

void app_t::preLayout()
{
    bottomBar.preferredHeight = render_t::instance()->fh;
    view_t::preLayout();
}

void app_t::setClipboard(std::string text)
{
    clipText = text;
}

std::string app_t::clipboard()
{
    return clipText;
}

void app_t::log(const char* format, ...)
{
    static char string[1024] = "";

    va_list args;
    va_start(args, format);
    vsnprintf(string, 1024, format, args);
    va_end(args);

    FILE* log_file = fopen(LOG_FILE, "a");
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
    const char* argScript = 0;
    const char* defaultTheme = "Monokai";

    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            argTheme = argv[i + 1];
        }
        if (strcmp(argv[i], "-s") == 0) {
            argScript = argv[i + 1];
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

    app_t::instance()->log("%d registered colors", theme->colorIndices.size());
    render_t::instance()->updateColors();

    applyTheme();
}

editor_ptr app_t::openEditor(std::string path)
{
    log("open: %s", path.c_str());
    for (auto gem : editors) {
        if (!gem->split)
            gem->setVisible(false);
    }

    for (auto gem : editors) {
        editor_ptr e = gem->editor;
        if (e->document.fullPath == path) {
            log("reopening existing tab");
            currentEditor = e;
            view_t::setFocus(currentEditor.get());
            gem->setVisible(true);
            // focused = currentEditor.get();
            return e;
        }
    }

    const char* filename = path.c_str();

    gem_ptr gem = std::make_shared<gem_t>();
    editor_ptr editor = gem->editor;
    editor->highlighter.lang = language_from_file(filename, extensions);
    editor->highlighter.theme = theme;
    editors.emplace_back(gem);

    editor->pushOp("OPEN", filename);
    editor->update(0);

    tabContent.addView(gem.get());

    currentEditor = editor;
    editor->name = "editor:";
    editor->name += path;

    gem->applyTheme();
    view_t::setFocus(currentEditor.get());

    // editor->highlighter.run(editor.get());
    return editor;
}

void app_t::layout(int x, int y, int width, int height)
{
    views.pop_back();
    view_t::layout(x, y, width, height);

    // popup is handled separately
    views.push_back(&popup);
    popup.layout(x, y, width, height);
}

bool app_t::input(char ch, std::string keys)
{
    operation_e cmd = operationFromKeys(keys);

    switch (cmd) {
    case QUIT:
        end = true;
        return true;
    case CLOSE: {
        bool found = false;
        view_list::iterator it = tabContent.views.begin();
        while (it != tabContent.views.end()) {
            gem_t* gem = (gem_t*)*it;
            if (gem->editor == currentEditor) {
                found = true;
                tabContent.views.erase(it);
                if (!tabContent.views.size()) {
                    end = true;
                    return true;
                }
                break;
            }
            it++;
        }
        gem_list::iterator it2 = editors.begin();
        while (it2 != editors.end()) {
            gem_ptr gem = *it2;
            if (gem->editor == currentEditor) {
                editors.erase(it2);
                break;
            }
            it2++;
        }
        if (found) {
            gem_t* gem = (gem_t*)(tabContent.views.front());
            editor_ptr nextEditor = gem->editor;
            app_t::instance()->openEditor(nextEditor->document.filePath);
        }
        return true;
    }
    case CANCEL:
        popup.hide();
        return false;
    case POPUP_SEARCH:
        popup.search("");
        return true;
    case POPUP_SEARCH_LINE:
        popup.search(":");
        return true;
    case POPUP_COMMANDS:
        popup.commands();
        return true;
    case POPUP_FILES:
        popup.files();
        return true;
    case MOVE_FOCUS_LEFT:
        view_t::shiftFocus(-1, 0);
        return true;
    case MOVE_FOCUS_RIGHT:
        if (explorer.isFocused()) {
            view_t::setFocus(currentEditor.get());
        } else {
            view_t::shiftFocus(1, 0);
        }
        return true;
    case MOVE_FOCUS_UP:
        view_t::shiftFocus(0, -1);
        return true;
    case MOVE_FOCUS_DOWN:
        view_t::shiftFocus(0, 1);
        return true;
    default:
        break;
    }

    return view_t::input(ch, keys);
}
