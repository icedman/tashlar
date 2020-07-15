#ifndef WINDOW_H
#define WINDOW_H

#include <curses.h>

#include "command.h"
#include "theme.h"

struct window_t {
    window_t(bool focusable);
    virtual void layout(int w, int h) = 0;
    virtual void update(int frames);
    virtual void render() = 0;
    virtual void renderCursor();
    virtual bool processCommand(command_e cmd, char ch);
    virtual bool isFocused();
    virtual bool isShown();

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

    bool shown;

    theme_ptr theme;
    int colorPair;
    int colorPairSelected;
    int colorPairIndicator;
};

#endif // WINDOW_H