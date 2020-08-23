#include "gutter.h"
#include "app.h"
#include "render.h"

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
