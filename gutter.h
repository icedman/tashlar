#ifndef GUTTER_H
#define GUTTER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"
#include "window.h"

struct gutter_t : public window_t {

    gutter_t()
        : window_t(false)
        , colorPair(0)
    {
    }

    void layout(int w, int h) override;
    void render() override;
    void renderLine(const char* line);

    theme_ptr theme;
    int colorPair;
};

void renderGutter(struct gutter_t& gutter);

#endif // GUTTER_H