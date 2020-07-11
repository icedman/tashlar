#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#include "command.h"
#include "extension.h"
#include "theme.h"

struct editor_t;
struct statusbar_t;

struct app_t {

    app_t();

    static app_t* instance();

    struct editor_t* currentEditor;
    struct statusbar_t* statusbar;

    std::vector<command_e> commandBuffer;
    std::string inputBuffer;

    std::string clipBoard;

    theme_ptr theme;
    std::vector<struct extension_t> extensions;

    // settings
    std::string themeName;
    bool lineWrap;
    bool showStatusBar;

    void configure(int argc, char** argv);
    void setupColors();

    // log
    static void iniLog();
    static void log(const char* format, ...);
};

#endif // APP_H
