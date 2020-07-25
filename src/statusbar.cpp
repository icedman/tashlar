#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "history.h"
#include "statusbar.h"

#define STATUS_ITEMS 6

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

void statusbar_t::render()
{
    if (!app_t::instance()->showStatusBar) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    wattron(win, A_REVERSE);

    // setText(doc->fileName, 0);

    static char tmp[512];
    // sprintf(tmp, "History %d/%d", (int)doc->snapShots.size(), (int)doc->snapShots.back().edits.size());
    sprintf(tmp, "%s", doc->windowsLineEnd ? "CR/LF" : "LF");
    setText(tmp, -4);

    sprintf(tmp, "Line: %d", 1 + (int)(block.lineNumber));
    setText(tmp, -3);
    sprintf(tmp, "Col: %d", 1 + (int)(cursor.position() - block.position));
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
        for (int i = 0; i < STATUS_ITEMS; i++) {
            std::string s = text[i];
            renderLine(s.c_str(), offset, sizes[i]);
            offset += s.length() + 2;
        }
    }

    offset = 2;
    for (int i = 0; i < STATUS_ITEMS; i++) {
        int idx = -1 + (i * -1);
        std::string s = text[idx];
        offset += s.length();
        renderLine(s.c_str(), viewWidth - offset, sizes[idx]);
        offset += 2;
    }
    wattroff(win, COLOR_PAIR(colorPair));
    wattroff(win, A_REVERSE);
    wrefresh(win);
}

void statusbar_t::renderLine(const char* line, int offsetX, int size)
{
    // app_t::instance()->log("%s %d", line, size);
    wmove(win, 0, offsetX);
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
        if (size != 0 && idx >= size) {
            return;
        }
        if (offsetX + idx >= viewWidth) {
            return;
        }
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
