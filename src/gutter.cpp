#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "gutter.h"
#include "render.h"

gutter_t::gutter_t()
    : view_t("gutter")
{
    backgroundColor = 1;
}

gutter_t::~gutter_t()
{
}

void gutter_t::preLayout()
{
    if (!isVisible())
        return;

    block_ptr block = editor->document.lastBlock();
    std::string lineNo = std::to_string(1 + block->lineNumber);
    preferredWidth = (lineNo.length() + 2) * render_t::instance()->fw;
    if (render_t::instance()->fw > 10) {
        preferredWidth += padding * 2;
    }
}

void gutter_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool gutter_t::isVisible()
{
    return visible && app_t::instance()->showGutter;
}
