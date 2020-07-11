#ifndef GUTTER_H
#define GUTTER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"

struct gutter_t {

    void render();
    void renderLine(const char* line, int offsetX = 0);
    
    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;

    WINDOW* win;

    theme_ptr theme;

    int colorPair;
    
};

#endif // GUTTER_H