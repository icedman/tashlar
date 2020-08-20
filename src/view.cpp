#include "view.h"
#include "app.h"
#include "render.h"

static view_t* focused = 0;

view_t::view_t(std::string name)
    : name(name)
    , visible(true)
    , canFocus(false)
    , floating(true)
    , scrollX(0)
    , scrollY(0)
    , maxScrollX(0)
    , maxScrollY(0)
    , x(0)
    , y(0)
    , width(0)
    , height(0)
    , preferredWidth(0)
    , preferredHeight(0)
    , flex(1)
    , viewLayout(LAYOUT_HORIZONTAL)
{
}

view_t::~view_t()
{
}

void view_t::setFocus(view_t* w)
{
    focused = w;
}

view_t* view_t::currentFocus()
{
    return focused;
}

bool view_t::isFocused()
{
    return this == focused;
}

void view_t::setVisible(bool v)
{
    visible = v;
}

bool view_t::isVisible()
{
    return visible;
}

void view_t::update(int delta)
{
    for (auto view : views) {
        view->update(delta);
    }
}

void view_t::layout(int x, int y, int width, int height)
{
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;

    rows = height / render_t::instance()->fh;
    cols = width / render_t::instance()->fw;

    switch (viewLayout) {
    case LAYOUT_HORIZONTAL:
        hlayout(x, y, width, height);
        break;
    case LAYOUT_VERTICAL:
        vlayout(x, y, width, height);
        break;
    default:
        break;
    }
}

void view_t::hlayout(int x, int y, int width, int height)
{
    // first pass
    int viewsWithReserve = 0;
    int totalFlex = 0;
    for (auto view : views) {
        if (!view->isVisible()) {
            viewsWithReserve++;
            continue;
        }
        totalFlex += view->flex;
        if (view->preferredWidth > 0) {
            viewsWithReserve++;
        }
    }

    // second pass
    int totalWidth = 0;
    for (auto view : views) {
        if (!view->isVisible())
            continue;
        int pw = view->preferredWidth;
        int w = (width * view->flex) / totalFlex;
        if (w > pw && pw > 0) {
            w = pw;
        }
        view->layout(0, y, w, height);
        totalWidth += w;
    }

    int freeWidth = width - totalWidth;

    // final pass
    int xx = 0;
    for (auto view : views) {
        if (!view->isVisible())
            continue;
        int w = view->width;
        if (freeWidth && view->preferredWidth == 0) {
            int ww = freeWidth / (views.size() - viewsWithReserve);
            w += ww;
            if (w < 0) {
                w = 1;
            }
        }
        view->layout(x + xx, y, w, height);
        xx += w;
    }
}

void view_t::vlayout(int x, int y, int width, int height)
{
    // first pass
    int viewsWithReserve = 0;
    int totalFlex = 0;
    for (auto view : views) {
        if (!view->isVisible()) {
            viewsWithReserve++;
            continue;
        }
        totalFlex += view->flex;
        if (view->preferredHeight > 0) {
            viewsWithReserve++;
        }
    }

    // second pass
    int totalHeight = 0;
    for (auto view : views) {
        if (!view->isVisible())
            continue;
        int ph = view->preferredHeight;
        int h = (height * view->flex) / totalFlex;
        if (h > ph && ph > 0) {
            h = ph;
        }
        view->layout(x, 0, width, h);
        totalHeight += h;
    }

    int freeHeight = height - totalHeight;

    // final pass
    int yy = 0;
    for (auto view : views) {
        if (!view->isVisible())
            continue;
        int h = view->height;
        if (freeHeight && view->preferredHeight == 0) {
            int hh = freeHeight / (views.size() - viewsWithReserve);
            h += hh;
            if (h < 0) {
                h = 1;
            }
        }
        view->layout(x, y + yy, width, h);
        yy += h;
    }
}

void view_t::render()
{
    if (!isVisible())
        return;

    for (auto view : views) {
        _begin(view);
        view->render();
        _end();
    }
}

void view_t::preLayout()
{
    for (auto view : views) {
        view->preLayout();
    }
}

void view_t::preRender()
{
    for (auto view : views) {
        view->preRender();
    }
}

void view_t::applyTheme()
{
    for (auto view : views) {
        view->applyTheme();
    }
}

bool view_t::input(char ch, std::string keys)
{
    bool handled = false;
    for (auto view : views) {
        handled = view->input(ch, keys);
        if (handled)
            break;
    }
    return handled;
}

void view_t::addView(view_t* view)
{
    views.push_back(view);
}

static void visibleViews(view_list& views, view_list& visibles)
{
    for (auto v : views) {
        if (!v->isVisible())
            continue;
        visibles.push_back(v);
        visibleViews(v->views, visibles);
    }
}

view_t* view_t::getViewAt(int x, int y)
{
    view_t* res = NULL;
    app_t* app = app_t::instance();
    view_list visibles;
    visibleViews(app->views, visibles);
    for (auto v : visibles) {
        if (x >= v->x && x < v->x + v->width && y >= v->y && y < v->y + v->height) {
            res = v;
        }
    }
    return res;
}

static view_t* findShiftFocus(view_t* view, int x, int y)
{
    if (x > 0) {
        x = view->width;
    }
    if (y > 0) {
        y = view->height;
    }
    view_t* next = view_t::getViewAt(view->x + x, view->y + y);
    if (next) {
        if (next->canFocus) {
            app_t::log("shift %s %d", next->name.c_str(), next->canFocus);
            return next;
        }
        return findShiftFocus(next, x, y);
    }
    return NULL;
}

view_t* view_t::shiftFocus(int x, int y)
{
    view_t* view = currentFocus();
    app_t::log("current: %s", view->name.c_str());
    view_t* next = findShiftFocus(view, x, y);
    if (next) {
        app_t::log("next: %s", next->name.c_str());
        setFocus(next);
    }
    return NULL;
}

void view_t::scroll(int s)
{
    scrollY -= s;
    if (scrollY < 0)
        scrollY = 0;
    if (scrollY >= maxScrollY)
        scrollY = maxScrollY - 1;
}
