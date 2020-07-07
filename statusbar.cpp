#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
    frames = f + 1;
}

void statusbar_t::render()
{
    int colorPair = color_pair_e::SELECTED;
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

bool statusbar_t::tick()
{
    if (frames == 0) {
        return false;
    }

    frames--;
    if (frames == 1) {
        status = "";
    }

    return true;
}