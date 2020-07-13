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
#include "tabbar.h"
#include "statusbar.h"

#include "dots.h"

#define MINIMAP_WIDTH 10

minimap_t::minimap_t()
        : window_t(false)
{}

void minimap_t::render()
{
    if (!app_t::instance()->showMinimap) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    int offsetY = editor->scrollY;
    int currentLine = block.lineNumber;

    int wc = 0;
    wc = buildUpDots(wc, 0, 1, 1);
    wc = buildUpDots(wc, 3, 1, 1);
    
    // todo: jump to first visible block
    int y = 0;
    for(int idx=0; idx<doc->blocks.size(); idx++) {
        auto& b = doc->blocks[idx];
        if (offsetY > 0) {
            offsetY -= b.lineCount;
            continue;
        }

        std::string lineNo = std::to_string(1 + b.lineNumber);
        std::string t = b.text();
        char *line = (char*)t.c_str();
        char *end = line + t.length();

        wattron(win, COLOR_PAIR(colorPair));
        wmove(win, y, 0);
        wclrtoeol(win);

        if (currentLine == b.lineNumber) {
            wattron(win, A_BOLD);
        }

        struct blockdata_t *data = b.data.get();
        if (data) {
            for(int j=0; j<b.length && line < end; j++) {
                char c = *line;
                if (c != ' ') {
                    waddwstr(win, dotsFromChar(wc));
                } else {
                    waddch(win, ' ');
                }
                if (j > viewWidth) {
                    break;
                }
                for(int k=0;k<8;k++) {
                    line++;
                    if (line == end) {
                        break;
                    }
                }
            }
        }
        
        // renderLine(lineNo.c_str());

        wattroff(win, COLOR_PAIR(colorPair));
        wattroff(win, A_BOLD);
        wattroff(win, A_REVERSE);

        for (int i = 0; i < b.lineCount; i++) {
            y++;
            wmove(win, y, 0);
            wclrtoeol(win);
            if (y >= viewHeight) {
                break;
            }
        }

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
