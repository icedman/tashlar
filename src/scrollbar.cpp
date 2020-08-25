#include "scrollbar.h"
#include "app.h"
#include "render.h"

scrollbar_t::scrollbar_t()
    : view_t("item")
    , scrollTo(0)
{
    preferredWidth = 6;
    padding = 0;
}

scrollbar_t::~scrollbar_t()
{
}

void scrollbar_t::render()
{
    if (!isVisible() || maxScrollY <= 0) {
        return;
    }

    _move(0, 0);
    _attron(_color_pair(colorIndicator));

    int sy = scrollY;
    if (sy < 0)
        sy = 0;
    int fh = render_t::instance()->fh;

    int s = sy / fh;
    int m = maxScrollY / fh;
    int h = (rows + 4) / m;

    for (int i = 0; i < h; i++) {
        _move((s * h) + i, 0);
        _addch(' ');
    }

    _attroff(_color_pair(colorIndicator));
}

void scrollbar_t::update(int delta)
{
}

void scrollbar_t::preLayout()
{
    preferredHeight = 1 * render_t::instance()->fh;
    if (render_t::instance()->fh > 10) {
        preferredHeight += padding * 2;
    }
}

void scrollbar_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    style_t style = theme->styles_for_scope("function");
    colorPrimary = pairForColor(style.foreground.index, true);
    colorIndicator = pairForColor(app->tabActiveBorder, true);
}

void scrollbar_t::mouseDown(int x, int y, int button, int clicks)
{
    app_t::log("down >>%d", y);
}

void scrollbar_t::mouseDrag(int x, int y, bool within)
{
    app_t::log("drag >>%d", y);
}
