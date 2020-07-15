#include <curses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <unicode/utf8.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "gutter.h"
#include "statusbar.h"
#include "tabbar.h"

#include "keybinding.h"

bool editor_proxy_t::processCommand(command_e cmd, char ch)
{
    return app_t::instance()->currentEditor->processCommand(cmd, ch);
}

void editor_proxy_t::layout(int w, int h)
{
    app_t::instance()->currentEditor->layout(w, h);
}

void editor_proxy_t::render()
{
    app_t::instance()->currentEditor->render();
}

void editor_proxy_t::renderCursor()
{
    app_t::instance()->currentEditor->renderCursor();
}

bool editor_proxy_t::isFocused()
{
    app_t::instance()->currentEditor->isFocused();
}

static struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos)
{
    span_info_t res;
    res.length = 0;
    for (auto span : blockData->spans) {
        if (span.length == 0) {
            continue;
        }
        if (pos >= span.start && pos < span.start + span.length) {
            res = span;
            if (res.state != BLOCK_STATE_UNKNOWN) {
                return res;
            }
        }
    }

    return res;
}

void editor_t::renderLine(const char* line, int offsetX, int offsetY, struct block_t* block, int relativeLine)
{
    if (!line) {
        return;
    }

    std::vector<struct cursor_t>* cursors = &document.cursors;

    bool hasFocus = isFocused();

    struct blockdata_t* blockData = block->data.get();

    int colorPair = color_pair_e::NORMAL;
    int colorPairSelected = color_pair_e::SELECTED;
    int wrapOffset = relativeLine * viewWidth;

    int skip = offsetX;
    int c;
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

        int rpos = idx - 1;
        int pos = block->position + idx - 1;
        int colorIdx = -1;

        // syntax here
        if (block && block->data) {
            struct span_info_t span = spanAtBlock(blockData, rpos + wrapOffset);
            if (span.length) {
                colorPair = pairForColor(span.colorIndex, false);
                colorPairSelected = pairForColor(span.colorIndex, true);
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

        wattron(win, COLOR_PAIR(colorPair));

        if (!isprint(c)) {
            // if (c < 0) {
            // todo.. learn utf-8 read utf8.h
            // }
            c = '?';
            waddch(win, c);
        } else {
            waddch(win, c);
        }

        wattroff(win, COLOR_PAIR(colorPair));
        wattroff(win, A_REVERSE);
        wattroff(win, A_UNDERLINE);
        x++;
    }
}

static void setFormatFromStyle(size_t start, size_t length, style_t& style, const char* line, struct blockdata_t* blockData, std::string scope)
{
    int s = -1;

    block_state_e state;
    if (scope.find("comment") != std::string::npos) {
        state = BLOCK_STATE_COMMENT;
    } else if (scope.find("string") != std::string::npos) {
        state = BLOCK_STATE_STRING;
    } else {
        state = BLOCK_STATE_UNKNOWN;
    }

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
                    .colorIndex = style.foreground.index,
                    .state = state
                };
                blockData->spans.insert(blockData->spans.begin(), 1, span);
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

    struct blockdata_t* blockData = block.data.get();

    bool previousState = blockData->state;
    block_state_e previousBlockState = BLOCK_STATE_UNKNOWN;

    struct block_t* prevBlock = block.previous;
    struct blockdata_t* prevBlockData = NULL;
    if (prevBlock) {
        prevBlockData = prevBlock->data.get();
    }

    std::string text = block.text();
    std::string str = text;
    str += "\n";

    //--------------
    // free minimap line cache
    //--------------
    for (int i = 0; i < 4; i++) {
        int ln = block.lineNumber - 4 + i;
        if (ln < 0)
            continue;
        struct block_t& pb = block.document->blocks[ln];
        if (pb.data && pb.data->dots) {
            free(pb.data->dots);
            pb.data->dots = 0;
        }
    }

    //--------------
    // call the grammar parser
    //--------------
    bool firstLine = true;
    parse::stack_ptr parser_state = NULL;
    std::map<size_t, scope::scope_t> scopes;
    blockData->scopes.clear();
    blockData->spans.clear();

    const char* first = str.c_str();
    const char* last = first + text.length() + 1;

    if (prevBlockData) {
        previousBlockState = prevBlockData->state;
        parser_state = prevBlockData->parser_state;
        if (parser_state && parser_state->rule) {
            blockData->lastPrevBlockRule = parser_state->rule->rule_id;
        }
        firstLine = !(parser_state != NULL);
    }

    if (!parser_state) {
        parser_state = lang->grammar->seed();
        firstLine = true;
    }

    if (text.length() > 500) {
        // too long to parse
    } else {
        parser_state = parse::parse(first, last, parser_state, scopes, firstLine);
    }

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

    //----------------------
    // gather brackets
    //----------------------
    blockData->brackets.clear();
    blockData->foldable = false;
    blockData->foldingBrackets.clear();
    if (lang->brackets) {
        std::vector<bracket_info_t> brackets;
        for (char* c = (char*)first; c < last;) {
            bool found = false;

            struct span_info_t span = spanAtBlock(blockData, c - first);
            if (span.length && (span.state == BLOCK_STATE_COMMENT || span.state == BLOCK_STATE_STRING)) {
                c++;
                continue;
            }

            // opening
            int i = 0;
            for (auto b : lang->bracketOpen) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .line = block.lineNumber,
                        .position = l,
                        .bracket = i,
                        .open = true });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            // closing
            i = 0;
            for (auto b : lang->bracketClose) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .line = block.lineNumber,
                        .position = l,
                        .bracket = i,
                        .open = false });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            c++;
        }

        blockData->brackets = brackets;

        // bracket pairing
        for (auto b : brackets) {
            if (!b.open && blockData->foldingBrackets.size()) {
                auto l = blockData->foldingBrackets.back();
                if (l.open && l.bracket == b.bracket) {
                    blockData->foldingBrackets.pop_back();
                } else {
                    // std::cout << "error brackets" << std::endl;
                }
                continue;
            }
            blockData->foldingBrackets.push_back(b);
        }

        // hack for if-else-
        if (blockData->foldingBrackets.size() == 2) {
            if (blockData->foldingBrackets[0].open != blockData->foldingBrackets[1].open && blockData->foldingBrackets[0].bracket == blockData->foldingBrackets[1].bracket) {
                blockData->foldingBrackets.clear();
            }
        }

        // format brackets with scope
        // style_t s = theme->styles_for_scope("bracket");
        // for (auto b : blockData->brackets) {
        // setFormatFromStyle(b.position, 1, s, first, blockData, "bracket");
        // }

        if (blockData->foldingBrackets.size()) {
            auto l = blockData->foldingBrackets.back();
            blockData->foldable = l.open;
        }

        // app_t::instance()->log("brackets %d %d", blockData->brackets.size(), blockData->foldingBrackets.size());
    }

    blockData->parser_state = parser_state;
    blockData->dirty = false;
    blockData->lastRule = parser_state->rule->rule_id;
    if (blockData->state == BLOCK_STATE_COMMENT) {
        blockData->lastRule = BLOCK_STATE_COMMENT;
    }

    //----------------------
    // mark next block for highlight
    // .. if necessary
    //----------------------
    if (blockData->lastRule) {
        struct block_t* next = block.next;
        if (next && next->isValid()) {
            struct blockdata_t* nextBlockData = next->data.get();
            //if (nextBlockData && blockData->lastRule != nextBlockData->lastPrevBlockRule) {
            //    nextBlockData->dirty = true;
            //}
            if (nextBlockData) {
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
    std::string t = block.text() + " ";
    char* str = (char*)t.c_str();

    for (int i = 0; i < block.lineCount; i++) {
        wmove(win, offsetY + i, 0);
        wclrtoeol(win);
        renderLine(str, offsetX, offsetY + i, &block, i);
        str += viewWidth;
    }
}

void editor_t::render()
{
}

void editor_t::renderCursor()
{
    /*
    struct document_t* doc = &document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (block.isValid()) {
        wmove(win, cursorScreenY, cursorScreenX);
        curs_set(1);
        // app_t::instance()->log("%d %d", cursorScreenY, cursorScreenX);
    } else {
        curs_set(0);
    }
    */
    curs_set(0);
    // do our own blink?
}

void editor_t::layout(int w, int h)
{
    viewX = 0;
    viewY = 0;
    viewWidth = w;
    viewHeight = h;

    struct app_t* app = app_t::instance();
    if (app->showStatusBar) {
        viewHeight -= app->statusbar->viewHeight;
    }
    if (app->showSidebar) {
        int explorerWidth = app->explorer->viewWidth;
        viewWidth -= explorerWidth;
        viewX += explorerWidth;
    }
    if (app->showTabbar) {
        int tabbarHeight = app->tabbar->viewHeight;
        viewHeight -= tabbarHeight;
        viewY += tabbarHeight;
    }
    if (app->showGutter) {
        int gutterWidth = app->gutter->viewWidth;
        viewWidth -= gutterWidth;
        viewX += gutterWidth;
    }
    if (app->showMinimap) {
        int minimapWidth = app->minimap->viewWidth;
        viewWidth -= minimapWidth;
    }
}

bool editor_t::processCommand(command_e cmd, char ch)
{
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // global
    //-----------------
    switch (cmd) {
    case CMD_CANCEL:
        doc->clearCursors();
        return false;

    case CMD_SAVE:
        doc->save();
        app->statusbar->setStatus("saved", 2000);
        return true;

    case CMD_PASTE:
        if (app->clipBoard.length() && app->clipBoard.length() < SIMPLE_PASTE_THRESHOLD) {
            app->inputBuffer = app->clipBoard;
        } else {
            doc->addSnapshot();
            doc->history().begin();
            doc->addBufferDocument(app->clipBoard);
            app->clipBoard = "";

            // cursorInsertText(&cursor, "/* WARNING: pasting very large buffer is not yet ready */");

            doc->insertFromBuffer(cursor, doc->buffers.back());

            cursorMovePosition(&cursor, cursor_t::EndOfDocument);
            doc->history().addPasteBuffer(cursor, doc->buffers.back());

            doc->history().end();
            doc->addSnapshot();
            doc->clearCursors();
        }
        return true;

    case CMD_UNDO:
        doc->undo();
        return true;

    default:
        break;
    }

    // proceed only if got focus
    if (!isFocused()) {
        return false;
    }

    std::string s;
    s += (char)ch;

    if (cmd == CMD_ENTER || ch == ENTER || s == "\n") {
        cmd = CMD_SPLIT_LINE;
    }

    return processEditorCommand(cmd, ch);
}

void editor_t::update(int frames)
{
    if (isFocused()) {
        renderCursor();
    }
}
