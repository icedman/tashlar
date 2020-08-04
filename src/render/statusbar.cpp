#include "statusbar.h"
#include "app.h"
#include "document.h"

#include <curses.h>

#define STATUS_ITEMS 6

void _clrtoeol(int w);

static void renderLine(const char* line, int offsetY, int offsetX, int size, int width)
{
    // app_t::instance()->log("%s %d", line, size);
    move(offsetY, offsetX);
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        addch(c);
        if (size != 0 && idx >= size) {
            return;
        }
        if (offsetX + idx >= width) {
            return;
        }
    }
}

void statusbar_t::render()
{
    if (!app_t::instance()->showStatusBar || !isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    editor_ptr editor = app->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    move(y, x);

    static char tmp[512];
    // sprintf(tmp, "History %d/%d", (int)doc->snapShots.size(), (int)doc->snapShots.back().edits.size());
    // setText(tmp, -5);

    sprintf(tmp, "%s", doc->windowsLineEnd ? "CR/LF" : "LF");
    setText(tmp, -4);

    sprintf(tmp, "Line: %d", 1 + (int)(block->lineNumber));
    setText(tmp, -3);
    sprintf(tmp, "Col: %d", 1 + (int)(cursor.position()));
    setText(tmp, -2);
    if (editor->highlighter.lang) {
        setText(editor->highlighter.lang->id, -1);
    } else {
        setText("", -1);
    }

    //-----------------
    // render the bar
    //-----------------

    attron(COLOR_PAIR(colorPrimary));

    move(y, x);
    _clrtoeol(width);
    move(y, x);

    int offset = 2;
    if (status.length()) {
        // renderLine(status.c_str(), offset, width);
    } else {
        for (int i = 0; i < STATUS_ITEMS; i++) {
            std::string s = text[i];
            renderLine(s.c_str(), y, x + offset, sizes[i], width);
            offset += s.length() + 2;
        }
    }

    offset = 2;
    for (int i = 0; i < STATUS_ITEMS; i++) {
        int idx = -1 + (i * -1);
        std::string s = text[idx];
        offset += s.length();
        renderLine(s.c_str(), y, x + width - offset, sizes[idx], width);
        offset += 2;
    }
    attroff(COLOR_PAIR(colorPrimary));
    attroff(A_REVERSE);
}