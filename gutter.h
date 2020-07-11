#ifndef GUTTER_H
#define GUTTER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"

struct gutter_t {

    gutter_t()
        : win(0)
        , colorPair(0) {
    }
    
    void render();
    void renderLine(const char* line);
    
    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;

    WINDOW* win;

    theme_ptr theme;
    int colorPair;
};

void renderGutter(struct gutter_t& gutter);

#endif // GUTTER_H