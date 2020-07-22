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

#define HIGHLIGHT_PRECEEDING_BLOCKS 32

bool editor_proxy_t::processCommand(command_t cmd, char ch)
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

void editor_proxy_t::update(int frames)
{
    app_t::instance()->currentEditor->update(frames);
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
    while (c = line[wrapOffset + idx++]) {
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
            if (firstCursor && hasFocus) {
                if (pos + 1 == cur.position() - wrapOffset) {
                    cursorScreenX = x + 1;
                    cursorScreenY = offsetY;
                }
                if (pos == cur.position() - wrapOffset) {
                    cursorScreenX = x;
                    cursorScreenY = offsetY;
                }
            }
            if (pos == cur.position() - wrapOffset) {
                if (firstCursor && hasFocus) {
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

            // brackets matching
            if (cursorBracket1.bracket != -1 && cursorBracket2.bracket != -1) {
                if ((pos == cursorBracket1.absolutePosition - wrapOffset) || (pos == cursorBracket2.absolutePosition - wrapOffset)) {
                    wattron(win, A_UNDERLINE);
                }
            }

            if (!cur.hasSelection()) {
                continue;
            }
            size_t startSel = cur.anchorPosition();
            size_t endSel = cur.position();
            if (startSel > endSel) {
                startSel = cur.position() + 1;
                endSel = cur.anchorPosition() + 1;
            }
            if (pos + wrapOffset >= startSel && pos + wrapOffset < endSel) {
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

void editor_t::highlightPreceedingBlocks(struct block_t& block)
{
    struct block_t* end = &block;
    struct block_t* start = &block;
    for (int i = 0; i < HIGHLIGHT_PRECEEDING_BLOCKS; i++) {
        if (!start->previous) {
            break;
        }
        start = start->previous;
    }

    while (start && start != end) {

        if (start->data && !start->data->dirty) {
            break;
        }

        // app_t::instance()->log("<< %d", start->lineNumber);
        highlightBlock(*start);
        start = start->next;
    }
}

void editor_t::gatherBrackets(struct block_t& block, char* first, char* last)
{
    struct blockdata_t* blockData = block.data.get();
    if (!block.data) {
        block.data = std::make_shared<blockdata_t>();
        block.data->dirty = true;
        blockData = block.data.get();
    }

    if (blockData->brackets.size()) {
        return;
    }

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
                        .absolutePosition = block.position + l,
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
                        .absolutePosition = block.position + l,
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

    frames = 15000;

    // app_t::instance()->log("highlight %d", block.uid);

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
    // for minimap line cache
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

    if (lang->lineComment.length()) {
        // comment out until the end
    }

    //----------------------
    // reset brackets
    //----------------------
    blockData->brackets.clear();
    blockData->foldable = false;
    blockData->foldingBrackets.clear();
    gatherBrackets(block, (char*)first, (char*)last);

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
        // str += viewWidth;
    }
}

void editor_t::render()
{
}

void editor_t::renderCursor()
{
    curs_set(0);
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

bool editor_t::processCommand(command_t cmdt, char ch)
{
    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    //-----------------
    // global
    //-----------------
    switch (cmd) {
    case CMD_CANCEL:
        doc->clearCursors();
        return false;

    case CMD_SAVE_AS:
        doc->saveAs(cmdt.args.c_str(), true);
        app->statusbar->setStatus("saved copy", 2000);
        return true;

    case CMD_SAVE_COPY:
        doc->saveAs(cmdt.args.c_str());
        app->statusbar->setStatus("saved copy", 2000);
        return true;

    case CMD_SAVE:
        doc->save();
        app->statusbar->setStatus("saved", 2000);
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

void editor_t::update(int tick)
{
    if (frames > 0) {
        frames -= tick;
        if (frames <= 0) {
            highlightPreceedingBlocks(*document.cursor().block());
            frames = 0;
        }
    }
}

void editor_t::matchBracketsUnderCursor()
{
    struct cursor_t cursor = app_t::instance()->currentEditor->document.cursor();
    if (cursor.position() != cursorBracket1.absolutePosition) {
        cursorBracket1 = bracketAtCursor(cursor);
        if (cursorBracket1.bracket != -1) {
            struct cursor_t matchCursor = findBracketMatchCursor(cursorBracket1, cursor);
            cursorBracket2 = bracketAtCursor(matchCursor);
            cursorBracket1.absolutePosition = cursor.position();
            cursorBracket2.absolutePosition = matchCursor.position();
            cursorBracket1.line = 0;
            cursorBracket2.line = 0;
            app_t::instance()->refresh();
            // app_t::instance()->log("brackets %d %d", cursorBracket1.absolutePosition, cursorBracket2.absolutePosition);
        }
    }
}

struct bracket_info_t editor_t::bracketAtCursor(struct cursor_t& cursor)
{
    bracket_info_t b;
    b.bracket = -1;;

    struct block_t* block = cursor.block();
    if (!block) {
        return b;
    }

    struct blockdata_t* blockData = block->data.get();
    if (!blockData) {
        return b;
    }

    size_t p = cursor.position() - block->position;
    for (auto bracket : blockData->brackets) {
        if (bracket.position == p) {
            return bracket;
        }
    }

    return b;
}

struct cursor_t editor_t::cursorAtBracket(struct bracket_info_t bracket)
{
    struct cursor_t cursor;

    struct block_t* block = document.cursor().block();
    while (block) {
        if (block->lineNumber == bracket.line) {
            cursor = document.cursor();
            cursorSetPosition(&cursor, block->position + bracket.position);
            break;
        }
        if (block->lineNumber > bracket.line) {
            block = block->next;
        } else {
            block = block->previous;
        }
    }
    return cursor;
}

struct cursor_t editor_t::findLastOpenBracketCursor(struct block_t block)
{
    if (!block.isValid()) {
        return cursor_t();
    }

    struct blockdata_t* blockData = block.data.get();
    if (!blockData) {
        return cursor_t();
    }

    struct cursor_t res;
    for (auto b : blockData->foldingBrackets) {
        if (b.open) {
            if (res.isNull()) {
                res = document.cursor();
            }
            cursorSetPosition(&res, block.position + b.position);
        }
    }

    return res;
}

struct cursor_t editor_t::findBracketMatchCursor(struct bracket_info_t bracket, struct cursor_t cursor)
{
    struct cursor_t cs = cursor;

    std::vector<bracket_info_t> brackets;
    struct block_t* block = cursor.block();

    if (bracket.open) {

        while (block) {
            struct blockdata_t* blockData = block->data.get();
            if (!blockData) {
                break;
            }

            for (auto b : blockData->brackets) {
                if (b.line == bracket.line && b.position < bracket.position) {
                    continue;
                }

                if (!b.open) {
                    auto l = brackets.back();
                    if (l.open && l.bracket == b.bracket) {
                        brackets.pop_back();
                    } else {
                        // error .. unpaired?
                        return cursor_t();
                    }

                    if (!brackets.size()) {
                        // std::cout << "found end!" << std::endl;
                        cursorSetPosition(&cursor, block->position + b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }

            block = block->next;
        }

    } else {

        // reverse
        while (block) {
            struct blockdata_t* blockData = block->data.get();
            if (!blockData) {
                break;
            }

            // for(auto b : blockData->brackets) {
            for (auto it = blockData->brackets.rbegin(); it != blockData->brackets.rend(); ++it) {
                bracket_info_t b = *it;
                if (b.line == bracket.line && b.position > bracket.position) {
                    continue;
                }

                if (b.open) {
                    auto l = brackets.back();
                    if (!l.open && l.bracket == b.bracket) {
                        brackets.pop_back();
                    } else {
                        // error .. unpaired?
                        return cursor_t();
                    }

                    if (!brackets.size()) {
                        // std::cout << "found begin!" << std::endl;
                        cursorSetPosition(&cursor, block->position + b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }

            block = block->previous;
        }
    }
    return cursor_t();
}

void editor_t::toggleFold(size_t line)
{
    struct document_t* doc = &document;
    ;
    struct block_t folder;

    for (auto b : doc->blocks) {
        if (b.lineNumber == line) {
            folder = b;
            break;
        }
    }

    struct cursor_t openBracket = findLastOpenBracketCursor(folder);
    if (openBracket.isNull()) {
        return;
    }

    bracket_info_t bracket = bracketAtCursor(openBracket);
    if (bracket.bracket == -1) {
        return;
    }
    struct cursor_t endBracket = findBracketMatchCursor(bracket, openBracket);
    if (endBracket.isNull()) {
        return;
    }

    struct block_t* block = openBracket.block();
    struct block_t* endBlock = endBracket.block();

    struct blockdata_t* blockData = block->data.get();
    if (!blockData) {
        return;
    }

    blockData->folded = !blockData->folded;
    block = block->next;
    while (block) {
        struct blockdata_t* targetData = block->data.get();
        targetData->folded = blockData->folded;
        targetData->foldable = false;
        targetData->foldedBy = blockData->folded ? line : 0;
        if (blockData->folded) {
            // block.setVisible(false);
            // block.setLineCount(0);
        } else {
            targetData->dirty = true;
            // block.setVisible(true);
            // block.setLineCount(1);
        }
        if (block == endBlock) {
            break;
        }
        block = block->next;
    }
}
