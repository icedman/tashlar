#include <curses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"

void editor_t::renderLine(const char* line, int offsetX, int offsetY, struct block_t* block, int relativeLine)
{
    if (!line) {
        return;
    }

    std::vector<struct cursor_t>* cursors = &document.cursors;

    int colorPair = color_pair_e::NORMAL;
    int colorPairSelected = color_pair_e::SELECTED;
    int wrapOffset = relativeLine * viewWidth;

    int skip = offsetX;
    char c;
    int idx = 0;
    int x = 0;
    while (c = line[idx++]) {
        if (skip-- > 0) {
            continue;
        }

        colorPair = color_pair_e::NORMAL;
        colorPairSelected = color_pair_e::SELECTED;

        if (c == '\t') {
            c = ' ';
        }

        if (block && cursors) {
            int rpos = idx - 1;
            int pos = block->position + idx - 1;
            int colorIdx = -1;

            // syntax here
            if (block && block->data) {
                struct blockdata_t* blockData = block->data.get();
                for (auto span : blockData->spans) {
                    if (rpos + wrapOffset >= span.start && rpos + wrapOffset < span.start + span.length) {
                        colorPair = pairForColor(span.colorIndex, false);
                        colorPairSelected = pairForColor(span.colorIndex, true);
                        break;
                    }
                }
            }

            // selection
            bool firstCursor = true;
            for (auto cur : *cursors) {
                if (firstCursor) {
                    if (pos + 1 == cur.position - wrapOffset) {
                        cursorScreenX = x + 1;
                        cursorScreenY = offsetY;
                    }
                    if (pos == cur.position - wrapOffset) {
                        cursorScreenX = x;
                        cursorScreenY = offsetY;
                    }
                }
                if (pos == cur.position - wrapOffset) {
                    if (firstCursor) {
                        wattron(win, A_REVERSE);
                    } else {
                        if (cur.hasSelection()) {
                            wattron(win, A_UNDERLINE);
                        }
                    }
                    if (colorIdx != -1) {
                        colorPair = 0;
                    }
                    colorPair = colorPairSelected;
                }
                firstCursor = false;

                if (!cur.hasSelection()) {
                    continue;
                }
                size_t startSel = cur.anchorPosition;
                size_t endSel = cur.position;
                if (startSel > endSel) {
                    startSel = cur.position + 1;
                    endSel = cur.anchorPosition + 1;
                }
                if (pos >= startSel - wrapOffset && pos < endSel - wrapOffset) {
                    colorPair = colorPairSelected;
                }
            }
        }

        wattron(win, COLOR_PAIR(colorPair));
        waddch(win, c);
        wattroff(win, COLOR_PAIR(colorPair));
        wattroff(win, A_REVERSE);
        wattroff(win, A_UNDERLINE);
        x++;
    }
}

static void setFormatFromStyle(size_t start, size_t length, style_t& style, const char* line, struct blockdata_t* blockData, std::string scope)
{
    int s = -1;
    for (int i = start; i < start + length; i++) {
        if (s == -1) {
            if (line[i] != ' ' && line[i] != '\t') {
                s = i;
            }
            continue;
        }
        if (line[i] == ' ' || i + 1 == start + length) {
            if (s != -1) {
                span_info_t span = {
                    .start = s,
                    .length = i - s + 1,
                    .red = style.foreground.red * 255,
                    .green = style.foreground.green * 255,
                    .blue = style.foreground.blue * 255
                };

                span.colorIndex = style.foreground.index; // nearestColor(span.red, span.green, span.blue);
                blockData->spans.push_back(span);
            }
            s = -1;
        }
    }
}

void editor_t::highlightBlock(struct block_t& block)
{
    if (!lang) {
        return;
    }

    if (!block.data) {
        block.data = std::make_shared<blockdata_t>();
        block.data->dirty = true;
    }

    if (!block.data->dirty) {
        return;
    }

    std::string text = block.text();

    struct blockdata_t* blockData = block.data.get();
    block_state_e previousBlockState = BLOCK_STATE_UNKNOWN;

    std::string str = text;
    str += "\n";

    bool firstLine = true;
    parse::stack_ptr parser_state = NULL;
    std::map<size_t, scope::scope_t> scopes;
    blockData->scopes.clear();
    blockData->spans.clear();

    const char* first = str.c_str();
    const char* last = first + text.length() + 1;

    struct block_t* prevBlock = block.previous;
    struct blockdata_t* prevBlockData = NULL;
    if (prevBlock) {
        prevBlockData = prevBlock->data.get();
    }
    if (prevBlockData) {
        previousBlockState = prevBlockData->state;
        parser_state = prevBlockData->parser_state;
        if (parser_state->rule) {
            blockData->lastPrevBlockRule = parser_state->rule->rule_id;
        }
        firstLine = !(parser_state != NULL);
    }

    if (!parser_state) {
        parser_state = lang->grammar->seed();
        firstLine = true;
    }

    parser_state = parse::parse(first, last, parser_state, scopes, firstLine);

    std::string prevScopeName;
    size_t si = 0;
    size_t n = 0;
    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    while (it != scopes.end()) {
        n = it->first;
        scope::scope_t scope = it->second;

        // if (settings->debug_scopes) {
        blockData->scopes.emplace(n, scope);
        // }

        std::string scopeName = to_s(scope);
        it++;

        if (n > si) {
            style_t s = theme->styles_for_scope(prevScopeName);
            setFormatFromStyle(si, n - si, s, first, blockData, prevScopeName);
        }

        prevScopeName = scopeName;
        // if (prevScopeName.find("variable") != std::string::npos) {
        //    prevScopeName = "parameter";
        // }
        si = n;
    }

    n = last - first;
    if (n > si) {
        style_t s = theme->styles_for_scope(prevScopeName);
        setFormatFromStyle(si, n - si, s, first, blockData, prevScopeName);
    }

    //----------------------
    // langauge config
    //----------------------

    //----------------------
    // find block comments
    //----------------------
    if (lang->blockCommentStart.length()) {
        size_t beginComment = text.find(lang->blockCommentStart);
        size_t endComment = text.find(lang->blockCommentEnd);
        style_t s = theme->styles_for_scope("comment");

        if (endComment == std::string::npos && (beginComment != std::string::npos || previousBlockState == BLOCK_STATE_COMMENT)) {
            blockData->state = BLOCK_STATE_COMMENT;
            int b = beginComment != std::string::npos ? beginComment : 0;
            int e = endComment != std::string::npos ? endComment : (last - first);
            setFormatFromStyle(b, e - b, s, first, blockData, "comment");
        } else {
            blockData->state = BLOCK_STATE_UNKNOWN;
            if (endComment != std::string::npos && previousBlockState == BLOCK_STATE_COMMENT) {
                setFormatFromStyle(0, endComment + lang->blockCommentEnd.length(), s, first, blockData, "comment");
            }
        }
    }

    blockData->parser_state = parser_state;
    blockData->dirty = false;

    //----------------------
    // mark next block for highlight
    // .. if necessary
    //----------------------
    if (parser_state->rule) {
        struct block_t* next = block.next;
        if (next && next->isValid()) {
            struct blockdata_t* nextBlockData = next->data.get();
            if (nextBlockData && parser_state->rule->rule_id != nextBlockData->lastPrevBlockRule) {
                nextBlockData->dirty = true;
            }
        }
    }
}

void editor_t::layoutBlock(struct block_t& block)
{
    block.lineCount = 1;
    if (app_t::instance()->lineWrap) {
        block.lineCount += block.length / viewWidth;
    }
}

void editor_t::renderBlock(struct block_t& block, int offsetX, int offsetY)
{
    std::string t = block.text();
    if (!t.length()) {
        t = " ";
    }

    char* str = (char*)t.c_str();

    for (int i = 0; i < block.lineCount; i++) {
        wmove(win, offsetY + i, 0);
        wclrtoeol(win);
        renderLine(str, offsetX, offsetY + i, &block, i);
        str += viewWidth;
    }
}
