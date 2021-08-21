#include "statusbar_view.h"
#include "statusbar.h"
#include "render.h"
#include "app.h"
#include "util.h"
#include "block.h"

statusbar_view_t::statusbar_view_t()
	: view_t("statusbar")
{
	preferredHeight = 1;
    backgroundColor = 2;
    canFocus = false;
}

statusbar_view_t::~statusbar_view_t()
{}

void statusbar_view_t::update(int delta)
{
	statusbar_t::instance()->update(delta);
}

void statusbar_view_t::preLayout()
{
    preferredHeight = getRenderer()->fh;
    if (!getRenderer()->isTerminal()) {
        preferredHeight += (padding * 2);
    }
}

static void renderLine(const char* line, int offsetY, int offsetX, int size, int width)
{
    // TODO
    int row = offsetY / render_t::instance()->fh;

    // log("%s %d", line, size);
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

void statusbar_view_t::render()
{
    if (!isVisible()) {
        return;
    }

    // log("statusbar h:%d %d", height, preferredHeight);

    app_t* app = app_t::instance();

    statusbar_t *statusbar = statusbar_t::instance();
    editor_ptr editor = app->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    int cols = width / getRenderer()->fw;
    int row = y / getRenderer()->fh;
    int col = x / getRenderer()->fw;

    _move(0, col);

    static char tmp[512];
    // sprintf(tmp, "History %d/%d", (int)doc->snapShots.size(), (int)doc->snapShots.back().edits.size());
    // setText(tmp, -5);

    sprintf(tmp, "%s", doc->windowsLineEnd ? "CR/LF" : "LF");
    statusbar->setText(tmp, -4);

    sprintf(tmp, "Line: %d", 1 + (int)(block->lineNumber));
    statusbar->setText(tmp, -3);
    sprintf(tmp, "Col: %d", 1 + (int)(cursor.position()));
    statusbar->setText(tmp, -2);
    if (editor->highlighter.lang) {
        statusbar->setText(editor->highlighter.lang->id, -1);
    } else {
        statusbar->setText("", -1);
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
    if (statusbar->status.length()) {
        renderLine(statusbar->status.c_str(), y, x + offset, statusbar->sizes[0], cols);
    } else {
        for (int i = 0; i < STATUS_ITEMS; i++) {
            std::string s = statusbar->text[i];
            renderLine(s.c_str(), y, x + offset, statusbar->sizes[i], cols);
            offset += s.length() + 2;
        }
    }

    offset = 2;
    for (int i = 0; i < STATUS_ITEMS; i++) {
        int idx = -1 + (i * -1);
        std::string s = statusbar->text[idx];
        offset += s.length();
        renderLine(s.c_str(), 0, x + cols - offset, statusbar->sizes[idx], cols);
        offset += 2;
    }
    _attroff(_color_pair(colorPrimary));
    _reverse(false);
}

void statusbar_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}
