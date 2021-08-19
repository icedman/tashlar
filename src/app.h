#ifndef APP_H
#define APP_H

#include "explorer.h"
#include "extension.h"
#include "gem.h"
#include "popup.h"
#include "scrollbar.h"
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

    void setClipboard(std::string text);
    std::string clipboard();
    std::string clipText;

    void configure(int argc, char** argv);
    void setupColors();
    void setupView();

    // view
    void layout(int x, int y, int width, int height) override;
    bool input(char ch, std::string keys) override;
    void preLayout() override;

    void refresh();

    extension_list extensions;
    theme_ptr theme;

    // settings
    int tabSize;
    bool showStatusBar;
    bool showGutter;
    bool showTabbar;
    bool showSidebar;
    bool showMinimap;
    bool enablePopup;
    
    std::string markup;

    bool debug;

    // colors
    std::string themeName;
    int fg;
    int bg;
    int bgApp;
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

    scrollbar_t explorerScrollbar;
    scrollbar_t editorScrollbar;

    view_t mainView;
    view_t tabView;
    view_t tabContent;
    view_t topBar;
    view_t bottomBar;

    editor_ptr openEditor(std::string path);
    editor_ptr currentEditor;

    bool end;
    int refreshCount;
};

int pairForColor(int colorIdx, bool selected);

#endif // APP_H
