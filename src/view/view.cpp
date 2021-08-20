#include "view.h"
#include "render.h"
#include "util.h"

static view_t* focused = 0;
static view_t* hovered = 0;
static view_t* dragged = 0;

static view_t dummyRoot;
static view_t* root = &dummyRoot;
static view_t* mainContainer = &dummyRoot;

void view_t::setRoot(view_t *r)
{
    root = r;
}

view_t* view_t::getRoot()
{
    return root;
}

void view_t::setMainContainer(view_t *m)
{
    mainContainer = m;
}

view_t* view_t::getMainContainer()
{
    return mainContainer;
}

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
    , padding(2)
    , viewLayout(LAYOUT_HORIZONTAL)
    , backgroundColor(0)
{
}

view_t::~view_t()
{
}

render_t* view_t::getRenderer()
{
    // should have been initiaed somewhere
    return render_t::instance();
}

void view_t::setFocus(view_t* w)
{
    if (focused != w) {
        if (w) {
            w->onFocusChanged(true);
        }
        if (focused) {
            focused->onFocusChanged(false);
        }
    }
    focused = w;
}

view_t* view_t::currentFocus()
{
    return focused;
}

void view_t::onFocusChanged(bool focused)
{
}

void view_t::setHovered(view_t* view)
{
    hovered = view;
}

view_t* view_t::currentHovered()
{
    if (!hovered) {
        return focused;
    }
    return hovered;
}

void view_t::setDragged(view_t* w)
{
    dragged = w;
}

view_t* view_t::currentDragged()
{
    return dragged;
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

void view_t::mouseDown(int x, int y, int button, int clicks)
{
}

void view_t::mouseUp(int x, int y, int button)
{
}

void view_t::mouseDrag(int x, int y, bool within)
{
}

void view_t::mouseHover(int x, int y)
{
}

void view_t::layout(int x, int y, int width, int height)
{
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;

    rows = height;
    if (!getRenderer()->isTerminal()) {
        rows -= padding * 2;
    }
    rows /= getRenderer()->fh;
    cols = width;
    if (!getRenderer()->isTerminal()) {
        cols -= padding * 2;
    }
    cols /= getRenderer()->fw;

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

void view_t::removeView(view_t* view)
{
    view_list::iterator it = views.begin();
    while(it != views.end()) {
        if (*it == view) {
            views.erase(it);
            return;
        }
        it++;
    }
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
    view_list rootList;
    rootList.push_back(view_t::getRoot());
    view_list visibles;
    visibleViews(rootList, visibles);
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
            log("shift %s %d", next->name.c_str(), next->canFocus);
            return next;
        }
        return findShiftFocus(next, x, y);
    }
    return NULL;
}

view_t* view_t::shiftFocus(int x, int y)
{
    view_t* view = currentFocus();
    log("current: %s", view->name.c_str());
    view_t* next = findShiftFocus(view, x, y);
    if (next) {
        log("next: %s", next->name.c_str());
        setFocus(next);
    }
    return NULL;
}

void view_t::scroll(int s)
{
    scrollY -= s;
    if (scrollY >= maxScrollY && maxScrollY > 0)
        scrollY = maxScrollY - 1;
    if (scrollY < 0)
        scrollY = 0;
}

view_t* view_t::viewFromPointer(int x, int y)
{
    if (!isVisible())
        return NULL;
    view_t* view = this;
    view_t* res = NULL;

    if (x >= view->x && x < view->x + view->width && y >= view->y && y < view->y + view->height) {
        res = view;
        for (auto v : views) {
            view_t* cres = v->viewFromPointer(x, y);
            if (cres) {
                res = cres;
            }
        }
    }
    return res;
}
