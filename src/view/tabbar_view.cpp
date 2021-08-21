#include "tabbar_view.h"
#include "app.h"
#include "editor.h"
#include "operation.h"
#include "render.h"
#include "gem_view.h"

#include <algorithm>

#define TABBAR_HEIGHT 1

tabbar_view_t::tabbar_view_t()
    : view_t("tabbar")
{
    preferredHeight = 1;
    canFocus = true;
    backgroundColor = 3;
}

void tabbar_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool tabbar_view_t::input(char ch, std::string keys)
{
    struct app_t* app = app_t::instance();

    operation_e cmd = operationFromKeys(keys);

    switch (cmd) {
    case TAB_1:
    case TAB_2:
    case TAB_3:
    case TAB_4:
    case TAB_5:
    case TAB_6:
    case TAB_7:
    case TAB_8:
    case TAB_9: {
        int idx = cmd - TAB_1;
        if (idx < tabs.size()) {
            focusGem(app->openEditor(tabs[idx].editor->document.fullPath), true);
        }
        return true;
    }
    case NEW_TAB:
        focusGem(app->openEditor("./"), true);
        return true;
    default:
        break;
    }

    // proceed only if got focus
    if (!isFocused()) {
        return false;
    }

    switch (cmd) {
    case MOVE_CURSOR_LEFT:
    case MOVE_CURSOR_RIGHT: {
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
            if (it->editor == app->currentEditor) {
                if (cmd == MOVE_CURSOR_LEFT) {
                    it = prev;
                } else {
                    it = next;
                }
                if (it != tabs.end()) {
                    focusGem(app->openEditor(it->editor->document.fullPath), true);
                    view_t::setFocus(this);
                }
                return true;
            }
            it++;
        }
        break;
    };
    case ENTER:
    case MOVE_CURSOR_UP:
    case MOVE_CURSOR_DOWN:
        focusGem(app->currentEditor, true);
        return true;
    default:
        break;
    }

    return false;
}

void tabbar_view_t::mouseDown(int x, int y, int button, int clicks)
{
    int fw = getRenderer()->fw;
    int col = (x - this->x - padding) / fw;
    for (auto& t : tabs) {
        if (col >= t.x && col < t.x + t.width) {
            // log("tab %s", t.name.c_str());
            focusGem(app_t::instance()->openEditor(t.editor->document.fullPath), true);
            return;
        }
    }
}

void tabbar_view_t::mouseHover(int x, int y)
{
    // mouseDown(x, y, 1, 0);
}

bool tabbar_view_t::isVisible()
{
    return visible && app_t::instance()->showTabbar;
}

void tabbar_view_t::preLayout()
{
    preferredHeight = getRenderer()->fh;
    if (!getRenderer()->isTerminal()) {
        preferredHeight += (padding * 2);
    }
}
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

void tabbar_view_t::render()
{
    if (!app_t::instance()->showTabbar || !isVisible()) {
        return;
    }

    // log("tabbar h:%d %d", height, preferredHeight);

    app_t* app = app_t::instance();
    _move(0, 0);

    bool hasFocus = isFocused();

    tabs.clear();
    if (!tabs.size()) {
        for (auto gem : gemList()) {
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
                item.width = 10;
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

    /*
    while (currentTabX + currentTabWidth + 1 - scrollX > cols) {
        scrollX++;
    }
    while (scrollX > 0 && currentTabX - scrollX <= 0) {
        scrollX--;
    }
    */

    int offsetX = scrollX;
    int x = 0;

    _move(0, 0);
    _clrtoeol(cols);
    _move(0, 0);

    int tabNo = 1;

    for (auto& t : tabs) {
        t.x = x - scrollX;
        int pair = colorPrimary;
        _attron(_color_pair(pair));
        if (t.editor == app->currentEditor) {
            pair = colorIndicator;
            if (hasFocus) {
                _bold(true);
                _reverse(true);
            }

            _attron(_color_pair(pair));
            _bold(true);
            renderLine(" ", offsetX, x, cols);
            // _attroff(_color_pair(colorIndicator));
            _bold(false);
        } else {
            _attron(_color_pair(pair));
            renderLine(" ", offsetX, x, cols);
        }

        renderLine(t.name.c_str(), offsetX, x, cols);

        if (t.editor == app->currentEditor) {
            _attron(_color_pair(pair));
            _bold(true);
            renderLine(" ", offsetX, x, cols);
            _attroff(_color_pair(pair));
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
