#include "statusbar.h"
#include "app.h"
#include "editor.h"
#include "render.h"

static struct statusbar_t* statusbarInstance = 0;

statusbar_t::statusbar_t()
    : view_t("statusbar")
    , frames(0)
{
    preferredHeight = 1;
    statusbarInstance = this;
    backgroundColor = 2;
}

struct statusbar_t* statusbar_t::instance()
{
    return statusbarInstance;
}

void statusbar_t::setText(std::string s, int pos, int size)
{
    sizes[pos] = size;
    text[pos] = s;
}

void statusbar_t::setStatus(std::string s, int f)
{
    status = s;
    frames = (f + 500) * 1000;
}

void statusbar_t::update(int delta)
{
    if (!app_t::instance()->showStatusBar) {
        return;
    }

    if (frames < 0) {
        return;
    }

    frames = frames - delta;
    if (frames < 500) {
        status = "";
    }
}

void statusbar_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool statusbar_t::isVisible()
{
    return visible && app_t::instance()->showStatusBar;
}

void statusbar_t::preLayout()
{
    preferredHeight = render_t::instance()->fh;
    if (render_t::instance()->fh > 10) {
        preferredHeight += (padding * 2);
    }
}
