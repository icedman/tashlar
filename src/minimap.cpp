#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>

#include "app.h"
#include "editor.h"
#include "minimap.h"

#include "dots.h"

#define MINIMAP_WIDTH 10

minimap_t::minimap_t()
    : view_t("minimap")
{
    preferredWidth = MINIMAP_WIDTH;
}

minimap_t::~minimap_t()
{
}

void buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth)
{
    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }

    struct blockdata_t* blockData = block->data.get();
    if (blockData->dots) {
        return;
    }

    // app_t::instance()->log("minimap %d", block->lineNumber);

    std::string line1;
    std::string line2;
    std::string line3;
    std::string line4;

    block_ptr it = block;
    line1 = it->text();
    if (it->next()) {
        it = it->next();
        line2 = it->text();
        if (it->next()) {
            it = it->next();
            line3 = it->text();
            if (it->next()) {
                it = it->next();
                line4 = it->text();
            }
        }
    }

    blockData->dots = buildUpDotsForLines(
        (char*)line1.c_str(),
        (char*)line2.c_str(),
        (char*)line3.c_str(),
        (char*)line4.c_str(),
        textCompress,
        bufferWidth);
}

void minimap_t::update(int delta)
{
    if (!isVisible()) {
        return;
    }

    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    int scroll = editor->scrollY + (-height * 0.8);
    int offsetY = scroll / 4;
    currentLine = block->lineNumber;

    // try disable scroll
    int lastLine = doc->blocks.back()->lineNumber;
    if (lastLine / 4 < height) {
        offsetY = 0;
    }

    int wc = 0;
    wc = buildUpDots(wc, 1, 1, 1);
    wc = buildUpDots(wc, 2, 1, 1);
}

void minimap_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool minimap_t::isVisible()
{
    return visible == app_t::instance()->showMinimap;
}
