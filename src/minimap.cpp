#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <sstream>

#include "app.h"
#include "editor.h"
#include "minimap.h"
#include "render.h"

#include "dots.h"

#define MINIMAP_WIDTH 10

minimap_t::minimap_t()
    : view_t("minimap")
    , offsetY(0)
{
    backgroundColor = 1;
}

minimap_t::~minimap_t()
{
}

void minimap_t::buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth)
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

    /*
    block_list &snapBlocks = editor->snapshots[0].snapshot;

    int *targetColors = (int*)malloc(bufferWidth * 8 * sizeof(int));
    blockData->dotColors = targetColors;

    if (render_t::instance()->fh > 10) {
    for(int i=0;i<bufferWidth;i++) {
        int colors[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

        blockdata_t *blockData = block->data.get();
        if (!blockData || !blockData->spans.size()) {
            if (block->lineNumber < snapBlocks.size()) {
                blockData = snapBlocks[block->lineNumber-1]->data.get();
            }
        }
        if (blockData) {
            span_info_t si = spanAtBlock(blockData, x * textCompress);
            colors[0] = pairForColor(si.colorIndex,false);
            si = spanAtBlock(blockData, (x + 1) * textCompress);
            colors[1] = pairForColor(si.colorIndex,false);

            colors[2] = colors[0]; colors[3] = colors[1];
            colors[4] = colors[0]; colors[5] = colors[1];
            colors[6] = colors[0]; colors[7] = colors[1];

            block_ptr bNext = block->next();
            if (bNext) {
                blockData = bNext->data.get();
                if (!blockData || !blockData->spans.size()) {
                    if (bNext->lineNumber < snapBlocks.size()) {
                        blockData = snapBlocks[bNext->lineNumber-1]->data.get();
                    }
                }
                if (blockData) {
                    si = spanAtBlock(blockData, x * textCompress);
                    colors[2] = pairForColor(si.colorIndex,false);
                    si = spanAtBlock(blockData, (x + 1) * textCompress);
                    colors[3] = pairForColor(si.colorIndex,false);
                }

                bNext = bNext->next();
                if (bNext) {
                    blockData = bNext->data.get();
                    if (!blockData || !blockData->spans.size()) {
                        if (bNext->lineNumber < snapBlocks.size()) {
                            blockData = snapBlocks[bNext->lineNumber-1]->data.get();
                        }
                    }
                    if (blockData) {
                        si = spanAtBlock(blockData, x * textCompress);
                        colors[4] = pairForColor(si.colorIndex,false);
                        si = spanAtBlock(blockData, (x + 1) * textCompress);
                        colors[5] = pairForColor(si.colorIndex,false);
                    }

                    bNext = bNext->next();
                    if (bNext) {
                        blockData = bNext->data.get();
                        if (!blockData || !blockData->spans.size()) {
                            if (bNext->lineNumber < snapBlocks.size()) {
                                blockData = snapBlocks[bNext->lineNumber-1]->data.get();
                            }
                        }
                        if (blockData) {
                            si = spanAtBlock(blockData, x * textCompress);
                            colors[6] = pairForColor(si.colorIndex,false);
                            si = spanAtBlock(blockData, (x + 1) * textCompress);
                            colors[7] = pairForColor(si.colorIndex,false);
                        }
                    }
                }
            }
        }

        int *t = targetColors;
        t += (i * 8);
        memcpy(t, colors, sizeof(int) * 8);
    }
    }
    */
}

void minimap_t::update(int delta)
{
    if (!isVisible()) {
        return;
    }

    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    int fh = render_t::instance()->fh;
    // int scroll = (editor->scrollY + (-rows * 0.8)) / fh;
    int scroll = ((editor->scrollY * fh) + (-rows * 2 / 3)) / fh;
    offsetY = scroll;
    currentLine = block->lineNumber;

    // try disable scroll
    int lastLine = doc->blocks.size();
    if (lastLine / 4 < rows || offsetY < 0) {
        offsetY = 0;
    }
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

void minimap_t::preLayout()
{
    preferredWidth = (padding * 2) + (MINIMAP_WIDTH * render_t::instance()->fw);
}

void minimap_t::scroll(int s)
{
    editor->scroll(s);
}

void minimap_t::mouseDown(int x, int y, int button, int clicks)
{
    scrollbar->mouseDown(x, y, button, clicks);
}

void minimap_t::mouseUp(int x, int y, int button)
{
    scrollbar->mouseUp(x, y, button);
}

void minimap_t::mouseDrag(int x, int y, bool within)
{
    scrollbar->mouseDrag(x, y, within);
}

void minimap_t::mouseHover(int x, int y)
{
    scrollbar->mouseHover(x, y);
}
