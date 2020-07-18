#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "minimap.h"
#include "statusbar.h"
#include "tabbar.h"

#include "dots.h"

#define MINIMAP_WIDTH 10
#define MINIMAP_TEXT_COMPRESS 5

minimap_t::minimap_t()
    : window_t(false)
{
}

void buildUpDotsForBlock(struct block_t* block, float textCompress, int bufferWidth)
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

    struct block_t* it = block;
    line1 = it->text();
    if (it->next) {
        it = it->next;
        line2 = it->text();
        if (it->next) {
            it = it->next;
            line3 = it->text();
            if (it->next) {
                it = it->next;
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

void minimap_t::render()
{
    if (!app_t::instance()->showMinimap) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);
    wmove(win, 0, 0);

    int scroll = editor->scrollY + (-viewHeight * 0.8);
    int offsetY = scroll / 4;
    int currentLine = block.lineNumber;

    // try disable scroll
    int lastLine = doc->blocks.back().lineNumber;
    if (lastLine / 4 < viewHeight) {
        offsetY = 0;
    }

    int wc = 0;
    wc = buildUpDots(wc, 1, 1, 1);
    wc = buildUpDots(wc, 2, 1, 1);

    // todo: jump to first visible block
    int y = 0;
    for (int idx = 0; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];
        if (offsetY > 0) {
            offsetY -= 1; // b.lineCount;
            continue;
        }

        int pair = colorPair;
        wmove(win, y++, 0);
        wclrtoeol(win);

        if (currentLine >= b.lineNumber && currentLine < b.lineNumber + 4) {
            wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(colorPairIndicator));
            waddwstr(win, L"\u2192");
            wattroff(win, COLOR_PAIR(colorPairIndicator));
            // wattron(win, A_REVERSE);
        } else {
            waddch(win, ' ');
        }

        buildUpDotsForBlock(&b, MINIMAP_TEXT_COMPRESS, 25);
        for (int x = 0; x < 25; x++) {
            // int cx = 1 + (x * 4);
            // for (auto span : b.data->spans) {
            // if (cx >= span.start && cx < span.start + span.length) {
            // pair = pairForColor(span.colorIndex, false);
            // break;
            // }
            // }

            wattron(win, COLOR_PAIR(pair));
            waddwstr(win, wcharFromDots(b.data->dots[x]));
            wattroff(win, COLOR_PAIR(pair));

            if (x >= viewWidth - 1) {
                break;
            }
        }

        wattroff(win, COLOR_PAIR(colorPair));
        wattroff(win, A_BOLD);
        // wattroff(win, A_REVERSE);

        if (y >= viewHeight) {
            break;
        }
    }
    while (y < viewHeight) {
        wmove(win, y++, 0);
        wclrtoeol(win);
    }

    wrefresh(win);
}

void minimap_t::renderLine(const char* line)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
    }
}

void minimap_t::layout(int w, int h)
{
    if (!app_t::instance()->showMinimap) {
        viewWidth = 0;
        return;
    }

    int minimapWidth = MINIMAP_WIDTH;

    viewX = w - minimapWidth;
    viewY = 0;
    viewWidth = minimapWidth;
    viewHeight = h;

    if (app_t::instance()->showStatusBar) {
        int statusbarHeight = app_t::instance()->statusbar->viewHeight;
        viewHeight -= statusbarHeight;
    }

    if (app_t::instance()->showTabbar) {
        int tabbarHeight = app_t::instance()->tabbar->viewHeight;
        viewHeight -= tabbarHeight;
        viewY += tabbarHeight;
    }
}
