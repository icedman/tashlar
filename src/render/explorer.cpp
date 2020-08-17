#include "explorer.h"
#include "app.h"
#include "document.h"
#include "editor.h"
#include "render.h"

static void renderLine(const char* line, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        _addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}

void explorer_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    editor_ptr editor = app_t::instance()->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(y, x);

    if (renderList.size() == 0) {
        buildFileList(renderList, &files, 0);
    }

    bool hasFocus = isFocused();
    if (currentItem == -1) {
        currentItem = 0;
    }

    // scroll to cursor
    // TODO: use math not loops
    while (true && height > 0) {
        int blockVirtualLine = currentItem;
        int blockScreenLine = blockVirtualLine - scrollY;

        // app_t::instance()->log(">%d %d scroll:%d items:%d h:%d", blockScreenLine, currentItem, scrollY, renderList.size(), height);

        if (blockScreenLine + 1 >= height) {
            scrollY++;
        } else if (blockScreenLine < 0) {
            scrollY--;
        } else {
            break;
        }
    }

    int idx = 0;
    int skip = scrollY;
    int y = 0;
    for (auto file : renderList) {
        if (skip-- > 0) {
            idx++;
            continue;
        }

        int pair = colorPrimary;
        _move(y, x);

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _reverse(true);
            } else {
                _bold(true);
            }
            for (int i = 0; i < width; i++) {
                _addch(' ');
            }
        }

        int x = 0;
        _move(y++, x);
        _attron(_color_pair(pair));
        int indent = file->depth;
        for (int i = 0; i < indent; i++) {
            _addch(' ');
            x++;
        }
        if (file->isDirectory) {
            _attron(_color_pair(colorIndicator));

#ifdef ENABLE_UTF8
            _addwstr(file->expanded ? L"\u2191" : L"\u2192");
#else
            _addch(file->expanded ? '-' : '+');
#endif

            _attroff(_color_pair(colorIndicator));
            _attron(_color_pair(pair));
        } else {
            _addch(' ');
        }
        _addch(' ');
        x += 2;

        renderLine(file->name.c_str(), x, width);

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _bold(true);
            } else {
                _bold(true);
            }
        }
        for (int i = 0; i < width - x; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(pair));
        _bold(false);
        _reverse(false);

        if (y >= height) {
            break;
        }

        idx++;
    }

    while (y < height) {
        _move(y++, x);
        _clrtoeol(width);
    }
}
