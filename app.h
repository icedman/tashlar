#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#include "command.h"
#include "editor.h"
#include "extension.h"
#include "theme.h"
#include "window.h"

struct statusbar_t;
struct gutter_t;

struct app_t {

    app_t();

    static app_t* instance();

    struct statusbar_t* statusbar;
    struct tabbar_t* tabbar;
    struct gutter_t* gutter;
    struct explorer_t* explorer;
    struct window_t* focused;

    std::vector<editor_ptr> editors;
    editor_ptr currentEditor;

    std::vector<command_e> commandBuffer;
    std::string inputBuffer;
    int refreshLoop;
    
    std::string clipBoard;

    theme_ptr theme;
    std::vector<struct extension_t> extensions;

    // settings
    std::string themeName;
    bool lineWrap;
    bool showStatusBar;
    bool showGutter;
    bool showTabbar;
    bool showSidebar;

    void configure(int argc, char** argv);
    void setupColors();
    void applyColors();
    void close();
    void refresh();
    
    // log
    static void iniLog();
    static void log(const char* format, ...);

    editor_ptr openEditor(std::string path);
};

int pairForColor(int colorIdx, bool selected);

#endif // APP_H
