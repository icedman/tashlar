#include "app.h"
#include "render.h"
#include "renderer.h"

#include <SDL2/SDL.h>

static view_list contextStack;
static int deferDraw = 0;

SDL_Window* window;
RenFont* font;

static int drawX = 0;
static int drawY = 0;
static int fw = 1;
static int fh = 1;
static int drawColorPair = 0;
static RenColor drawColor;
static RenColor drawBg;

static std::map<int, int> colorMap;

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
    char txt[] = { c, 0 };
    int cw = ren_get_font_width(font, txt);

    // background
    RenRect rect = { .x = drawX * fw, .y = drawY * fh, .width = fw, .height = fh };
    ren_draw_rect(rect, drawBg);
    
    ren_draw_text(font, txt, drawX * fw + (fw/2 - cw/2), drawY * fh, drawColor);
    drawX ++;
}

void _addstr(const char* str)
{
    int l = strlen(str);
    for(int i=0; i<l; i++) {
        _addch(str[i]);
    }
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
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
    
    atexit(SDL_Quit);

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    app_t::log("%d %d", dm.w, dm.h);
    width = dm.w * 0.75;
    height = dm.h * 0.75;

    window = SDL_CreateWindow(
        "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);

    ren_init(window);
    
    font = ren_load_font("./fonts/font.ttf", 16);
    font = ren_load_font("./fonts/monospace.ttf", 16);
    if (font) {
        app_t::log("font loaded");
        fw = ren_get_font_width(font, "AaBbCcDdEeFfGg") / 14;
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
                render_t::instance()->width = e.window.data1;
                render_t::instance()->height = e.window.data2;
                deferDraw = 1;
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
        app_t::log("key down");

        std::string keySequence;
        std::string mod;
        
        switch (e.key.keysym.sym) {
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
            } else
            if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9) {
                keySequence += (char)(e.key.keysym.sym - SDLK_1) + '1';
            } else
            if (e.key.keysym.sym == SDLK_0) {
                keySequence += (char)'0';
            }
            break;
        }

        if (keySequence.length()) {
            if (e.key.keysym.mod & KMOD_CTRL) {
                mod = "ctrl"; 
            }
            if (e.key.keysym.mod & KMOD_ALT) {
                if (mod.length()) mod += "+";
                mod += "alt"; 
            }
            if (e.key.keysym.mod & KMOD_SHIFT) {
                if (mod.length()) mod += "+";
                mod += "shift"; 
            }
            if (mod.length()) {
                keySequence = mod + "+" + keySequence;
            }

            app_t::log("keydown %s", keySequence.c_str());
            pushKey(0, keySequence);
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
        return 0;

    case SDL_MOUSEBUTTONUP:
        return 0;

    case SDL_MOUSEMOTION:
        deferDraw = 0;
        return 0;

    case SDL_MOUSEWHEEL:
        return 0;

    default:
        return 0;
    }

    return 0;
}

void render_t::update(int delta)
{
    RenRect rect = { .x = 0, .y = 0, .width = width, .height = height };
    
    RenColor bg = {
        .b = 0,
        .g = 0,
        .r = 0,
        .a = 150
    };
    ren_draw_rect(rect, bg);

    // app_t::log("event!");
    // SDL_WaitEventTimeout(NULL, 1 * 1000);
    poll_event();
}

static void renderView(view_t* view)
{
    if (!view->isVisible()) return;

    if (!view->views.size()) {

        int margin = view->height > 1 ? 4 : 0;
        RenRect rect = {
            .x = view->x + margin,
            .y = view->y + margin,
            .width = view->width - (margin * 2),
            .height = view->height - (margin * 2),
        };
        RenColor color = {
            .b = 255,
            .g = 0,
            .r = 255,
            .a = 255
        };
        RenColor textColor = {
            .b = 255,
            .g = 255,
            .r = 255,
            .a = 255
        };

        if (deferDraw > 0) {
            // 
        } else {
            // ren_set_clip_rect(rect);
            // ren_draw_rect(rect, color);
            // ren_draw_text(font, view->name.c_str(), view->x, view->y, textColor);
        }
        
        // app_t::log(">%s x:%d y:%d w:%d h:%d", view->name.c_str(), view->x, view->y, view->width, view->height);
    }

    for(auto child : view->views) {
        renderView(child);
    }
}
    
void render_t::render()
{
    // app_t::log("render!");

    RenRect rects[] = {
        { .x = 0, .y = 0, .width = width, .height = height }
    };
    
    ren_update_rects(rects, 1);
    
    // app_t *app = app_t::instance();
    // renderView(app);
}

static void addColorPair(int idx, int fg, int bg)
{
    app_t* app = app_t::instance();
    
    BgFg pair = {
        .bg = color_info_t::term_color(bg),
        .fg = color_info_t::term_color(fg)
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
    addColorPair(color_pair_e::NORMAL, app->fg, app->bg);
    addColorPair(color_pair_e::SELECTED, app->selFg, app->selBg);

    colorMap[color_pair_e::NORMAL] = color_pair_e::NORMAL;
    colorMap[color_pair_e::SELECTED] = color_pair_e::SELECTED;
    
    int idx = 32;

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        colorMap[it->first] = idx;
        addColorPair(idx++, it->first, app->bg);
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
