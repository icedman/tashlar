#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"

struct statusbar_t {
public:
    statusbar_t()
        : win(0)
        , frames(0)
    {
    }

    std::string status;
    std::vector<std::string> start;
    std::vector<std::string> end;

    void setText(std::string text, int pos = 0);
    void setStatus(std::string status, int frames = 2);

    void render();
    void renderLine(const char* line, int offsetX = 0);
    bool tick();

    int frames; // x seconds from last kbhit (corresponds to kbhit timeout)

    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;

    WINDOW* win;

    theme_ptr theme;
};

#endif // STATUSBAR_H