#include "popup.h"
#include "app.h"

#include <curses.h>

void _clrtoeol(int w);

static void renderLine(const char* line, int offsetX, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}

void popup_t::render()
{
    if (!app_t::instance()->showGutter || !isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    int _y = this->y;
    int _x = this->x;

    attron(COLOR_PAIR(colorPrimary));
    
    for (int i = 0; i < height - 1 || i == 0; i++) {
        move(_y + i, _x + 1);
        _clrtoeol(width - 2);
    }

    move(_y, _x);

    // wattron(win, COLOR_PAIR(colorPair));
    // box(win, ACS_VLINE, ACS_HLINE);
    // wattroff(win, COLOR_PAIR(colorPair));
    // box(win, ' ', ' ');

    if (!text.length()) {
        move(_y, _x + 3);
        addstr(placeholder.c_str());
    }

    move(_y, _x + 2);
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
    }

    app_t::log(">>%s %d", str, offsetX);
    renderLine(str, offsetX, x, width);
    
    addch('|');
    attroff(COLOR_PAIR(colorIndicator));
    attron(COLOR_PAIR(colorPrimary));

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
        move(_y + y++, _x + 1);
        x = 1;
        if (idx - 1 == currentItem) {
            attron(A_REVERSE);
            if (type == POPUP_FILES) {
                // app_t::instance()->statusbar->setStatus(item.fullPath, 8000);
            }
        }
        attron(COLOR_PAIR(colorPrimary));
        renderLine(item.name.c_str(), offsetX, x, width);
        
        for (int i = x; i < width - 1; i++) {
            addch(' ');
        }
        
        attroff(COLOR_PAIR(colorPrimary));
        attroff(A_REVERSE);
        if (y >= height - 1) {
            break;
        }
    }

    attroff(COLOR_PAIR(colorIndicator));
    attroff(COLOR_PAIR(colorPrimary));
}

