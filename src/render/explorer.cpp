#include "explorer.h"
#include "app.h"
#include "document.h"
#include "editor.h"

#include <curses.h>

static void renderLine(const char* line, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}

void explorer_t::render()
{
    if (!app_t::instance()->showSidebar || !isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    editor_ptr editor = app_t::instance()->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    move(y, x);

    if (renderList.size() == 0) {
        buildFileList(renderList, &files, 0);
    }

    bool hasFocus = isFocused();
    if (currentItem == -1) {
        currentItem = 0;
    }

    // scroll to cursor
    // TODO: use math not loops
    while (true) {
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
        move(y, x);

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                attron(A_REVERSE);
            } else {
                attron(A_BOLD);
            }
            for (int i = 0; i < width; i++) {
                addch(' ');
            }
        }

        int x = 0;
        move(y++, x);
        attron(COLOR_PAIR(pair));
        int indent = file->depth;
        for (int i = 0; i < indent; i++) {
            addch(' ');
            x++;
        }
        if (file->isDirectory) {
            attron(COLOR_PAIR(colorIndicator));

#ifdef ENABLE_UTF8
            addwstr(file->expanded ? L"\u2191" : L"\u2192");
#else
            addch(file->expanded ? '-' : '+');
#endif

            attroff(COLOR_PAIR(colorIndicator));
            attron(COLOR_PAIR(pair));
        } else {
            addch(' ');
        }
        addch(' ');
        x += 2;

        renderLine(file->name.c_str(), x, width);
        //for (int i = 0; i < width - x; i++) {
        //    waddch(win, ' ');
        //}

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                attron(A_REVERSE);
            } else {
                attron(A_BOLD);
            }
        }
        for (int i = 0; i < width - x; i++) {
            addch(' ');
        }

        attroff(COLOR_PAIR(pair));
        attroff(A_REVERSE);
        attroff(A_BOLD);

        if (y >= height) {
            break;
        }

        idx++;
    }

    while (y < height) {
        move(y++, x);
        for (int i = 0; i < width; i++) {
            addch(' ');
        }
    }
}
