#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#include "command.h"
#include "extension.h"
#include "theme.h"

struct editor_t;
struct statusbar_t;
struct gutter_t;

struct app_t {

    app_t();

    static app_t* instance();

    struct editor_t* currentEditor;
    struct statusbar_t* statusbar;
    struct gutter_t* gutter;
    struct explorer_t* explorer;

    std::vector<command_e> commandBuffer;
    std::string inputBuffer;

    std::string clipBoard;

    theme_ptr theme;
    std::vector<struct extension_t> extensions;

    // settings
    std::string themeName;
    bool lineWrap;
    bool showStatusBar;
    bool showGutter;
    bool showSidebar;

    void configure(int argc, char** argv);
    void setupColors();
    void applyColors();

    // log
    static void iniLog();
    static void log(const char* format, ...);
};

int pairForColor(int colorIdx, bool selected);
    
#endif // APP_H
