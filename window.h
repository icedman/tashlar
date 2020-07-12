#ifndef WINDOW_H
#define WINDOW_H

#include "command.h"
#include <curses.h>

struct window_t {
    window_t(bool focusable);
    virtual void layout(int w, int h);
    virtual void render();
    virtual void renderCursor();
    virtual bool processCommand(command_e cmd, char ch);
    virtual bool isFocused();

    void scrollToCursor(int x, int y);

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