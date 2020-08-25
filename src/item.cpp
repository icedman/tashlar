#include "item.h"
#include "app.h"
#include "render.h"

item_view_t::item_view_t()
    : view_t("item")
{
    preferredHeight = 1;
    padding = 2;
}

item_view_t::~item_view_t()
{
}

void item_view_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t::log("%s %d %d %d %d", text.c_str(), x, y, width, preferredHeight);

    if (!text.length())
        return;

    _move(0, 0);
    _attron(_color_pair(colorPrimary));
    _addstr(text.c_str());
    _attroff(_color_pair(colorPrimary));
}

void item_view_t::update(int delta)
{
}

void item_view_t::preLayout()
{
    preferredHeight = 1 * render_t::instance()->fh;
    if (render_t::instance()->fh > 10) {
        preferredHeight += padding * 2;
    }
}
