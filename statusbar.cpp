#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "editor.h"
#include "app.h"
#include "statusbar.h"

#define STATUS_ITEMS 4

int pairForColor(int idx, bool selected);

void statusbar_t::setText(std::string text, int pos)
{
    if (!start.size()) {
        for (int i = 0; i < STATUS_ITEMS; i++) {
            start.push_back("");
            end.push_back("");
        }
    }

    if (pos < 0) {
        pos += STATUS_ITEMS + 1;
        end[pos % STATUS_ITEMS] = text;
    } else {
        start[pos % STATUS_ITEMS] = text;
    }
}

void statusbar_t::setStatus(std::string s, int f)
{
    status = s;
    frames = (f + 500) * 1000;
}

void statusbar_t::render()
{
    if (colorPair == 0) {
        style_t s = theme->styles_for_scope("comment");
        colorPair = pairForColor(s.foreground.index, false);
    }

    wattron(win, COLOR_PAIR(colorPair));
    wmove(win, 0, 0);

    // wclrtoeol(win);
    for (int i = 0; i < viewWidth; i++) {
        waddch(win, ' ');
    }

    wmove(win, 0, 0);

    int offset = 2;
    if (status.length()) {
        renderLine(status.c_str(), offset);
    } else {
        for (auto s : start) {
            renderLine(s.c_str(), offset);
            offset += s.length() + 2;
        }
    }

    offset = 2;
    for (auto s : end) {
        offset += s.length();
        renderLine(s.c_str(), viewWidth - offset);
        offset += 2;
    }
    wattroff(win, COLOR_PAIR(colorPair));
}

void statusbar_t::renderLine(const char* line, int offsetX)
{
    wmove(win, 0, offsetX);
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
    }
}

bool statusbar_t::tick(int tick)
{
    if (frames < 0) {
        return false;
    }

    frames = frames - tick;

    if (frames < 500) {
        status = "";
        return true;
    }

    bool res = prevStatus != status;
    prevStatus = status;
    return res;
}

void renderStatus(struct statusbar_t& statusbar)
{
    if (!app_t::instance()->showStatusBar) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor;

    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // calculate view
    //-----------------
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    statusbar.viewX = 0;
    statusbar.viewY = w.ws_row - 1;
    statusbar.viewWidth = w.ws_col;
    statusbar.viewHeight = 1;

    if (!statusbar.win) {
        statusbar.win = newwin(statusbar.viewHeight, statusbar.viewWidth, 0, 0);
    }

    mvwin(statusbar.win, statusbar.viewY, statusbar.viewX);
    wresize(statusbar.win, statusbar.viewHeight, statusbar.viewWidth);

    static char tmp[512];
    sprintf(tmp, "Line: %d Col: %d",
        1 + (int)(block.lineNumber),
        1 + (int)(cursor.position - block.position));

    statusbar.setText(doc->fileName, 0);
    statusbar.setText(tmp, -2);
    if (editor->lang) {
        statusbar.setText(editor->lang->id, -1);
    } else {
        statusbar.setText("", -1);
    }
    statusbar.render();
}