#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#include "command.h"
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

    // settings
    bool lineWrap;

    void configure();
    void setupColors();
    
    // log
    static void iniLog();
    static void log(const char* format, ...);
};

#endif // APP_H