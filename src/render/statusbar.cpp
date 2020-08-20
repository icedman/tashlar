#include "statusbar.h"
#include "app.h"
#include "document.h"
#include "render.h"

#define STATUS_ITEMS 6

static void renderLine(const char* line, int offsetY, int offsetX, int size, int width)
{
    int row = offsetY / render_t::instance()->fh;

    // app_t::instance()->log("%s %d", line, size);
    _move(0, offsetX);

    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        _addch(c);
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
    if (!isVisible()) {
        return;
    }

    // app_t::log("statusbar h:%d %d", height, preferredHeight);

    app_t* app = app_t::instance();

    editor_ptr editor = app->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    int cols = width / render_t::instance()->fw;
    int row = y / render_t::instance()->fh;
    int col = x / render_t::instance()->fw;

    _move(0, col);

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

    _attron(_color_pair(colorPrimary));
    // _reverse(true);

    _move(0, col);
    _clrtoeol(cols);
    _move(0, col);

    int offset = 2;
    if (status.length()) {
        renderLine(status.c_str(), y, x + offset, sizes[0], cols);
    } else {
        for (int i = 0; i < STATUS_ITEMS; i++) {
            std::string s = text[i];
            renderLine(s.c_str(), y, x + offset, sizes[i], cols);
            offset += s.length() + 2;
        }
    }

    offset = 2;
    for (int i = 0; i < STATUS_ITEMS; i++) {
        int idx = -1 + (i * -1);
        std::string s = text[idx];
        offset += s.length();
        renderLine(s.c_str(), 0, x + cols - offset, sizes[idx], cols);
        offset += 2;
    }
    _attroff(_color_pair(colorPrimary));
    _reverse(false);
}
