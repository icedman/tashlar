#include "render.h"

#include <SDL2/SDL.h>

static view_list contextStack;

SDL_Window* window;

int pairForColor(int colorIdx, bool selected)
{
    return 0;
}

static view_t* context()
{
    return contextStack.back();
}

void _clrtoeol(int w)
{
}

void _move(int y, int x)
{
}

void _addch(char c)
{
}

void _addstr(const char* str)
{
}

void _addwstr(const wchar_t* str)
{
}

void _attron(int attr)
{
}

void _attroff(int attr)
{
}

int _color_pair(int pair)
{
    return pair;
}

void _underline(bool b)
{
}

void _bold(bool b)
{
}

void _reverse(bool b)
{
}

void _begin(view_t* view)
{
    contextStack.push_back(view);
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
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_EnableScreenSaver();
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    atexit(SDL_Quit);

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    window = SDL_CreateWindow(
        "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w * 0.8, dm.h * 0.8,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
}

void render_t::shutdown()
{
    SDL_DestroyWindow(window);
}

void render_t::update(int delta)
{
}

void render_t::render()
{
}

void render_t::updateColors()
{
}
