#include "scrollbar.h"
#include "app.h"
#include "render.h"

scrollbar_t::scrollbar_t()
    : view_t("scrollbar")
    , scrollTo(-1)
{
    preferredWidth = 6;
    padding = 0;
    thumbSize = 2;
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

    int fh = render_t::instance()->fh;

    int th = thumbSize;
    int sy = scrollY;
    if (sy < 0)
        sy = 0;

    int s = (rows - thumbSize + 1)  * sy / maxScrollY;
    for(int i=0;i<thumbSize; i++) {
        _move(s + i, 0);
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
    scrollTo = maxScrollY * (y - this->y) / height;
    // app_t::log(">y:%d t:%d h:%d %d", y, this->y, height, scrollTo);
}

void scrollbar_t::mouseDrag(int x, int y, bool within)
{
    mouseDown(x, y, 1, 0);
}
