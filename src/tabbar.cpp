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

#include <algorithm>

#define TABBAR_HEIGHT 1

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

    struct app_t* app = app_t::instance();
    bool hasFocus = isFocused();

    if (!tabs.size()) {
        for (auto e : app->editors) {
            struct tabitem_t item = {
                // .name = " " + e->document.fileName + " ",
                .name = e->document.fileName,
                .itemNumber = (int)tabs.size(),
                .width = (int)e->document.fileName.length() + 2,
                .editor = e
            };
            tabs.emplace_back(item);
        }
        sort(tabs.begin(), tabs.end(), compareFile);
    }

    // compute
    int tabX = 0;
    int totalWidth = 0;
    int currentTabX = 0;
    int currentTabWidth = 0;
    for (auto t : tabs) {
        if (t.editor->id == app->currentEditor->id) {
            currentTabX = tabX;
            currentTabWidth = t.width;
        }
        totalWidth += t.width;
        tabX += t.width;
    }

    while (currentTabX + currentTabWidth + 1 - scrollX > viewWidth) {
        scrollX++;
    }
    while (scrollX > 0 && currentTabX - scrollX <= 0) {
        scrollX--;
    }

    int offsetX = scrollX;
    int x = 0;
    wmove(win, 0, 0);
    wclrtoeol(win);

    int tabNo = 1;
    for (auto t : tabs) {
        int pair = colorPair;
        if (t.editor->id == app->currentEditor->id) {

            // wattron(win, A_BOLD);
            if (hasFocus) {
                wattron(win, A_REVERSE);
                // pair = colorPairSelected;
            }

            wattron(win, COLOR_PAIR(colorPairIndicator));
            wattron(win, A_BOLD);
            renderLine("[", offsetX, x);
            wattroff(win, COLOR_PAIR(colorPairIndicator));
            wattroff(win, A_BOLD);

            // wattron(win, hasFocus ? A_REVERSE : A_BOLD);
        } else {
            renderLine(" ", offsetX, x);
        }

        wattron(win, COLOR_PAIR(pair));
        renderLine(t.name.c_str(), offsetX, x);
        wattroff(win, COLOR_PAIR(pair));

        if (t.editor->id == app->currentEditor->id) {
            // wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(colorPairIndicator));
            wattron(win, A_BOLD);
            renderLine("]", offsetX, x);
            wattroff(win, COLOR_PAIR(colorPairIndicator));
            wattroff(win, A_BOLD);
        } else {
            renderLine(" ", offsetX, x);
        }

        wattroff(win, A_REVERSE);

        tabNo++;
    }

    // renderWidget();

    wrefresh(win);
}

void tabbar_t::renderWidget()
{
    /*
    struct app_t *app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);  

    std::vector<std::string> levels;

    struct block_t *cur = &block;
    for(int i=0;i<20;i++) {
       struct blockdata_t *data = cur->data.get();
       if (!data) break;

           app->log("----------");

       const char *text = cur->text().c_str();
       std::vector<span_info_t>::reverse_iterator it = data->spans.rbegin();
       int j=0;
       while(it != data->spans.rend()) {
           span_info_t span = *it;
           app->log("%d %s;", j++, span.scope.c_str());
           levels.insert(levels.begin(), 1, span.scope);
           it++;
       }

       cur = cur->previous;
       if (!cur) {
           break;
       }

           app->log("!");
       break;
    }

    std::string s;
    int x = 0;
    int offsetX = 0;
    wmove(win, 1, 0);
    wclrtoeol(win);
    renderLine(s.c_str(), offsetX, x);
    */
}

void tabbar_t::renderLine(const char* line, int& offsetX, int& x)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        waddch(win, c);
        if (x++ >= viewWidth) {
            return;
        }
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
    viewHeight = TABBAR_HEIGHT;

    if (app_t::instance()->showSidebar) {
        int explorerWidth = app_t::instance()->explorer->viewWidth;
        viewWidth -= explorerWidth;
        viewX += explorerWidth;
    }
}

bool tabbar_t::processCommand(command_t cmdt, char ch)
{
    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();

    switch (cmd) {
    case CMD_TAB_0:
    case CMD_TAB_1:
    case CMD_TAB_2:
    case CMD_TAB_3:
    case CMD_TAB_4:
    case CMD_TAB_5:
    case CMD_TAB_6:
    case CMD_TAB_7:
    case CMD_TAB_8:
    case CMD_TAB_9: {
        app->openEditor(tabs[cmd - CMD_TAB_0 - 1].editor->document.fullPath);
        return true;
    }
    case CMD_NEW_TAB:
        app->openEditor("./untitled");
        return true;
    default:
        break;
    }

    // proceed only if got focus
    if (!isFocused()) {
        return false;
    }

    switch (cmd) {
    case CMD_MOVE_CURSOR_LEFT:
    case CMD_MOVE_CURSOR_RIGHT: {
        std::vector<struct tabitem_t>::iterator it = tabs.begin();
        std::vector<struct tabitem_t>::iterator prev = it;
        std::vector<struct tabitem_t>::iterator next = it;
        while (it != tabs.end()) {
            if (it != tabs.begin()) {
                prev = it - 1;
            }
            if (it + 1 != tabs.end()) {
                next = it + 1;
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
    case CMD_MOVE_CURSOR_UP:
    case CMD_MOVE_CURSOR_DOWN:
        app->focused = app->currentEditor.get();
        return true;
    default:
        break;
    }

    return false;
}
