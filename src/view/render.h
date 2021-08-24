#ifndef RENDER_H
#define RENDER_H

#include "view.h"
#define SELECTED_OFFSET 500
   
int _color_pair(int pair);

void _underline(bool);
void _bold(bool);
void _reverse(bool);
void _italic(bool);

void _clrtoeol(int w);

void _move(int y, int x);
void _addch(char c);
void _addstr(const char* str);
void _addwstr(const wchar_t* str);
void _attron(int attr);
void _attroff(int attr);
bool _drawdots(int dots, int* colors);

void _begin(view_t* view);
void _end();
int _keyMods();

bool _isShiftDown();

struct render_t {

    render_t();
    ~render_t();

    void initialize();
    void shutdown();
    void update();
    void render();
    void updateColors();
    void input();
    void delay(int ms);
    bool isTerminal();

    std::string getClipboardText();
    void setClipboardText(std::string text);

    int pairForColor(int colorIdx, bool selected);

    static render_t* instance();

    int width;
    int height;
    int rows;
    int cols;
    int fw;
    int fh;
};

#endif // RENDER_H
