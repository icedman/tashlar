#include "tabbar.h"
#include "app.h"

#include <algorithm>
#include <curses.h>

void _clrtoeol(int w);

static bool compareFile(struct tabitem_t& f1, struct tabitem_t& f2)
{
    return f1.name < f2.name;
}

static void renderLine(const char* line, int& offsetX, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        // waddch(win, c);
        addch(c);
        if (x++ >= width) {
            return;
        }
    }
}

void tabbar_t::render()
{
    if (!app_t::instance()->showTabbar || !isVisible()) {
        return;
    }

    app_t* app = app_t::instance();
    move(y, x);

    bool hasFocus = isFocused();

    tabs.clear();
    if (!tabs.size()) {
        for (auto gem : app->editors) {
            editor_ptr e = gem->editor;
            struct tabitem_t item = {
                // .name = " " + e->document.fileName + " ",
                .name = e->document.fileName,
                .itemNumber = (int)tabs.size(),
                .width = (int)e->document.fileName.length() + 2,
                .editor = e
            };

            if (!item.name.length()) {
                item.name = "untitled";
            }
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
        if (t.editor == app->currentEditor) {
            currentTabX = tabX;
            currentTabWidth = t.width;
        }
        totalWidth += t.width;
        tabX += t.width;
    }

    while (currentTabX + currentTabWidth + 1 - scrollX > width) {
        scrollX++;
    }
    while (scrollX > 0 && currentTabX - scrollX <= 0) {
        scrollX--;
    }

    int _x = this->x;

    int offsetX = scrollX;
    int x = 0;

    move(y, _x);
    _clrtoeol(width);
    move(y, _x);

    int tabNo = 1;
    for (auto t : tabs) {
        int pair = colorPrimary;
        if (t.editor == app->currentEditor) {

            if (hasFocus) {
                attron(A_REVERSE);
                // pair = colorSecondary;
            }

            attron(COLOR_PAIR(colorIndicator));
            attron(A_BOLD);
            renderLine("[", offsetX, x, width);
            attroff(COLOR_PAIR(colorIndicator));
            attroff(A_BOLD);
        } else {
            renderLine(" ", offsetX, x, width);
        }

        attron(COLOR_PAIR(pair));
        renderLine(t.name.c_str(), offsetX, x, width);
        attroff(COLOR_PAIR(pair));

        if (t.editor == app->currentEditor) {

            attron(COLOR_PAIR(colorIndicator));
            attron(A_BOLD);
            renderLine("]", offsetX, x, width);
            attroff(COLOR_PAIR(colorIndicator));
            attroff(A_BOLD);

        } else {
            renderLine(" ", offsetX, x, width);
        }

        attroff(A_REVERSE);

        tabNo++;
    }
}