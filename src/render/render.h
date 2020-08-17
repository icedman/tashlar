#ifndef RENDER_H
#define RENDER_H

#include "view.h"
#define SELECTED_OFFSET 500

int _color_pair(int pair);

void _underline(bool);
void _bold(bool);
void _reverse(bool);

void _clrtoeol(int w);

void _move(int y, int x);
void _addch(char c);
void _addstr(const char* str);
void _addwstr(const wchar_t* str);
void _attron(int attr);
void _attroff(int attr);

void _begin(view_t* view);
void _end();

struct render_t {

    render_t();
    ~render_t();

    void initialize();
    void shutdown();
    void update(int delta);
    void render();
    void updateColors();

    static render_t* instance();

    int width;
    int height;
};

#endif // RENDER_H
