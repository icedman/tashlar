#include "tabbar.h"
#include "app.h"
#include "render.h"

#include <algorithm>

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
        _addch(c);
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

    // app_t::log("tabbar h:%d %d", height, preferredHeight);

    app_t* app = app_t::instance();
    _move(0, 0);

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

    while (currentTabX + currentTabWidth + 1 - scrollX > cols) {
        scrollX++;
    }
    while (scrollX > 0 && currentTabX - scrollX <= 0) {
        scrollX--;
    }

    int offsetX = scrollX;
    int x = 0;

    _move(0, 0);
    _clrtoeol(cols);
    _move(0, 0);

    int tabNo = 1;

    for (auto t : tabs) {
        int pair = colorPrimary;
        _attron(_color_pair(pair));
        if (t.editor == app->currentEditor) {

            if (hasFocus) {
                _bold(true);
                // pair = colorSecondary;
            }

            _attron(_color_pair(colorIndicator));
            _bold(true);
            _reverse(true);
            renderLine(" ", offsetX, x, cols);
            // _attroff(_color_pair(colorIndicator));
            _bold(false);
        } else {
            _attron(_color_pair(pair));
            renderLine(" ", offsetX, x, cols);
        }

        renderLine(t.name.c_str(), offsetX, x, cols);

        if (t.editor == app->currentEditor) {

            _attron(_color_pair(colorIndicator));
            _bold(true);
            renderLine(" ", offsetX, x, cols);
            _attroff(_color_pair(colorIndicator));
            _bold(false);
            _reverse(false);

        } else {
            renderLine(" ", offsetX, x, cols);
        }

        _bold(false);
        _attroff(_color_pair(pair));

        tabNo++;
    }

    _reverse(false);
}
