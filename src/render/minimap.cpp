#include "minimap.h"
#include "app.h"
#include "dots.h"
#include "render.h"

#define MINIMAP_TEXT_COMPRESS 5
#define MINIMAP_TEXT_BUFFER 25

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

    // int fh = render_t::instance()->fh;
    // int sy = editor->scrollY / fh;

    int sy = offsetY;
    int y = 0;
    for (int idx = sy; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];

        if (y == 0) {
            firstVisibleLine = b->lineNumber;
        }
        lastVisibleLine = b->lineNumber;

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

        buildUpDotsForBlock(b, MINIMAP_TEXT_COMPRESS, MINIMAP_TEXT_BUFFER);

        int textCompress = MINIMAP_TEXT_COMPRESS;
        block_list& snapBlocks = editor->snapshots[0].snapshot;

        for (int x = 0; x < MINIMAP_TEXT_BUFFER; x++) {
            int x1 = x + 1;
            int x2 = x1 + 1;
            _attron(_color_pair(pair));

            int colors[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

            if (render_t::instance()->fh > 10 && snapBlocks.size() && b->lineNumber > 0) {

                blockdata_t* blockData = b->data.get();
                if (!blockData || !blockData->spans.size()) {
                    if (b->lineNumber <= snapBlocks.size()) {
                        blockData = snapBlocks[b->lineNumber - 1]->data.get();
                    }
                }
                if (blockData) {
                    span_info_t si = spanAtBlock(blockData, x * textCompress);
                    colors[0] = pairForColor(si.colorIndex, false);
                    si = spanAtBlock(blockData, (x1 + 1) * textCompress);
                    colors[1] = pairForColor(si.colorIndex, false);

                    colors[2] = colors[0];
                    colors[3] = colors[1];
                    colors[4] = colors[0];
                    colors[5] = colors[1];
                    colors[6] = colors[0];
                    colors[7] = colors[1];

                    block_ptr bNext = b->next();
                    if (bNext) {
                        blockData = bNext->data.get();
                        if (!blockData || !blockData->spans.size()) {
                            if (bNext->lineNumber <= snapBlocks.size()) {
                                blockData = snapBlocks[bNext->lineNumber - 1]->data.get();
                            }
                        }
                        if (blockData) {
                            si = spanAtBlock(blockData, x1 * textCompress);
                            colors[2] = pairForColor(si.colorIndex, false);
                            si = spanAtBlock(blockData, (x2)*textCompress);
                            colors[3] = pairForColor(si.colorIndex, false);
                        }

                        bNext = bNext->next();
                        if (bNext) {
                            blockData = bNext->data.get();
                            if (!blockData || !blockData->spans.size()) {
                                if (bNext->lineNumber <= snapBlocks.size()) {
                                    blockData = snapBlocks[bNext->lineNumber - 1]->data.get();
                                }
                            }
                            if (blockData) {
                                si = spanAtBlock(blockData, x1 * textCompress);
                                colors[4] = pairForColor(si.colorIndex, false);
                                si = spanAtBlock(blockData, (x2)*textCompress);
                                colors[5] = pairForColor(si.colorIndex, false);
                            }

                            bNext = bNext->next();
                            if (bNext) {
                                blockData = bNext->data.get();
                                if (!blockData || !blockData->spans.size()) {
                                    if (bNext->lineNumber <= snapBlocks.size()) {
                                        blockData = snapBlocks[bNext->lineNumber - 1]->data.get();
                                    }
                                }
                                if (blockData) {
                                    si = spanAtBlock(blockData, x1 * textCompress);
                                    colors[6] = pairForColor(si.colorIndex, false);
                                    si = spanAtBlock(blockData, (x2)*textCompress);
                                    colors[7] = pairForColor(si.colorIndex, false);
                                }
                            }
                        }
                    }
                }
            }

            if (!_drawdots(b->data->dots[x], colors)) {
#ifdef ENABLE_UTF8
                _addwstr(wcharFromDots(b->data->dots[x]));
#endif
            }

            _attroff(_color_pair(pair));

            if (x >= cols - 2) {
                break;
            }
        }

        _attroff(_color_pair(pair));
        _bold(false);

        if (y > rows) {
            break;
        }
    }
    while (y < rows) {
        _move(y++, 0);
        _clrtoeol(cols);
    }
}
