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
    struct app_t * app = app_t::instance();
    
    if (!tabs.size()) {
        for(auto e : app->editors) {
            struct tabitem_t item = {
                .name = e->document.fileName,
                .itemNumber = tabs.size(),
                .width = e->document.fileName.length() + 2,
                .editor = e
            };
            tabs.emplace_back(item);
        }
        sort(tabs.begin(), tabs.end(), compareFile);
    }

    wmove(win, 0,0);
    for(int i=0;i<viewWidth;i++) {
        waddch(win, ' ');
    }
    
    wmove(win, 0,4);
    for(auto t : tabs) {
        std::string name = " ";
        name += t.name;
        name += " ";

        if (t.editor->id == app->currentEditor->id) {
            wattron(win, A_REVERSE);
        }
        
        renderLine(name.c_str());
        wattroff(win, A_REVERSE);
    }
}

void tabbar_t::renderLine(const char* line)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
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

void renderTabbar(struct tabbar_t& tabbar)
{
    if (!app_t::instance()->showTabbar) {
        return;
    }

    if (!tabbar.win) {
        tabbar.win = newwin(tabbar.viewHeight, tabbar.viewWidth, 0, 0);
    }

    mvwin(tabbar.win, tabbar.viewY, tabbar.viewX);
    wresize(tabbar.win, tabbar.viewHeight, tabbar.viewWidth);
    
    tabbar.render();
}

bool tabbar_t::processCommand(command_e cmd, char ch)
{
    return false;
}
    