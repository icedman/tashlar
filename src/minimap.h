#ifndef MINIMAP_H
#define MINIMAP_H

#include <string>
#include <vector>

#include "document.h"
#include "extension.h"
#include "window.h"

struct minimap_t : public window_t {

    minimap_t();
    void layout(int w, int h) override;
    void render() override;
    void renderLine(const char* line);
};

#endif // MINIMAP_H
