#include "minimap.h"
#include "app.h"
#include "dots.h"
#include "render.h"

#define MINIMAP_TEXT_COMPRESS 5

void minimap_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    int _y = this->y;
    int _x = this->x;
    _move(_y, _x);

    int y = 0;
    for (int idx = editor->scrollY; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];

        int pair = colorPrimary;
        _move(_y + y, _x);
        _clrtoeol(width);
        _move(_y + y++, _x);

        if (currentLine >= b->lineNumber && currentLine < b->lineNumber + 4) {
            _bold(true);
            _attron(_color_pair(colorIndicator));

#ifdef ENABLE_UTF8
            _addwstr(L"\u2192");
#else
            _addch('>');
#endif
            _attroff(_color_pair(colorIndicator));
        } else {
            _addch(' ');
        }

        buildUpDotsForBlock(b, MINIMAP_TEXT_COMPRESS, 25);
        for (int x = 0; x < 25; x++) {
            // int cx = 1 + (x * 4);
            // for (auto span : b.data->spans) {
            // if (cx >= span.start && cx < span.start + span.length) {
            // pair = pairForColor(span.colorIndex, false);
            // break;
            // }
            // }

            _attron(_color_pair(pair));
#ifdef ENABLE_UTF8
            _addwstr(wcharFromDots(b->data->dots[x]));
#endif
            _attroff(_color_pair(pair));

            if (x >= width - 2) {
                break;
            }
        }

        _attroff(_color_pair(pair));
        _bold(false);

        if (y >= height) {
            break;
        }
    }
    while (y < height) {
        _move(_y + y++, _x);
        _clrtoeol(width);
    }
}
