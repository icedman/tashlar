#include "gutter_view.h"
#include "render.h"
#include "app.h"
#include "util.h"
#include "block.h"

gutter_view_t::gutter_view_t(editor_ptr editor)
	: view_t("gutter")
    , editor(editor)
{
    backgroundColor = 1;
    canFocus = false;
}

gutter_view_t::~gutter_view_t()
{}

void gutter_view_t::update(int delta)
{}

bool gutter_view_t::isVisible()
{
    return visible && app_t::instance()->showGutter;
}

void gutter_view_t::preLayout()
{
    if (!isVisible()) {
        return;
    }

    block_ptr block = editor->document.lastBlock();
    std::string lineNo = std::to_string(1 + block->lineNumber);
    preferredWidth = (lineNo.length() + 3) * getRenderer()->fw;
    if (!getRenderer()->isTerminal()) {
        preferredWidth += padding * 2;
    }
}

void gutter_view_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(0, 0);
    // _addch('+');

    int l = 0;
    block_list::iterator it = editor->document.blocks.begin();
    block_ptr currentBlock = editor->document.cursor().block();
    if (editor->view->scrollY > 0) {
        it += editor->view->scrollY;
    }

    while (it != editor->document.blocks.end()) {
        auto& b = *it++;
        if (l >= rows + 1)
            break;

        std::string lineNo = std::to_string(1 + b->lineNumber);
        // std::string lineNo = std::to_string(1 + b->screenLine);

        if (b->data && b->data->folded && !b->data->foldable) {
            // continue;
        }

        if (b->data && b->data->foldable) {
            // if (b->data->folded) {
            //     lineNo += "+";
            // } else {
            //     lineNo += "^";
            // }
        } else {
            // lineNo += " ";
        }
        lineNo += " ";

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

        if (b->data && b->data->foldable) {
            _move(l, cols - 1);
            _bold(true);
            _attron(_color_pair(colorIndicator));
        #ifdef ENABLE_UTF8
            if (b->data->folded) {
                _addwstr(L"\u2191");
            } else {
                _addwstr(L"\u2192");
            }
        #else
            _addch('>');
            if (b->data->folded) {
                _addch('+');
            } else {
                _addch('^');
            }
        #endif
            _attroff(_color_pair(colorIndicator));
            _bold(false);
        }

        l += b->lineCount;
    }

    while (l < height) {
        _move(l, 0);
        _clrtoeol(cols);
        _move(l++, 0);
    }
}

void gutter_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

