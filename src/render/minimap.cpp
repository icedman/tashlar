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

    _move(0, 0);

    int y = 0;
    for (int idx = editor->scrollY; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];

        int pair = colorPrimary;
        _move(y, 0);
        _clrtoeol(cols);
        _move(y++, 0);

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

            _attron(_color_pair(pair));
#ifdef ENABLE_UTF8
            if (!_drawdots(b->data->dots[x])) {
                _addwstr(wcharFromDots(b->data->dots[x]));
            }
#endif
            _attroff(_color_pair(pair));

            if (x >= cols - 2) {
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
        _move(y++, 0);
        _clrtoeol(cols);
    }
}
