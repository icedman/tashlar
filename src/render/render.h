#ifndef RENDER_H
#define RENDER_H

#include "view.h"

int _color_pair(int pair);

void _underline(bool);
void _bold(bool);
void _reverse(bool);

void _clrtoeol(int w);

void _move(int y, int x);
void _addch(char c);
void _addstr(const char *str);
void _addwstr(const wchar_t *str);
void _attron(int attr);
void _attroff(int attr);

void _begin(view_t *view);
void _end();

#endif // RENDER_H
