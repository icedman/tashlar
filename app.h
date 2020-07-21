#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#include "command.h"
#include "editor.h"
#include "extension.h"
#include "popup.h"
#include "theme.h"
#include "window.h"

struct statusbar_t;
struct gutter_t;
struct tabbar_t;
struct explorer_t;
struct minimap_t;
struct popup_t;

struct cmd_t {
    cmd_t(command_e c = CMD_UNKNOWN, std::string a = "")
        : cmd(c)
        , args(a)
    {
    }
    command_e cmd;
    std::string args;
};

struct app_t {

    app_t();
    ~app_t();

    static app_t* instance();

    struct statusbar_t* statusbar;
    struct explorer_t* explorer;
    struct tabbar_t* tabbar;
    struct window_t* gutter;
    struct window_t* minimap;
    struct window_t* popup;
    struct window_t* focused;

    int refreshLoop;

    std::vector<editor_ptr> editors;
    editor_ptr currentEditor;

    std::vector<cmd_t> commandBuffer;
    std::string inputBuffer;

    std::string clipBoard;

    theme_ptr theme;
    std::vector<struct extension_t> extensions;

    // settings
    int tabSize;
    bool lineWrap;
    bool showStatusBar;
    bool showGutter;
    bool showTabbar;
    bool showSidebar;
    bool showMinimap;

    // colors
    std::string themeName;
    int fg;
    int bg;
    int selFg;
    int selBg;
    int treeFg;
    int treeBg;
    int tabFg;
    int tabBg;
    int tabHoverFg;
    int tabHoverBg;
    int tabActiveBorder;

    void configure(int argc, char** argv);
    void setupColors();
    void applyColors();
    void close();
    void refresh();
    void layout();
    void render();

    void update(int frames);
    void resetIdle();
    int isIdle();

    bool processCommand(command_e cmd, char ch);

    // log
    static void initLog();
    static void log(const char* format, ...);

    editor_ptr openEditor(std::string path);
    void save();
    void saveAs(std::string path, bool replaceName = false);

    std::vector<struct window_t*> windows;
    int width;
    int height;

    std::vector<std::string> excludeFiles;
    std::vector<std::string> excludeFolders;
    std::string scriptPath;

    bool idle;
    int idleIndex;
    int idleCount;
};

int pairForColor(int colorIdx, bool selected);

#endif // APP_H
