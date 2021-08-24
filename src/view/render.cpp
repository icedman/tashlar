#include "render.h"
#include "app.h"

#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static std::map<int, int> colorMap;
static int drawBaseX = 0;
static int drawBaseY = 0;

std::string getClipboardText()
{
    return "";
}

void setClipboardText(std::string text)
{}

int pairForColor(int colorIdx, bool selected)
{
    if (selected && colorIdx == color_pair_e::NORMAL) {
        return color_pair_e::SELECTED;
    }
    return colorMap[colorIdx + (selected ? SELECTED_OFFSET : 0)];
}

static view_list contextStack;

static view_t* context()
{
    return contextStack.back();
}

void _clrtoeol(int w)
{
    while (w-- > 0)
        addch(' ');
}

void _move(int y, int x)
{
    move(drawBaseY + y, drawBaseX + x);
}

void _addch(char c)
{
    addch(c);
}

void _addstr(const char* str)
{
    addstr(str);
}

void _addwstr(const wchar_t* str)
{
    addwstr(str);
}

void _attron(int attr)
{
    attron(attr);
}

void _attroff(int attr)
{
    attroff(attr);
}

bool _drawdots(int dots, int* colors)
{
    return false;
}

int _color_pair(int pair)
{
    return COLOR_PAIR(pair);
}

void _underline(bool b)
{
    if (b)
        attron(A_UNDERLINE);
    else
        attroff(A_UNDERLINE);
}

void _bold(bool b)
{
    if (b)
        attron(A_BOLD);
    else
        attroff(A_BOLD);
}

void _reverse(bool b)
{
    if (b)
        attron(A_REVERSE);
    else
        attroff(A_REVERSE);
}

void _italic(bool)
{
}

void _begin(view_t* view)
{
    contextStack.push_back(view);

    drawBaseX = view->x;
    drawBaseY = view->y;
}

void _end()
{
    contextStack.pop_back();
}

static struct render_t* renderInstance = 0;

struct render_t* render_t::instance()
{
    return renderInstance;
}

render_t::render_t()
{
    renderInstance = this;
}

render_t::~render_t()
{
}

void render_t::initialize()
{
    setlocale(LC_ALL, "");

    initscr();
    raw();
    noecho();
    nodelay(stdscr, true);

    use_default_colors();
    start_color();

    if (has_colors() && can_change_color()) {
        color_info_t::set_term_color_count(256);
    } else {
        color_info_t::set_term_color_count(8);
    }

    curs_set(0);
    clear();
}

void render_t::shutdown()
{
    endwin();
}

void render_t::update()
{
    static struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    width = ws.ws_col;
    height = ws.ws_row;
    cols = width;
    rows = height;
    fw = 1;
    fh = 1;
}

void render_t::render()
{
    refresh();
}

void render_t::input()
{
}

void render_t::updateColors()
{
    colorMap.clear();

    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    //---------------
    // build the color pairs
    //---------------
    init_pair(color_pair_e::NORMAL, app->fg, app->bg);
    init_pair(color_pair_e::SELECTED, app->selFg, app->selBg);

    int idx = 32;

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first] = idx;
        init_pair(idx++, it->first, app->bg);
        it++;
    }

    it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first + SELECTED_OFFSET] = idx;
        init_pair(idx++, it->first, app->selBg);
        if (it->first == app->selBg) {
            colorMap[it->first + SELECTED_OFFSET] = idx + 1;
        }
        it++;
    }
}

void render_t::delay(int ms)
{
}

bool render_t::isTerminal()
{
    return true;
}

int _keyMods()
{
    return 0;
}

bool _isShiftDown()
{
    return false;
}