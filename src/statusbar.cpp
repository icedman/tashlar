#include "statusbar.h"
#include "app.h"
#include "editor.h"
#include "render.h"

#define STATUS_ITEMS 6

static struct statusbar_t* statusbarInstance = 0;

statusbar_t::statusbar_t()
    : view_t("statusbar")
    , frames(0)
{
    preferredHeight = 1;
    statusbarInstance = this;
    backgroundColor = 2;
}

struct statusbar_t* statusbar_t::instance()
{
    return statusbarInstance;
}

void statusbar_t::setText(std::string s, int pos, int size)
{
    sizes[pos] = size;
    text[pos] = s;
}

void statusbar_t::setStatus(std::string s, int f)
{
    status = s;
    frames = (f + 500) * 1000;
}

void statusbar_t::update(int delta)
{
    if (!app_t::instance()->showStatusBar) {
        return;
    }

    if (frames < 0) {
        return;
    }

    frames = frames - delta;
    if (frames < 500) {
        status = "";
    }
}

void statusbar_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool statusbar_t::isVisible()
{
    return visible && app_t::instance()->showStatusBar;
}

void statusbar_t::preLayout()
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

    int cols = width / getRenderer()->fw;
    int row = y / getRenderer()->fh;
    int col = x / getRenderer()->fw;

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
