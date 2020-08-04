#include "gutter.h"
#include "app.h"

#include <curses.h>

void _clrtoeol(int w);

void gutter_t::render()
{
    if (!app_t::instance()->showGutter || !isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    int cols = editor->width;
    int rows = editor->height;

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    move(y, x);

    int l = 0;
    block_list::iterator it = editor->document.blocks.begin();
    block_ptr currentBlock = editor->document.cursor().block();
    if (editor->scrollY > 0) {
        it += editor->scrollY;
    }
    while (it != editor->document.blocks.end()) {
        auto& b = *it++;
        if (l >= rows)
            break;

        std::string lineNo = std::to_string(1 + b->lineNumber);

        int pair = colorPrimary;
        if (b == currentBlock) {
            attron(A_BOLD);
        }
        move(y + l, x);
        _clrtoeol(width);
        attron(COLOR_PAIR(pair));
        move(y + l++, x + width - lineNo.length() - 1);
        addstr(lineNo.c_str());
        attroff(COLOR_PAIR(pair));
        attroff(A_BOLD);
    }

    while (l < height) {
        move(y + l, x);
        _clrtoeol(width);
        move(y + l++, x);
    }
}
