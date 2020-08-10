#include "editor.h"
#include "app.h"

#include <curses.h>

struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos);

void _clrtoeol(int w)
{
    while (w-- > 0)
        addch(' ');
}

void editor_t::render()
{
    if (!isVisible())
        return;

    editor_t* editor = this;
    int cols = editor->width;
    int rows = editor->height;

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
        if (c++ > height + preceedingBlocks)
            break;
    }

    // if (hlTarget == nullptr && it != document.blocks.end()) {
    // hlTarget = *it;
    // highlighter.run(this);
    // }

    //---------------

    it = document.blocks.begin();
    if (editor->scrollY > 0) {
        it += editor->scrollY;
    }
    while (it != editor->document.blocks.end()) {
        auto& b = *it++;

        if (l >= rows)
            break;

        int colorPair = color_pair_e::NORMAL;
        int colorPairSelected = color_pair_e::SELECTED;

        size_t lineNo = b->lineNumber;

        // editor->highlighter.highlightBlock(b);
        struct blockdata_t* blockData = b->data.get();

        std::string text = b->text() + " ";
        
        // todo.. find a proper place for this
        editor->completer.addLine(text);

        char* line = (char*)text.c_str();
        for (int sl = 0; sl < b->lineCount; sl++) {
            move(y + l, x);
            _clrtoeol(width);
            move(y + l++, x);

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
                        attron(A_UNDERLINE);
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
                    attron(A_BOLD);
                    attron(A_UNDERLINE);
                }

                attron(COLOR_PAIR(colorPair));
                addch(ch);
                attroff(COLOR_PAIR(colorPair));
                attroff(A_BOLD);
                attroff(A_UNDERLINE);
            }
        }
    }

    while (l < rows) {
        move(y + l, x);
        _clrtoeol(width);
        move(y + l++, x);
        // addch('~');
    }
}
