#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"

struct statusbar_t {
    statusbar_t()
        : win(0)
        , frames(0)
        , colorPair(0)
    {
    }

    std::string status;
    std::vector<std::string> start;
    std::vector<std::string> end;

    void setText(std::string text, int pos = 0);
    void setStatus(std::string status, int frames = 2000);

    void render();
    void renderLine(const char* line, int offsetX = 0);
    bool tick(int tick);

    int frames; // x mseconds from last kbhit (corresponds to kbhit timeout)
    std::string prevStatus;

    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;

    WINDOW* win;

    theme_ptr theme;

    int colorPair;
};

#endif // STATUSBAR_H