#include "app.h"
#include "dots.h"
#include "render.h"
#include "util.h"

static view_list contextStack;

// #define _draw_rect rencache_draw_rect
// #define _draw_text rencache_draw_text
// #define _draw_wtext ren_draw_wtext

static float scrollVelocity = 0;

static int drawBaseX = 0;
static int drawBaseY = 0;
static int drawX = 0;
static int drawY = 0;
static int drawColorPair = 0;
// static RenColor drawColor;
// static RenColor drawBg;
static bool drawBold = false;
static bool drawReverse = false;
static bool drawItalic = false;
static bool drawUnderline = false;

static int dragX = 0;
static int dragY = 0;

static int keyMods = 0;

static std::map<int, int> colorMap;

// RenColor bgColor;
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
    // int cw = ren_get_font_width(font, (char*)txt);

    // RenColor bg = drawBg;
    // RenColor fg = drawColor;
    if (drawReverse) {
        // fg = drawBg;
        // bg = drawColor;
    }

    // background
    // RenRect rect = { .x = drawBaseX + drawX * fw, .y = drawBaseY + drawY * fh, .width = fw, .height = fh };
    // if (bg.r != bgColor.r || bg.g != bgColor.g || bg.b != bgColor.b) {
        // _draw_rect(rect, bg);
        // log("bg: %d %d %d (%d %d %d)", bg.r, bg.g, bg.b, bgColor.r, bgColor.g, bgColor.b);
    // }

    // _draw_text(font, (char*)txt, drawBaseX + (drawX * fw) + (fw / 2 - cw / 2), drawBaseY + (drawY * fh), fg, drawBold, drawItalic);

    if (drawUnderline) {
        // rect.y += (rect.height - 2);
        // rect.height = 1;
        // _draw_rect(rect, fg);
    }

    drawX++;
}

bool _drawdots(int dots, int* colors)
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

    float ffw = fw / 2;
    float ffh = fh / 4;

    for (int i = 0; i < 8; i++) {
        _color_pair(colors[i]);

        // RenColor fg = drawColor;
        // fg.a = 100;

        if (dots & dm[i]) {
            int ofx = offs[i * 2 + 1] * ffw;
            int ofy = offs[i * 2] * ffh;
            // RenRect rect = {
            //     .x = ofx + drawBaseX + drawX * fw,
            //     .y = ofy + drawBaseY + drawY * fh,
            //     .width = dw,
            //     .height = dw
            // };
            // _draw_rect(rect, fg);
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
    int fw = render_t::instance()->fw;
    int fh = render_t::instance()->fh;
    // int cw = ren_get_font_width(font, "Z");
    // RenColor fg = drawColor;

    int dw = 4;
    int ofx = fw / 2 - dw / 2;
    int ofy = fh / 2 - dw / 2;
    // RenRect rect = {
    //     .x = ofx + drawBaseX + drawX * fw,
    //     .y = ofy + drawBaseY + drawY * fh,
    //     .width = dw,
    //     .height = dw
    // };
    // _draw_rect(rect, fg);

    /*
    _draw_wtext(font, str, drawBaseX + (drawX * fw) + (fw / 2 - cw / 2), drawBaseY + (drawY * fh), fg, drawBold, drawItalic);
    */
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

    /*
    RenColor color = {
        .b = (uint8_t)bgFg.fg.blue,
        .g = (uint8_t)bgFg.fg.green,
        .r = (uint8_t)bgFg.fg.red,
        .a = (uint8_t)bgFg.fg.alpha
    };
    drawColor = color;

    RenColor bg = {
        .b = (uint8_t)bgFg.bg.blue,
        .g = (uint8_t)bgFg.bg.green,
        .r = (uint8_t)bgFg.bg.red,
        .a = (uint8_t)bgFg.bg.alpha
    };
    drawBg = bg;
	*/

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

void _italic(bool b)
{
    drawItalic = b;
}

void _begin(view_t* view)
{
    contextStack.push_back(view);

    /*
    RenRect rect = {
        .x = view->x,
        .y = view->y,
        .width = view->width,
        .height = view->height,
    };
    */

    drawBaseX = view->x + view->padding;
    drawBaseY = view->y + view->padding;

    if (view->isVisible() && view->backgroundColor != 0) {
        int off = 5 * view->backgroundColor;
        // int bb = bgColor.b - off;
        // int gg = bgColor.g - off;
        // int rr = bgColor.r - off;
        // if (bb < 0)
        //     bb = 0;
        // if (gg < 0)
        //     gg = 0;
        // if (rr < 0)
        //     rr = 0;
        // if (bb > 255)
        //     bb = 255;
        // if (gg > 255)
        //     gg = 255;
        // if (rr > 255)
        //     rr = 255;
        /*
        RenColor bg = {
            .b = (uint8_t)bb,
            .g = (uint8_t)gg,
            .r = (uint8_t)rr,
            .a = 255
        };
        rencache_draw_rect(rect, bg);
        */
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
    fw = 1;
    fh = 1;
}

void render_t::shutdown()
{
}

void render_t::update(int delta)
{
    BgFg bgFg = colorPairs[color_pair_e::NORMAL];

    /*
    bgColor = {
        .b = (uint8_t)bgFg.bg.blue,
        .g = (uint8_t)bgFg.bg.green,
        .r = (uint8_t)bgFg.bg.red,
        .a = (uint8_t)bgFg.bg.alpha
    };
    */

    // RenRect rect = { .x = 0, .y = 0, .width = width, .height = height };
    // _draw_rect(rect, bgColor);

    if (fh > 0) {
        rows = height / fh;
    }
    if (fw > 0) {
        cols = width / fw;
    }
}

void render_t::input()
{
}

static void renderView(view_t* view)
{
    if (!view->views.size()) {
        int m = 1;

        // RenRect rect = {
        //     .x = view->x + m,
        //     .y = view->y + m,
        //     .width = view->width - (m * 2),
        //     .height = view->height - (m * 2),
        // };

        // RenColor color = {
        //     .b = 255,
        //     .g = 0,
        //     .r = 255,
        //     .a = 50
        // };

        // _draw_rect(rect, color);
    }

    for (auto v : view->views) {
        renderView(v);
    }
}

void render_t::render()
{
    // renderView(app_t::instance());
    // rencache_end_frame();
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

void render_t::delay(int ms)
{
}

bool render_t::isTerminal()
{
    return false;
}

int _keyMods()
{
    return keyMods;
}
