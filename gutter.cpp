#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "gutter.h"
#include "statusbar.h"
#include "tabbar.h"

void gutter_t::render()
{
    if (!app_t::instance()->showGutter) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (app_t::instance()->showSidebar) {
        int explorerWidth = app_t::instance()->explorer->viewWidth;
        viewX += explorerWidth;
    }

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    wmove(win, 0, 0);

    int offsetY = editor->scrollY;
    int currentLine = block.lineNumber;

    //-----------------
    // render the gutter
    //-----------------
    // todo: jump to first visible block
    int y = 0;
    for (auto& b : doc->blocks) {
        if (offsetY > 0) {
            offsetY -= b.lineCount;
            continue;
        }

        std::string lineNo = std::to_string(1 + b.lineNumber);
        int x = viewWidth - (lineNo.length() + 1);

        if (b.data && b.data->foldable) {
            lineNo += "-";
        }

        wattron(win, COLOR_PAIR(colorPair));
        wmove(win, y, 0);
        wclrtoeol(win);

        if (currentLine == b.lineNumber) {
            wattron(win, A_BOLD);
        }

        wmove(win, y, x);
        renderLine(lineNo.c_str());

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

void gutter_t::renderLine(const char* line)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
    }
}

void gutter_t::layout(int w, int h)
{
    if (!app_t::instance()->showGutter) {
        viewWidth = 0;
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;

    int gutterWidth = 4;
    int lineNoMax = std::to_string(doc->blocks.back().lineNumber).length() + 2;
    if (lineNoMax > gutterWidth) {
        gutterWidth = lineNoMax;
    }

    viewX = 0;
    viewY = 0;
    viewWidth = gutterWidth;
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
