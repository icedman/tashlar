#include "minimap.h"
#include "app.h"
#include "dots.h"

#include <curses.h>

#define MINIMAP_TEXT_COMPRESS 5

void _clrtoeol(int w);

void minimap_t::render()
{
    if (!app_t::instance()->showMinimap || !isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    int _y = this->y;
    int _x = this->x;
    move(_y, _x);

    int y = 0;
    for (int idx = editor->scrollY; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];

        int pair = colorPrimary;
        move(_y + y, _x);
        _clrtoeol(width);
        move(_y + y++, _x);

        if (currentLine >= b->lineNumber && currentLine < b->lineNumber + 4) {
            attron(A_BOLD);
            attron(COLOR_PAIR(colorIndicator));

#ifdef ENABLE_UTF8
            addwstr(L"\u2192");
#else
            addch('>');
#endif
            attroff(COLOR_PAIR(colorIndicator));
        } else {
            addch(' ');
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

            attron(COLOR_PAIR(pair));
#ifdef ENABLE_UTF8
            addwstr(wcharFromDots(b->data->dots[x]));
#endif
            attroff(COLOR_PAIR(pair));

            if (x >= width - 2) {
                break;
            }
        }

        attroff(COLOR_PAIR(pair));
        attroff(A_BOLD);

        if (y >= height) {
            break;
        }
    }
    while (y < height) {
        move(_y + y++, _x);
        _clrtoeol(width);
    }
}
