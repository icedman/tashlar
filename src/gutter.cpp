#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "gutter.h"

gutter_t::gutter_t()
    : view_t("gutter")
{
}

gutter_t::~gutter_t()
{
}

void gutter_t::calculate()
{
    if (!isVisible())
        return;
    block_ptr block = editor->document.lastBlock();
    std::string lineNo = std::to_string(1 + block->lineNumber);
    preferredWidth = (lineNo.length() + 1);
}

void gutter_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}