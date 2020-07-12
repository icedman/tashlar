#ifndef TABBAR_H
#define TABBAR_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "editor.h"
#include "theme.h"
#include "window.h"

struct tabitem_t {
    std::string name;
    
    int itemNumber;
    int width;
    
    editor_ptr editor;
};

struct tabbar_t: public window_t {

    tabbar_t()
        : window_t(true)
        , colorPair(0)
    {
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderLine(const char* line);
    
    theme_ptr theme;
    int colorPair;

    std::vector<struct tabitem_t> tabs;
};

void renderTabbar(struct tabbar_t& tabbar);

#endif // TABBAR_H