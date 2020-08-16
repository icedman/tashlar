#include "render.h"
#include "gutter.h"
#include "app.h"

void gutter_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    int cols = editor->width;
    int rows = editor->height;

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(y, x);

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
        // std::string lineNo = std::to_string(1 + b->screenLine);

        if (b->data && b->data->folded && !b->data->foldable) {
            continue;
        }

        if (b->data && b->data->foldable) {
            lineNo += "-";
        } else {
            lineNo += " ";
        }

        int pair = colorPrimary;
        if (b == currentBlock) {
            _bold(true);
        }
        for (int sl = 0; sl < b->lineCount; sl++) {
            _move(y + l + sl, x);
            _clrtoeol(width);
        }
        _attron(_color_pair(pair));
        _move(y + l, x + width - lineNo.length() - 1);
        _addstr(lineNo.c_str());
        _attroff(_color_pair(pair));
        _bold(false);

        l += b->lineCount;
    }

    while (l < height) {
        _move(y + l, x);
        _clrtoeol(width);
        _move(y + l++, x);
    }
}
