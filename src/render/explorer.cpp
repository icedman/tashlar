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

    // app_t::log("explorer h:%d", height);

    app_t* app = app_t::instance();

    editor_ptr editor = app_t::instance()->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(0, 0);

    if (regenerateList) {
        regenerateList = false;
        allFiles.clear();
        renderList.clear();
        buildFileList(renderList, &files, 0);
    }

    bool isHovered = view_t::currentHovered() == this;
    bool hasFocus = isFocused() || isHovered;
    if (currentItem == -1) {
        currentItem = 0;
    }

    int idx = 0;
    int skip = (scrollY / render_t::instance()->fh);
    int y = 0;
    for (auto file : renderList) {
        if (skip-- > 0) {
            idx++;
            continue;
        }

        int pair = colorPrimary;
        _move(y, 0);
        _attron(_color_pair(pair));

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _reverse(true);
            } else {
                _bold(true);
            }
            for (int i = 0; i < cols; i++) {
                _addch(' ');
            }
        }

        if (file->fullPath == app_t::instance()->currentEditor->document.fullPath) {
            pair = colorIndicator;
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

        renderLine(file->name.c_str(), x, cols);

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _bold(true);
            } else {
                _bold(true);
            }
        }
        for (int i = 0; i < cols - x; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(pair));
        _bold(false);
        _reverse(false);

        if (y >= rows + 1) {
            break;
        }

        idx++;
    }

    while (y < rows) {
        _move(y++, x);
        _clrtoeol(cols);
    }
}
