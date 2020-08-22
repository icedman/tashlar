#include "editor.h"
#include "app.h"
#include "render.h"

struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos);

void editor_t::render()
{
    if (!isVisible())
        return;

    editor_t* editor = this;

    struct cursor_t mainCursor = editor->document.cursor();
    cursor_list cursors = editor->document.cursors;

    editor->matchBracketsUnderCursor();

    // app_t::log("scroll %d %d", editor->scrollX, editor->scrollY);

    size_t cx;
    size_t cy;

    int l = 0;
    block_list::iterator it = editor->document.blocks.begin();

    //---------------
    // highlight
    //---------------
    int preceedingBlocks = 16;
    int idx = scrollY - preceedingBlocks;
    if (idx < 0) {
        idx = 0;
    }
    it += idx;

    int c = 0;
    while (it != document.blocks.end()) {
        block_ptr b = *it;
        highlighter.highlightBlock(b);
        it++;
        if (c++ > rows + preceedingBlocks)
            break;
    }

    //---------------
    it = document.blocks.begin();
    if (editor->scrollY > 0) {
        it += editor->scrollY;
    }

    bool firstLine = true;
    while (it != editor->document.blocks.end()) {
        auto& b = *it++;

        if (l >= rows + 1)
            break;

        int colorPair = color_pair_e::NORMAL;
        int colorPairSelected = color_pair_e::SELECTED;

        size_t lineNo = b->lineNumber;
        struct blockdata_t* blockData = b->data.get();

        std::string text = b->text() + " ";

        char* line = (char*)text.c_str();
        for (int sl = 0; sl < b->lineCount; sl++) {
            _move(l, 0);
            _clrtoeol(cols);
            _move(l++, 0);

            if (text.length() < editor->scrollX) {
                continue;
            }

            if (blockData->folded && !blockData->foldable) {
                l--;
                continue;
            }

            // app_t::instance()->log("%s", line);
            int col = 0;
            for (int i = editor->scrollX;; i++) {
                size_t pos = i + (sl * cols);
                if (col++ >= cols) {
                    break;
                }

                char ch = line[pos];
                if (ch == 0)
                    break;

                bool hl = false;
                bool ul = false;

                colorPair = color_pair_e::NORMAL;
                colorPairSelected = color_pair_e::SELECTED;

                // syntax here
                if (blockData) {
                    struct span_info_t span = spanAtBlock(blockData, pos);
                    if (span.length) {
                        colorPair = pairForColor(span.colorIndex, false);
                        colorPairSelected = pairForColor(span.colorIndex, true);
                    }
                }

                // bracket
                if (cursorBracket1.bracket != -1 && cursorBracket2.bracket != -1) {
                    if ((pos == cursorBracket1.position && lineNo == cursorBracket1.line) || (pos == cursorBracket2.position && lineNo == cursorBracket2.line)) {
                        _underline(true);
                    }
                }

                // cursors
                for (auto& c : cursors) {
                    if (pos == c.position() && b == c.block()) {
                        hl = true;
                        ul = c.hasSelection();
                    }
                    if (!c.hasSelection())
                        continue;

                    cursor_position_t start = c.selectionStart();
                    cursor_position_t end = c.selectionEnd();
                    if (b->lineNumber < start.block->lineNumber || b->lineNumber > end.block->lineNumber)
                        continue;
                    if (b == start.block && pos < start.position)
                        continue;
                    if (b == end.block && pos > end.position)
                        continue;
                    hl = true;
                    break;
                }

                if (hl) {
                    colorPair = colorPairSelected;
                }
                if (ul) {
                    _bold(true);
                    _underline(true);
                }

                _attron(_color_pair(colorPair));
                _addch(ch);
                _attroff(_color_pair(colorPair));
                _bold(false);
                _underline(false);
            }
        }
    }

    while (l < rows) {
        _move(l, 0);
        _clrtoeol(cols);
        _move(l++, 0);
        // addch('~');
    }
}
