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
    std::map<int, std::string> text;
    std::map<int, int> sizes;

    void setText(std::string text, int pos = 0, int size = 12);
    void setStatus(std::string status, int frames = 2000);

    void layout(int w, int h) override;
    void render() override;
    void renderLine(const char* line, int offsetX = 0, int size = 0);
    void update(int tick);

    int frames; // x mseconds from last kbhit (corresponds to kbhit timeout)
    std::string prevStatus;
};

#endif // STATUSBAR_H