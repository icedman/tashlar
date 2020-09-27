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
    preferredWidth = (lineNo.length() + 3) * render_t::instance()->fw;
    if (!render_t::instance()->isTerminal()) {
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

void gutter_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(0, 0);

    int l = 0;
    block_list::iterator it = editor->document.blocks.begin();
    block_ptr currentBlock = editor->document.cursor().block();
    if (editor->scrollY > 0) {
        it += editor->scrollY;
    }
    while (it != editor->document.blocks.end()) {
        auto& b = *it++;
        if (l >= rows + 1)
            break;

        std::string lineNo = std::to_string(1 + b->lineNumber);
        // std::string lineNo = std::to_string(1 + b->screenLine);

        if (b->data && b->data->folded && !b->data->foldable) {
            continue;
        }

        if (b->data && b->data->foldable) {
            if (b->data->folded) {
                lineNo += "+";
            } else {
                lineNo += "^";
            }
        } else {
            lineNo += " ";
        }

        int pair = colorPrimary;

        if (b == currentBlock) {
            // _italic(true);
            // _bold(true);
            pair = colorIndicator;
        }
        for (int sl = 0; sl < b->lineCount; sl++) {
            _move(l + sl, 0);
            _clrtoeol(cols);
        }
        _attron(_color_pair(pair));
        _move(l, cols - lineNo.length());
        _addstr(lineNo.c_str());
        _attroff(_color_pair(pair));
        _italic(false);
        _bold(false);

        l += b->lineCount;
    }

    while (l < height) {
        _move(l, 0);
        _clrtoeol(cols);
        _move(l++, 0);
    }
}
