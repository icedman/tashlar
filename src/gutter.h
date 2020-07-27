#ifndef GUTTER_H
#define GUTTER_H

#include <string>
#include <vector>

#include "extension.h"
#include "window.h"

struct gutter_t : public window_t {

    gutter_t()
        : window_t(false)
    {
    }

    void layout(int w, int h) override;
    void render() override;
    void renderLine(const char* line);
};

#endif // GUTTER_H
