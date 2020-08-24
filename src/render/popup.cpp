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
    // app_t::log("%d %d %d %d ", x, y, width, height);

    _attron(_color_pair(colorPrimary));

    for (int i = 0; i < height - 1 || i == 0; i++) {
        _move(0 + i, 0);
        _clrtoeol(cols);
    }

    int inputOffset = 2;
    if (type == POPUP_COMPLETION) {
        inputOffset -= 1;
    } else {
        _move(0, 0);
        // fill input text background
        for (int i = 0; i < cols; i++) {
            _addch(' ');
        }
    }

    _move(0, 0);
    if (!text.length()) {
        _move(0, 0 + 1);
        _addstr(placeholder.c_str());
    }

    _move(0, 0);
    int offsetX = 0;
    int x = 0;

    if (currentItem < 0) {
        currentItem = 0;
    }
    if (currentItem >= items.size()) {
        currentItem = items.size() - 1;
    }
    char* str = (char*)text.c_str();
    if (text.length() > cols - 3) {
        offsetX = text.length() - (cols - 3);
    };
    renderLine(str, offsetX, x, cols);

    _attron(_color_pair(colorIndicator));
    _addch('|');
    _attroff(_color_pair(colorIndicator));
    _attron(_color_pair(colorPrimary));

    int skip = (scrollY / render_t::instance()->fh);
    int idx = 0;
    int y = (inputOffset - 1);
    for (auto& item : items) {
        idx++;
        if (skip-- > 0) {
            continue;
        }
        _move(y++, 0);
        x = 1;
        if (idx - 1 == currentItem) {
            _reverse(true);
            if (type == POPUP_FILES) {
                // app_t::instance()->statusbar->setStatus(item.fullPath, 8000);
            }
        }
        _attron(_color_pair(colorPrimary));
        renderLine(item.name.c_str(), offsetX, x, cols);

        for (int i = x; i < cols + 1; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(colorPrimary));
        _reverse(false);
        if (y >= rows) {
            break;
        }
    }

    _attroff(_color_pair(colorIndicator));
    _attroff(_color_pair(colorPrimary));
}
