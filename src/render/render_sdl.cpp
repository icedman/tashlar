#include "app.h"
#include "render.h"
#include "renderer.h"

#include <SDL2/SDL.h>

static view_list contextStack;

SDL_Window* window;

void pushKey(char c, std::string keySequence);

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

    // app_t::log("%d %d", dm.w, dm.h);

    window = SDL_CreateWindow(
        "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w * 0.75, dm.h * 0.75,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);

    ren_init(window);
}

void render_t::shutdown()
{
    SDL_DestroyWindow(window);
}

static int poll_event()
{

    char buf[16];
    int mx, my, wx, wy;
    SDL_Event e;

    if (!SDL_PollEvent(&e)) {
        return 0;
    }

    switch (e.type) {
    case SDL_QUIT:
        pushKey(0, "ctrl+q");
        return 1;

    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
            return 3;
        } else if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {
            return 1;
        }
        /* on some systems, when alt-tabbing to the window SDL will queue up
      ** several KEYDOWN events for the `tab` key; we flush all keydown
      ** events on focus so these are discarded */
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            SDL_FlushEvent(SDL_KEYDOWN);
        }
        return 0;

    case SDL_DROPFILE:
        SDL_GetGlobalMouseState(&mx, &my);
        SDL_GetWindowPosition(window, &wx, &wy);
        SDL_free(e.drop.file);
        return 4;

    case SDL_KEYDOWN:
        app_t::log("key down");
        pushKey(0, "ctrl+q");
        return 2;

    case SDL_KEYUP:
        return 2;

    case SDL_TEXTINPUT:
        return 2;

    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == 1) {
            SDL_CaptureMouse((SDL_bool)1);
        }
        return 5;

    case SDL_MOUSEBUTTONUP:
        return 4;

    case SDL_MOUSEMOTION:
        return 5;

    case SDL_MOUSEWHEEL:
        return 2;

    default:
        return 0;
    }

    return 0;
}

void render_t::update(int delta)
{
    // app_t::log("event!");

    // SDL_WaitEventTimeout(NULL, 1 * 1000);

    poll_event();
}

void render_t::render()
{
    // app_t::log("render!");

    RenRect rects[] = {
        { .x = 0, .y = 0, .width = 200, .height = 200 }
    };

    ren_update_rects(rects, 1);
}

void render_t::updateColors()
{
}
