#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <sstream>

#include "minimap_view.h"
#include "app.h"
#include "editor.h"
#include "render.h"
#include "dots.h"

#include "scrollbar_view.h"

#define MINIMAP_WIDTH 10

minimap_view_t::minimap_view_t(editor_ptr editor)
    : view_t("minimap")
    , offsetY(0)
    , editor(editor)
{
    backgroundColor = 1;
}

minimap_view_t::~minimap_view_t()
{
}

#define minimap_view_tEXT_COMPRESS 5
#define minimap_view_tEXT_BUFFER 25

void minimap_view_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(0, 0);

    // int fh = getRenderer()->fh;
    // int sy = editor->scrollY / fh;

    int sy = offsetY;
    int y = 0;
    for (int idx = sy; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];

        if (y == 0) {
            firstVisibleLine = b->lineNumber;
        }
        lastVisibleLine = b->lineNumber;

        int pair = colorPrimary;
        _move(y, 0);
        _clrtoeol(cols);
        _move(y++, 0);

        if (currentLine >= b->lineNumber && currentLine < b->lineNumber + 4) {
            _bold(true);
            _attron(_color_pair(colorIndicator));

#ifdef ENABLE_UTF8
            _addwstr(L"\u2192");
#else
            _addch('>');
#endif

            _attroff(_color_pair(colorIndicator));
        } else {
            _addch(' ');
        }

        buildUpDotsForBlock(b, minimap_view_tEXT_COMPRESS, minimap_view_tEXT_BUFFER);

        int textCompress = minimap_view_tEXT_COMPRESS;
        block_list& snapBlocks = editor->snapshots[0].snapshot;

        for (int x = 0; x < minimap_view_tEXT_BUFFER; x++) {
            int x1 = x + 1;
            int x2 = x1 + 1;
            _attron(_color_pair(pair));

            int colors[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

            if (!getRenderer()->isTerminal() && snapBlocks.size() && b->lineNumber > 0) {

                blockdata_t* blockData = b->data.get();
                if (!blockData || !blockData->spans.size()) {
                    if (b->lineNumber <= snapBlocks.size()) {
                        blockData = snapBlocks[b->lineNumber - 1]->data.get();
                    }
                }
                if (blockData) {
                    span_info_t si = spanAtBlock(blockData, x * textCompress);
                    colors[0] = pairForColor(si.colorIndex, false);
                    si = spanAtBlock(blockData, (x1 + 1) * textCompress);
                    colors[1] = pairForColor(si.colorIndex, false);

                    colors[2] = colors[0];
                    colors[3] = colors[1];
                    colors[4] = colors[0];
                    colors[5] = colors[1];
                    colors[6] = colors[0];
                    colors[7] = colors[1];

                    block_ptr bNext = b->next();
                    if (bNext) {
                        blockData = bNext->data.get();
                        if (!blockData || !blockData->spans.size()) {
                            if (bNext->lineNumber <= snapBlocks.size()) {
                                blockData = snapBlocks[bNext->lineNumber - 1]->data.get();
                            }
                        }
                        if (blockData) {
                            si = spanAtBlock(blockData, x1 * textCompress);
                            colors[2] = pairForColor(si.colorIndex, false);
                            si = spanAtBlock(blockData, x2 * textCompress);
                            colors[3] = pairForColor(si.colorIndex, false);
                        }

                        bNext = bNext->next();
                        if (bNext) {
                            blockData = bNext->data.get();
                            if (!blockData || !blockData->spans.size()) {
                                if (bNext->lineNumber <= snapBlocks.size()) {
                                    blockData = snapBlocks[bNext->lineNumber - 1]->data.get();
                                }
                            }
                            if (blockData) {
                                si = spanAtBlock(blockData, x1 * textCompress);
                                colors[4] = pairForColor(si.colorIndex, false);
                                si = spanAtBlock(blockData, x2 * textCompress);
                                colors[5] = pairForColor(si.colorIndex, false);
                            }

                            bNext = bNext->next();
                            if (bNext) {
                                blockData = bNext->data.get();
                                if (!blockData || !blockData->spans.size()) {
                                    if (bNext->lineNumber <= snapBlocks.size()) {
                                        blockData = snapBlocks[bNext->lineNumber - 1]->data.get();
                                    }
                                }
                                if (blockData) {
                                    si = spanAtBlock(blockData, x1 * textCompress);
                                    colors[6] = pairForColor(si.colorIndex, false);
                                    si = spanAtBlock(blockData, x2 * textCompress);
                                    colors[7] = pairForColor(si.colorIndex, false);
                                }
                            }
                        }
                    }
                }
            }

            if (!_drawdots(b->data->dots[x], colors)) {
#ifdef ENABLE_UTF8
                _addwstr(wcharFromDots(b->data->dots[x]));
#endif
            }

            _attroff(_color_pair(pair));

            if (x >= cols - 2) {
                break;
            }
        }

        _attroff(_color_pair(pair));
        _bold(false);

        if (y > rows) {
            break;
        }
    }
    while (y < rows) {
        _move(y++, 0);
        _clrtoeol(cols);
    }
}

void minimap_view_t::buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth)
{
    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }

    struct blockdata_t* blockData = block->data.get();
    if (blockData->dots) {
        return;
    }

    // log("minimap %d", block->lineNumber);

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

void minimap_view_t::update(int delta)
{
    if (!isVisible()) {
        return;
    }

    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    int fh = getRenderer()->fh;
    // int scroll = (editor->scrollY + (-rows * 0.8)) / fh;
    int scroll = ((editor->view->scrollY * fh) + (-rows * 2 / 3)) / fh;
    offsetY = scroll;
    currentLine = block->lineNumber;

    // try disable scroll
    int lastLine = doc->blocks.size();
    if (lastLine / 4 < rows || offsetY < 0) {
        offsetY = 0;
    }
}

void minimap_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool minimap_view_t::isVisible()
{
    return visible == app_t::instance()->showMinimap;
}

void minimap_view_t::preLayout()
{
    preferredWidth = (padding * 2) + (MINIMAP_WIDTH * getRenderer()->fw);
}

void minimap_view_t::scroll(int s)
{
    editor->view->scroll(s);
}

void minimap_view_t::mouseDown(int x, int y, int button, int clicks)
{
    parentView->verticalScrollbar->mouseDown(x, y, button, clicks);
}

void minimap_view_t::mouseUp(int x, int y, int button)
{
    parentView->verticalScrollbar->mouseUp(x, y, button);
}

void minimap_view_t::mouseDrag(int x, int y, bool within)
{
    parentView->verticalScrollbar->mouseDrag(x, y, within);
}

void minimap_view_t::mouseHover(int x, int y)
{
    parentView->verticalScrollbar->mouseHover(x, y);
}
