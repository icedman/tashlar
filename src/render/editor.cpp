#include "editor.h"
#include "app.h"

#include <curses.h>

struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos);

void _clrtoeol(int w)
{
    while (w-- > 0)
        addch(' ');
}

void editor_t::layout(int _x, int _y, int w, int h)
{
    if (!document.blocks.size()) {
        return;
    }

    x = _x;
    y = _y;
    width = w;
    height = h;

    maxScrollX = 0;
    maxScrollY = document.lastBlock()->screenLine + (h / 3);
}

void editor_t::calculate()
{
    if (width == 0 || height == 0 || !isVisible())
        return;

    cursor_t mainCursor = document.cursor();

    // update scroll
    block_ptr cursorBlock = mainCursor.block();

    int screenX = mainCursor.position();
    int screenY = cursorBlock->screenLine;

    int lookAheadX = (width / 3);
    int lookAheadY = 0; // (height/5);
    if (screenY - scrollY < lookAheadY) {
        scrollY = screenY - lookAheadY;
        if (scrollY < 0)
            scrollY = 0;
    }
    if (screenY - scrollY + 1 > height) {
        scrollY = -(height - screenY) + 1;
    }
    if (screenX - scrollX < lookAheadX) {
        scrollX = screenX - lookAheadX;
        if (scrollX < 0)
            scrollX = 0;
    }
    if (screenX - scrollX + 2 > width) {
        scrollX = -(width - screenX) + 2;
    }

    app_t::log("b:%d screenY:%d scrollY:%d", cursorBlock->lineNumber, cursorBlock->screenLine, scrollY);
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
        char* line = (char*)text.c_str();

        editor->completer.addLine(text);

        move(y + l, x);
        _clrtoeol(width);
        move(y + l++, x);

        if (text.length() < editor->scrollX) {
            continue;
        }

        // app_t::instance()->log("%s", line);
        int col = 0;
        for (int i = editor->scrollX;; i++) {
            size_t pos = i;
            if (col++ >= cols) {
                break;
            }

            char ch = line[i];
            if (ch == 0)
                break;

            bool hl = false;
            bool ul = false;

            colorPair = color_pair_e::NORMAL;
            colorPairSelected = color_pair_e::SELECTED;

            // syntax here
            if (blockData) {
                struct span_info_t span = spanAtBlock(blockData, i);
                if (span.length) {
                    colorPair = pairForColor(span.colorIndex, false);
                    colorPairSelected = pairForColor(span.colorIndex, true);
                }
            }
        
            // bracket
            if (cursorBracket1.bracket != -1 && cursorBracket2.bracket != -1) {
                if ((pos == cursorBracket1.position && lineNo == cursorBracket1.line) ||
                    (pos == cursorBracket2.position && lineNo == cursorBracket2.line)) {
                    attron(A_UNDERLINE);
                }
            }

            // cursors
            for (auto& c : cursors) {
                if (i == c.position() && b == c.block()) {
                    hl = true;
                    ul = c.hasSelection();
                }
                if (!c.hasSelection())
                    continue;

                cursor_position_t start = c.selectionStart();
                cursor_position_t end = c.selectionEnd();
                if (b->lineNumber < start.block->lineNumber || b->lineNumber > end.block->lineNumber)
                    continue;
                if (b == start.block && i < start.position)
                    continue;
                if (b == end.block && i > end.position)
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

    while (l < rows) {
        move(y + l, x);
        _clrtoeol(width);
        move(y + l++, x);
        // addch('~');
    }
}
