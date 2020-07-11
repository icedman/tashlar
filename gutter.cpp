#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "editor.h"
#include "app.h"
#include "gutter.h"
#include "explorer.h"

void gutter_t::render()
{}

void gutter_t::renderLine(const char* line)
{    
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
    }
}

void renderGutter(struct gutter_t& gutter)
{
    if (!app_t::instance()->showGutter) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor;
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // calculate view
    //-----------------
    int gutterWidth = 4;
    int lineNoMax = std::to_string(doc->blocks.back().lineNumber).length() + 2;
    if (lineNoMax > gutterWidth) {
        gutterWidth = lineNoMax;
    }
    
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    gutter.viewX = 0;
    gutter.viewY = 0;
    gutter.viewWidth = gutterWidth;
    gutter.viewHeight = editor->viewHeight;

    if (app_t::instance()->showSidebar) {
        int explorerWidth = app_t::instance()->explorer->viewWidth;
        gutter.viewX += explorerWidth;
    }

    if (!gutter.win) {
        gutter.win = newwin(gutter.viewHeight, gutter.viewWidth, 0, 0);
    }

    mvwin(gutter.win, gutter.viewY, gutter.viewX);
    wresize(gutter.win, gutter.viewHeight, gutter.viewWidth);

    wmove(gutter.win, 0, 0);
    
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
        int x = gutter.viewWidth - (lineNo.length() + 1);

        wattron(gutter.win, COLOR_PAIR(gutter.colorPair));
        wmove(gutter.win, y, 0);
        wclrtoeol(gutter.win);

        if (block.lineNumber == b.lineNumber) {
            wattron(gutter.win, A_BOLD);
        }
        
        wmove(gutter.win, y, x);        
        gutter.renderLine(lineNo.c_str());
        wattroff(gutter.win, COLOR_PAIR(gutter.colorPair));
        wattroff(gutter.win, A_BOLD);
        wattroff(gutter.win, A_REVERSE);

        for(int i=0;i<b.lineCount;i++) {
            y++;
            wmove(gutter.win, y, 0);
            wclrtoeol(gutter.win);
            if (y >= gutter.viewHeight) {
                break;
            }
        }

        if (y >= gutter.viewHeight) {
            break;
        }
    }
    while (y < gutter.viewHeight) {
        wmove(gutter.win, y++, 0);
        wclrtoeol(gutter.win);
    }
}

