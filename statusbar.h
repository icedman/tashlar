#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"
#include "window.h"

struct statusbar_t : public window_t {

    statusbar_t()
        : window_t(false)
        , frames(0)
        , colorPair(0)
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
    bool tick(int tick);

    int frames; // x mseconds from last kbhit (corresponds to kbhit timeout)
    std::string prevStatus;

    theme_ptr theme;
    int colorPair;
};

void renderStatus(struct statusbar_t& statusbar);

#endif // STATUSBAR_H