#include "scrollbar_view.h"
#include "app.h"
#include "render.h"

scrollbar_view_t::scrollbar_view_t()
    : view_t("scrollbar")
    , scrollTo(-1)
{
    preferredWidth = 6;
    padding = 0;
    thumbSize = 2;
}

scrollbar_view_t::~scrollbar_view_t()
{
}

void scrollbar_view_t::render()
{
    if (!isVisible() || maxScrollY <= 0) {
        return;
    }

    int pair = colorPrimary;
    if (this == view_t::currentDragged() || (this == view_t::currentHovered() && !view_t::currentDragged)) {
        pair = colorIndicator;
    }

    _move(0, 0);
    _attron(_color_pair(pair));
    _reverse(true);

    int fh = getRenderer()->fh;

    int th = thumbSize;
    int sy = scrollY;
    if (sy < 0)
        sy = 0;

    int s = (rows - thumbSize + 1) * sy / maxScrollY;
    for (int i = 0; i < thumbSize; i++) {
        _move(s + i, 0);
        _addch(' ');
    }

    _attroff(_color_pair(pair));
    _reverse(false);
}

void scrollbar_view_t::update(int delta)
{
}

void scrollbar_view_t::preLayout()
{
    preferredHeight = 1 * getRenderer()->fh;
    if (getRenderer()->fh > 10) {
        preferredHeight += padding * 2;
    }
}

void scrollbar_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    style_t style = theme->styles_for_scope("function");
    colorPrimary = pairForColor(style.foreground.index, true);
    colorIndicator = pairForColor(app->tabActiveBorder, true);
}

void scrollbar_view_t::mouseDown(int x, int y, int button, int clicks)
{
    scrollTo = maxScrollY * (y - this->y) / height;
    // log(">y:%d t:%d h:%d %d", y, this->y, height, scrollTo);
}

void scrollbar_view_t::mouseDrag(int x, int y, bool within)
{
    mouseDown(x, y, 1, 0);
}