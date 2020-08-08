#ifndef APP_H
#define APP_H

#include "explorer.h"
#include "extension.h"
#include "gem.h"
#include "popup.h"
#include "statusbar.h"
#include "tabbar.h"
#include "view.h"

#include <string>

enum color_pair_e {
    NORMAL = 0,
    SELECTED
};

struct app_t : view_t {
    app_t();
    ~app_t();

    static app_t* instance();
    static void log(const char* format, ...);

    void setClipboard(std::string text);
    std::string clipboard();
    std::string clipText;

    void configure(int argc, char** argv);
    void setupColors();

    // view
    /*
    void update(int delta) override;
    void render() override;
    void calculate() override;
    */

    void layout(int x, int y, int width, int height) override;
    bool input(char ch, std::string keys) override;

    extension_list extensions;
    theme_ptr theme;

    // settings
    int tabSize;
    bool lineWrap;
    bool showStatusBar;
    bool showGutter;
    bool showTabbar;
    bool showSidebar;
    bool showMinimap;

    bool debug;

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

    std::vector<std::string> excludeFiles;
    std::vector<std::string> excludeFolders;
    std::string scriptPath;

    gem_list editors;
    explorer_t explorer;
    statusbar_t statusBar;
    tabbar_t tabBar;
    popup_t popup;

    view_t mainView;
    view_t tabView;
    view_t tabContent;
    view_t topBar;
    view_t bottomBar;

    editor_ptr openEditor(std::string path);
    editor_ptr currentEditor;

    bool end;
};

int pairForColor(int colorIdx, bool selected);

#endif // APP_H