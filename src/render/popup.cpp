#include "popup.h"
#include "app.h"
#include "render.h"

static void renderLine(const char* line, int offsetX, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        _addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}

void popup_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    int _y = this->y;
    int _x = this->x;

    _attron(_color_pair(colorPrimary));

    for (int i = 0; i < height - 1 || i == 0; i++) {
        _move(_y + i, _x + 1);
        _clrtoeol(width - 2);
    }

    _move(_y, _x);

    // w_attron(win, _color_pair(colorPair));
    // box(win, ACS_VLINE, ACS_HLINE);
    // wattroff(win, _color_pair(colorPair));
    // box(win, ' ', ' ');

    if (!text.length()) {
        _move(_y, _x + 2);
        _addstr(placeholder.c_str());
    }

    _move(_y, _x + 1);
    int offsetX = 0;
    int x = 1;

    if (currentItem < 0) {
        currentItem = 0;
    }
    if (currentItem >= items.size()) {
        currentItem = items.size() - 1;
    }
    char* str = (char*)text.c_str();
    if (text.length() > width - 3) {
        offsetX = text.length() - (width - 3);
    };
    renderLine(str, offsetX, x, width);

    _attron(_color_pair(colorIndicator));
    _addch('|');
    _attroff(_color_pair(colorIndicator));
    _attron(_color_pair(colorPrimary));

    int inputOffset = 2;
    if (type == POPUP_COMPLETION) {
        inputOffset -= 1;
    }

    // app_t::instance()->log("items: %d inputOffset: %d", items.size(), inputOffset);
    // scroll to cursor
    // TODO: use math not loops
    if (items.size()) {
        int viewportHeight = height - inputOffset;

        // app_t::instance()->log("items:%d current:%d scroll:%d h:%d", items.size(), currentItem, scrollY, viewportHeight);
        while (true) {
            int blockVirtualLine = currentItem;
            int blockScreenLine = blockVirtualLine - scrollY;
            if (blockScreenLine >= viewportHeight) {
                scrollY++;
            } else if (blockScreenLine <= 0) {
                scrollY--;
            } else {
                break;
            }
            break;
        }

    } else {
        scrollY = 0;
    }

    int skip = scrollY;

    int idx = 0;
    int y = (inputOffset - 1);
    for (auto& item : items) {
        idx++;
        if (skip-- > 0) {
            continue;
        }
        _move(_y + y++, _x + 1);
        x = 1;
        if (idx - 1 == currentItem) {
            _bold(true);
            if (type == POPUP_FILES) {
                // app_t::instance()->statusbar->setStatus(item.fullPath, 8000);
            }
        }
        _attron(_color_pair(colorPrimary));
        renderLine(item.name.c_str(), offsetX, x, width);

        for (int i = x; i < width - 1; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(colorPrimary));
        _bold(false);
        if (y >= height - 1) {
            break;
        }
    }

    _attroff(_color_pair(colorIndicator));
    _attroff(_color_pair(colorPrimary));
}
