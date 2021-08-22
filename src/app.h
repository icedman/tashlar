#ifndef APP_H
#define APP_H

#include "extension.h"
#include "editor.h"

#include <string>

enum color_pair_e {
    NORMAL = 0,
    SELECTED
};

int pairForColor(int colorIdx, bool selected);

struct app_t {
    app_t();
    ~app_t();

    static app_t* instance();

    void setClipboard(std::string text);
    std::string clipboard();
    std::string clipText;

    void configure(int argc, char** argv);
    void setupColors();

    // view
    // void layout(int x, int y, int width, int height) override;
    // void preLayout() override;

    void refresh();
    bool isFresh();

    bool showStatusBar;
    bool showGutter;
    bool showTabbar;
    bool showSidebar;
    bool showMinimap;
    bool enablePopup;
    int tabSize;
    bool lineWrap;

    extension_list extensions;
    theme_ptr theme;
    
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

    editor_ptr openEditor(std::string path);
    editor_ptr currentEditor;

    editor_list editors;

    bool end;
    int refreshCount;
};

int pairForColor(int colorIdx, bool selected);

#endif // APP_H
