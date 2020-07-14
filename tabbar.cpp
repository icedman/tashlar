#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "tabbar.h"
#include "gutter.h"
#include "statusbar.h"

#include <algorithm>

static bool compareFile(struct tabitem_t& f1, struct tabitem_t& f2)
{
    return f1.name < f2.name;
}
    
void tabbar_t::render()
{
    if (!app_t::instance()->showTabbar) {
        return;
    }

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    struct app_t * app = app_t::instance();
    bool hasFocus = isFocused();
    
    if (!tabs.size()) {
        for(auto e : app->editors) {
            struct tabitem_t item = {
                // .name = " " + e->document.fileName + " ",
                .name = e->document.fileName,
                .itemNumber = tabs.size(),
                .width = e->document.fileName.length() + 2,
                .editor = e
            };
            tabs.emplace_back(item);
        }
        sort(tabs.begin(), tabs.end(), compareFile);
    }

    // compute
    int x = 0;
    int totalWidth = 0;
    int currentTabX = 0;
    int currentTabWidth = 0;
    for(auto t : tabs) {
        if (t.editor->id == app->currentEditor->id) {
            currentTabX = x;
            currentTabWidth = t.width;            
        }
        totalWidth += t.width;
        x += t.width;
    }

    while (currentTabX + currentTabWidth + 1 - scrollX > viewWidth) {
        scrollX++;
    }
    while (scrollX > 0 && currentTabX - scrollX <= 0) {
        scrollX--;
    }
    
    int offsetX = scrollX;
    wmove(win, 0,0);
    wclrtoeol(win);
    for(auto t : tabs) {
        int pair = colorPair;
        if (t.editor->id == app->currentEditor->id) {
            
            // wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(colorPairIndicator));
            wattron(win, A_BOLD);
            renderLine("[", offsetX);
            wattroff(win, COLOR_PAIR(colorPairIndicator));
            wattroff(win, A_BOLD);

            if (hasFocus) {
                wattron(win, A_REVERSE);
                // pair = colorPairSelected;
            }
            // wattron(win, hasFocus ? A_REVERSE : A_BOLD);
        } else {
            renderLine(" ", offsetX);
        }
        wattron(win, COLOR_PAIR(pair));
        renderLine(t.name.c_str(), offsetX);
        wattroff(win, COLOR_PAIR(pair));
        
        wattroff(win, A_REVERSE);
        
        if (t.editor->id == app->currentEditor->id) {
            // wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(colorPairIndicator));
            wattron(win, A_BOLD);
            renderLine("]", offsetX);
            wattroff(win, COLOR_PAIR(colorPairIndicator));
            wattroff(win, A_BOLD);
        } else {
            renderLine(" ", offsetX);
        }
    }

    wrefresh(win);
}

void tabbar_t::renderLine(const char* line, int& offsetX)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        waddch(win, c);
    }
}

void tabbar_t::layout(int w, int h)
{
    if (!app_t::instance()->showTabbar) {
        viewWidth = 0;
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;

    viewX = 0;
    viewY = 0;
    viewWidth = w;
    viewHeight = 1;

    if (app_t::instance()->showSidebar) {
        int explorerWidth = app_t::instance()->explorer->viewWidth;
        viewWidth -= explorerWidth;
        viewX += explorerWidth;
    }
}

bool tabbar_t::processCommand(command_e cmd, char ch)
{    
    // proceed only if got focus
    if (app_t::instance()->focused->id != id) {
        return false;
    }

    struct app_t * app = app_t::instance();
    
    switch(cmd) {
    case CMD_MOVE_CURSOR_LEFT:
    case CMD_MOVE_CURSOR_RIGHT: {
        std::vector<struct tabitem_t>::iterator it = tabs.begin();
        std::vector<struct tabitem_t>::iterator prev = it;
        std::vector<struct tabitem_t>::iterator next = it;
        while(it != tabs.end()) {
            if (it != tabs.begin()) {
                prev = it-1;
            }
            if (it + 1 != tabs.end()) {
                next = it+1;
            }
            if (it->editor->id == app->currentEditor->id) {
                if (cmd == CMD_MOVE_CURSOR_LEFT) {
                    it = prev;
                } else {
                    it = next;
                }
                if (it != tabs.end()) {
                    app->openEditor(it->editor->document.fullPath);
                    app->focused = this;
                }
                return true;
            }
            it++;
        }
        break;
    };  
    case CMD_ENTER:
    case CMD_MOVE_CURSOR_DOWN:
        app->focused = app->currentEditor.get();
        return true;
    default:
        break;
    }
    
    return false;
}
