#include "app.h"
#include "dots.h"
#include "rencache.h"
#include "render.h"
#include "renderer.h"

#include <SDL2/SDL.h>

static view_list contextStack;

#define _draw_rect rencache_draw_rect
#define _draw_text rencache_draw_text

SDL_Window* window;
RenFont* font;

static float scrollVelocity = 0;

static int drawBaseX = 0;
static int drawBaseY = 0;
static int drawX = 0;
static int drawY = 0;
static int drawColorPair = 0;
static RenColor drawColor;
static RenColor drawBg;
static bool drawBold = false;
static bool drawReverse = false;
static bool drawUnderline = false;

static view_t* dragView = 0;
static int dragX = 0;
static int dragY = 0;

static std::map<int, int> colorMap;

RenColor bgColor;
struct BgFg {
    color_info_t bg;
    color_info_t fg;
};

static std::map<int, BgFg> colorPairs;

void pushKey(char c, std::string keySequence);

int pairForColor(int colorIdx, bool selected)
{
    if (selected && colorIdx == color_pair_e::NORMAL) {
        return color_pair_e::SELECTED;
    }
    return colorMap[colorIdx + (selected ? SELECTED_OFFSET : 0)];
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
    drawX = x;
    drawY = y;
}

void _addch(char c)
{
    int fw = render_t::instance()->fw;
    int fh = render_t::instance()->fh;

    int txt[] = { c, 0 };
    int cw = ren_get_font_width(font, (char*)txt);

    RenColor bg = drawBg;
    RenColor fg = drawColor;

    if (drawReverse) {
        fg = drawBg;
        bg = drawColor;
    }

    // background
    RenRect rect = { .x = drawBaseX + drawX * fw, .y = drawBaseY + drawY * fh, .width = fw, .height = fh };
    if (bg.r != bgColor.r || bg.g != bgColor.g || bg.b != bgColor.b) {
        _draw_rect(rect, bg);
        // app_t::log("bg: %d %d %d (%d %d %d)", bg.r, bg.g, bg.b, bgColor.r, bgColor.g, bgColor.b);
    }
    _draw_text(font, (char*)txt, drawBaseX + (drawX * fw) + (fw / 2 - cw / 2), drawBaseY + (drawY * fh), fg);

    if (drawUnderline) {
        rect.y += (rect.height - 2);
        rect.height = 1;
        _draw_rect(rect, fg);
    }

    drawX++;
}

bool _drawdots(int dots)
{
    static const int offs[] = {
        0, 0, 0, 1,
        1, 0, 1, 1,
        2, 0, 2, 1,
        3, 0, 3, 1
    };

    int* dm = dotMap();
    int fw = render_t::instance()->fw;
    int fh = render_t::instance()->fh;
    int dw = 2;
    RenColor fg = drawColor;

    float ffw = fw / 2;
    float ffh = fh / 4;

    for (int i = 0; i < 8; i++) {
        if (dots & dm[i]) {
            int ofx = offs[i * 2 + 1] * ffw;
            int ofy = offs[i * 2] * ffh;
            RenRect rect = {
                .x = ofx + drawBaseX + drawX * fw,
                .y = ofy + drawBaseY + drawY * fh,
                .width = dw,
                .height = dw
            };
            _draw_rect(rect, fg);
        }
    }

    drawX++;
    return true;
}

void _addstr(const char* str)
{
    int l = strlen(str);
    for (int i = 0; i < l; i++) {
        _addch(str[i]);
    }
}

void _addwstr(const wchar_t* str)
{
    // unsupported
    drawX++;
}

void _attron(int attr)
{
}

void _attroff(int attr)
{
}

int _color_pair(int pair)
{
    drawColorPair = pair;
    BgFg bgFg = colorPairs[drawColorPair];

    RenColor color = {
        .b = bgFg.fg.blue,
        .g = bgFg.fg.green,
        .r = bgFg.fg.red,
        .a = bgFg.fg.alpha
    };

    drawColor = color;

    RenColor bg = {
        .b = bgFg.bg.blue,
        .g = bgFg.bg.green,
        .r = bgFg.bg.red,
        .a = bgFg.bg.alpha
    };

    drawBg = bg;
    return drawColorPair;
}

void _underline(bool b)
{
    drawUnderline = b;
}

void _bold(bool b)
{
    drawBold = b;
}

void _reverse(bool b)
{
    drawReverse = b;
}

void _begin(view_t* view)
{
    contextStack.push_back(view);
    RenRect rect = {
        .x = view->x,
        .y = view->y,
        .width = view->width,
        .height = view->height,
    };
    drawBaseX = view->x;
    drawBaseY = view->y;

    if (view->isVisible() && view->backgroundColor != 0) {
        int off = 5 * view->backgroundColor;
        int bb = bgColor.b - off;
        int gg = bgColor.g - off;
        int rr = bgColor.r - off;
        if (bb < 0)
            bb = 0;
        if (gg < 0)
            gg = 0;
        if (rr < 0)
            rr = 0;
        if (bb > 255)
            bb = 255;
        if (gg > 255)
            gg = 255;
        if (rr > 255)
            rr = 255;
        RenColor bg = {
            .b = bb,
            .g = gg,
            .r = rr,
            .a = 255
        };
        rencache_draw_rect(rect, bg);
    }
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
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    atexit(SDL_Quit);

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    width = dm.w * 0.75;
    height = dm.h * 0.75;

    window = SDL_CreateWindow(
        "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);

    ren_init(window);
    // rencache_show_debug(true);

    // font = ren_load_font("./fonts/font.ttf", 16);
    font = ren_load_font("./fonts/FiraCode-Regular.ttf", 16);
    // font = ren_load_font("./fonts/monospace.ttf", 16);
    if (font) {
        app_t::log("font loaded");
        fw = ren_get_font_width(font, "1234567890AaBbCcDdEeFfGg") / 24;
        fh = ren_get_font_height(font);
    }
}

void render_t::shutdown()
{
    SDL_DestroyWindow(window);
    if (font) {
        ren_free_font(font);
    }
}

static int poll_event()
{
    if (scrollVelocity != 0) {
        view_t::currentHovered()->scroll(scrollVelocity);
        pushKey(0, "wheel");
        float d = -scrollVelocity / 2;
        scrollVelocity += d;
        if (-1 < d && d < 1) {
            scrollVelocity = 0;
        }
    }

    char buf[16];
    int mx, my, wx, wy;
    SDL_Event e;

    if (!SDL_PollEvent(&e)) {
        return 0;
    }

    switch (e.type) {
    case SDL_QUIT:
        pushKey(0, "ctrl+q");
        return 0;

    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
            if (e.window.data1 && e.window.data2) {
                // SDL_SetWindowSize(window, e.window.data1, e.window.data2);

                SDL_GetWindowSize(window, &render_t::instance()->width, &render_t::instance()->height);
                ren_init(window);
                pushKey(0, "resize");
                // render_t::instance()->width = e.window.data1;
                // render_t::instance()->height = e.window.data2;
            }
            return 0;
        } else if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {
            return 0;
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
        return 0;

    case SDL_KEYDOWN: {
        std::string keySequence;
        std::string mod;

        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
            pushKey(27, "");
            return 0;
        case SDLK_TAB:
            keySequence = "tab";
            break;
        case SDLK_HOME:
            keySequence = "home";
            break;
        case SDLK_END:
            keySequence = "end";
            break;
        case SDLK_PAGEUP:
            keySequence = "pageup";
            break;
        case SDLK_PAGEDOWN:
            keySequence = "pagedown";
            break;
        case SDLK_RETURN:
            keySequence = "enter";
            break;
        case SDLK_BACKSPACE:
            keySequence = "backspace";
            break;
        case SDLK_DELETE:
            keySequence = "delete";
            break;
        case SDLK_LEFT:
            keySequence = "left";
            break;
        case SDLK_RIGHT:
            keySequence = "right";
            break;
        case SDLK_UP:
            keySequence = "up";
            break;
        case SDLK_DOWN:
            keySequence = "down";
            break;
        default:
            if (e.key.keysym.sym >= SDLK_a && e.key.keysym.sym <= SDLK_z) {
                keySequence += (char)(e.key.keysym.sym - SDLK_a) + 'a';
            } else if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9) {
                keySequence += (char)(e.key.keysym.sym - SDLK_1) + '1';
            } else if (e.key.keysym.sym == SDLK_0) {
                keySequence += (char)'0';
            }
            break;
        }

        if (keySequence.length()) {
            if (e.key.keysym.mod & KMOD_CTRL) {
                mod = "ctrl";
            }
            if (e.key.keysym.mod & KMOD_SHIFT) {
                if (mod.length())
                    mod += "+";
                mod += "shift";
            }
            if (e.key.keysym.mod & KMOD_ALT) {
                if (mod.length())
                    mod += "+";
                mod += "alt";
            }
            if (mod.length()) {
                keySequence = mod + "+" + keySequence;
            }

            if (keySequence.length() > 1) {
                pushKey(0, keySequence);
                app_t::log("keydown %s", keySequence.c_str());
            }
        }

        return 0;
    }

    case SDL_KEYUP:
        return 0;

    case SDL_TEXTINPUT:
        app_t::log("text input %c", e.text.text[0]);
        pushKey(e.text.text[0], "");
        return 0;

    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == 1) {
            SDL_CaptureMouse((SDL_bool)1);
        }
        pushKey(0, "mousedown");
        dragView = view_t::currentHovered();
        view_t::currentHovered()->mouseDown(e.button.x, e.button.y, e.button.button, e.button.clicks);
        return 0;

    case SDL_MOUSEBUTTONUP:
        pushKey(0, "mouseup");
        if (view_t::currentHovered() == dragView) {
            view_t::currentHovered()->mouseUp(e.button.x, e.button.y, e.button.button);
        } else {
            view_t::currentHovered()->mouseUp(-1, -1, e.button.button);
        }
        dragView = 0;
        return 0;

    case SDL_MOUSEMOTION: {
        view_t::setHovered(app_t::instance()->viewFromPointer(e.motion.x, e.motion.y));
        if (view_t::currentHovered() == dragView) {
            view_t::currentHovered()->mouseDrag(e.motion.x, e.motion.y);
            app_t::log("motion %d %d", e.motion.x, e.motion.y);
            pushKey(0, "mousemove");
        }
        return 0;
    }

    case SDL_MOUSEWHEEL:
        scrollVelocity += (e.wheel.y * 2);
        return 0;

    default:
        return 0;
    }

    return 0;
}

void render_t::update(int delta)
{
    RenRect rect = { .x = 0, .y = 0, .width = width, .height = height };
    BgFg bgFg = colorPairs[color_pair_e::NORMAL];
    bgColor = {
        .b = bgFg.bg.blue,
        .g = bgFg.bg.green,
        .r = bgFg.bg.red,
        .a = bgFg.bg.alpha
    };
    _draw_rect(rect, bgColor);

    if (fh > 0) {
        rows = height / fh;
    }
    if (fw > 0) {
        cols = width / fw;
    }

    rencache_begin_frame();
}

void render_t::input()
{
    // app_t::log("event!");
    SDL_WaitEventTimeout(NULL, 50);
    poll_event();
}

static void renderView(view_t* view)
{
    if (!view->views.size()) {
        int m = 1;
        RenRect rect = {
            .x = view->x + m,
            .y = view->y + m,
            .width = view->width - (m * 2),
            .height = view->height - (m * 2),
        };

        RenColor color = {
            .b = 255,
            .g = 0,
            .r = 255,
            .a = 50
        };

        _draw_rect(rect, color);
    }

    for (auto v : view->views) {
        renderView(v);
    }
}

void render_t::render()
{
    // renderView(app_t::instance());
    rencache_end_frame();
}

static void addColorPair(int idx, int fg, int bg)
{
    app_t* app = app_t::instance();

    BgFg pair = {
        .bg = color_info_t::true_color(bg),
        .fg = color_info_t::true_color(fg)
    };

    colorPairs[idx] = pair;
}

void render_t::updateColors()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    //---------------
    // build the color pairs
    //---------------
    addColorPair(color_pair_e::NORMAL, app->fg, app->bgApp);
    addColorPair(color_pair_e::SELECTED, app->selFg, app->selBg);

    colorMap[color_pair_e::NORMAL] = color_pair_e::NORMAL;
    colorMap[color_pair_e::SELECTED] = color_pair_e::SELECTED;

    int idx = 32;

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first] = idx;
        addColorPair(idx++, it->first, app->bgApp);
        it++;
    }

    it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first + SELECTED_OFFSET] = idx;
        addColorPair(idx++, it->first, app->selBg);
        if (it->first == app->selBg) {
            colorMap[it->first + SELECTED_OFFSET] = idx + 1;
        }
        it++;
    }
}
