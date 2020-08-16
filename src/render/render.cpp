#include "render.h"

#include <curses.h>

static view_list contextStack;

static view_t *context()
{
    return contextStack.back();
}

void _clrtoeol(int w)
{
    while (w-- > 0)
        addch(' ');
}

void _move(int y, int x) {
    move(y, x);
}

void _addch(char c) {
    addch(c);
}

void _addstr(const char *str) {
    addstr(str);
}

void _addwstr(const wchar_t *str)
{
    addwstr(str);
}

void _attron(int attr) {
    attron(attr);
}

void _attroff(int attr) {
    attroff(attr);
}

int _color_pair(int pair)
{
    return COLOR_PAIR(pair);
}

void _underline(bool b) {
    if (b) attron(A_UNDERLINE); else attroff(A_UNDERLINE);
}

void _bold(bool b) {
    if (b) attron(A_BOLD); else attroff(A_BOLD);
}

void _reverse(bool b) {
    if (b) attron(A_REVERSE); else attroff(A_REVERSE);
}

void _begin(view_t *view)
{
    contextStack.push_back(view);
}

void _end()
{
    contextStack.pop_back();
}
