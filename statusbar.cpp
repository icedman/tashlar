#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "statusbar.h"

#define STATUS_ITEMS 4

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
    if (!app_t::instance()->showStatusBar) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    static char tmp[512];
    sprintf(tmp, "Line: %d Col: %d",
        1 + (int)(block.lineNumber),
        1 + (int)(cursor.position - block.position));

    setText(doc->fileName, 0);
    setText(tmp, -2);
    if (editor->lang) {
        setText(editor->lang->id, -1);
    } else {
        setText("", -1);
    }

    //-----------------
    // render the bar
    //-----------------
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

    wrefresh(win);
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

void statusbar_t::update(int tick)
{
    if (!app_t::instance()->showStatusBar) {
        return;
    }
    
    if (frames < 0) {
        return;
    }

    frames = frames - tick;
    if (frames < 500) {
        status = "";
    }

    if (prevStatus != status) {
        render();
    }
    prevStatus = status;
}

void statusbar_t::layout(int w, int h)
{
    if (!app_t::instance()->showStatusBar) {
        viewHeight = 0;
        return;
    }
    viewX = 0;
    viewY = h - 1;
    viewWidth = w;
    viewHeight = 1;
}

