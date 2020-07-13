#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "window.h"

struct statusbar_t : public window_t {

    statusbar_t()
        : window_t(false)
        , frames(0)
    {
    }

    std::string status;
    std::vector<std::string> start;
    std::vector<std::string> end;

    void setText(std::string text, int pos = 0);
    void setStatus(std::string status, int frames = 2000);

    void layout(int w, int h) override;
    void render() override;
    void renderLine(const char* line, int offsetX = 0);
    void update(int tick);

    int frames; // x mseconds from last kbhit (corresponds to kbhit timeout)
    std::string prevStatus;
};

#endif // STATUSBAR_H