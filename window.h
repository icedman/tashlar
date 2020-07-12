#ifndef WINDOW_H
#define WINDOW_H

#include <curses.h>
#include "command.h"

struct window_t {
    window_t(bool focusable);
    virtual void layout(int w, int h);
    virtual void render();
    virtual void renderCursor();
    virtual bool processCommand(command_e cmd, char ch);

    int id;
    bool focusable;
    int scrollX;
    int scrollY;
    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;
    int cursorScreenX;
    int cursorScreenY;
    WINDOW* win;
};

#endif // WINDOW_H