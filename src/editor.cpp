#include "editor.h"
#include "app.h"
#include "keyinput.h"

#include "render.h" // rows & cols

#include <iostream>
#include <sstream>

// 20K
// beyond this threshold, paste will use an additional file buffer
#define SIMPLE_PASTE_THRESHOLD 20000

editor_t::editor_t()
    : view_t("editor")
{
    cols = 0;
    rows = 0;
    canFocus = true;
    document.editor = this;
    _scrollToCursor = true;
    _foldedLines = 0;
}

editor_t::~editor_t()
{
    completer.cancel();
    highlighter.cancel();
}

void editor_t::pushOp(std::string op, std::string params)
{
    pushOp(operationFromName(op), params);
}

void editor_t::pushOp(operation_e op, std::string params)
{
    operation_t operation = {
        .op = op,
        .params = params
    };
    pushOp(operation);
}

void editor_t::pushOp(operation_t op)
{
    operations.push_back(op);
}

void editor_t::runOp(operation_t op)
{
    app_t* app = app_t::instance();

    if (snapshots.size()) {
        snapshot_t& snapshot = snapshots.back();
        snapshot.history.push_back(op);
    }

    int intParam = 0;
    try {
        intParam = std::stoi(op.params);
    } catch (std::exception e) {
    }

    std::string strParam = op.params;
    operation_e _op = op.op;

    //-------------------
    // handle selections
    //-------------------
    if (document.hasSelections()) {
        switch (_op) {
        case TAB:
        case ENTER:
        case INSERT: {
            operation_t d = { .op = DELETE_SELECTION };
            runOp(d);
            break;
        }
        case DELETE:
        case BACKSPACE: {
            operation_t d = { .op = DELETE_SELECTION };
            runOp(d);
            return;
        }

        default:
            break;
        }
    }

    cursor_list cursors = document.cursors;
    cursor_t mainCursor = document.cursor();
    cursor_util::sortCursors(cursors);

    if (mainCursor.block()) {
        app_t::log("%s %d %d", nameFromOperation(op.op).c_str(), mainCursor.block()->lineNumber, mainCursor.position());
    }

    switch (_op) {
    case CANCEL:
        operations.clear();
        return;
    case OPEN:
        document.open(strParam, false);
        createSnapshot();
        highlighter.run(this);
        completer.run(this);
        return;
    case SAVE: {
        if (document.fileName == "") {
            popup_t::instance()->prompt("filename");
            return;
        }
        app_t::log("saving %s", document.fileName.c_str());
        document.save();
        std::ostringstream ss;
        ss << "saved ";
        ss << document.fullPath;
        statusbar_t::instance()->setStatus(ss.str(), 3500);
        return;
    }
    case SAVE_AS:
        return;
    case SAVE_COPY:
        return;
    case UNDO:
        return;
    case REDO:
        return;
    case CLOSE:
        return;

    case PASTE:
        // app_t::log("paste %s", app_t::instance()->clipboard().c_str());
        if (!app_t::instance()->clipboard().length()) {
            return;
        }

        if (app_t::instance()->clipboard().length() < SIMPLE_PASTE_THRESHOLD) {
            inputBuffer = app_t::instance()->clipboard();
        } else {
            document.addBufferDocument(app_t::instance()->clipboard());
            document.insertFromBuffer(mainCursor, document.buffers.back());
            document.clearCursors();
            createSnapshot();
        }
        break;

    case CUT:
    case COPY:
        if (mainCursor.hasSelection()) {
            app_t::instance()->setClipboard(mainCursor.selectedText());
        }
        if (_op == COPY) {
            return;
        }
        _op = DELETE_SELECTION;
        break;

    case ADD_CURSOR_AND_MOVE_UP:
        document.addCursor(mainCursor);
        _op = MOVE_CURSOR_UP;
        break;
    case ADD_CURSOR_AND_MOVE_DOWN:
        document.addCursor(mainCursor);
        _op = MOVE_CURSOR_DOWN;
        break;
    case ADD_CURSOR_FOR_SELECTED_WORD:
        if (mainCursor.hasSelection() && mainCursor.selectedText().length()) {
            cursor_t res = document.findNextOccurence(mainCursor, mainCursor.selectedText());
            if (!res.isNull()) {
                res.normalizeSelection(mainCursor.isSelectionNormalized());
                document.addCursor(mainCursor);
                document.setCursor(res, true); // replace main cursor
                app_t::log("found %s at %s", mainCursor.selectedText().c_str(), res.block()->text().c_str());
                return;
            }
        } else {
            _op = SELECT_WORD;
        }
        break;
    case CLEAR_CURSORS:
        mainCursor.clearSelection();
        document.clearCursors();
        document.setCursor(mainCursor, true);
        break;

    case SELECT_ALL:
        document.clearCursors();
        mainCursor = document.cursor();
        mainCursor.moveStartOfDocument();
        mainCursor.moveEndOfDocument(true);
        document.setCursor(mainCursor);
        return;

    case TOGGLE_FOLD:
        toggleFold(mainCursor.block()->lineNumber);
        return;

    default:
        break;
    }

    for (auto& cur : cursors) {
        switch (_op) {

        case SELECT_WORD:
            cur.selectWord();
            break;

        case TOGGLE_COMMENT: {
            int count = cur.toggleLineComment();
            bool m = cur.isMultiBlockSelection();
            cursor_t c = cur;
            c.cursor.position = 0;
            c.anchor.position = 0;
            cursor_t a = cur;
            a.cursor = a.anchor;
            a.cursor.position = 0;
            a.anchor.position = 0;
            cursor_util::advanceBlockCursors(cursors, c, count);
            if (m) {
                cursor_util::advanceBlockCursors(cursors, a, count);
            }
            break;
        }
        case INDENT: {
            int count = cur.indent();
            bool m = cur.isMultiBlockSelection();
            cursor_t c = cur;
            c.cursor.position = 0;
            c.anchor.position = 0;
            cursor_t a = cur;
            a.cursor = a.anchor;
            a.cursor.position = 0;
            a.anchor.position = 0;
            cursor_util::advanceBlockCursors(cursors, c, count);
            if (m) {
                cursor_util::advanceBlockCursors(cursors, a, count);
            }
        } break;
        case UNINDENT: {
            int count = cur.unindent();
            if (count > 0) {
                bool m = cur.isMultiBlockSelection();
                cursor_t c = cur;
                c.cursor.position = 0;
                c.anchor.position = 0;
                cursor_t a = cur;
                a.cursor = a.anchor;
                a.cursor.position = 0;
                a.anchor.position = 0;
                if (count > 1) {
                    cursor_util::advanceBlockCursors(cursors, c, -count + 1);
                    if (m) {
                        cursor_util::advanceBlockCursors(cursors, a, -count + 1);
                    }
                }
            }
        } break;
        case SELECT_LINE:
            cur.moveStartOfLine();
            cur.moveEndOfLine(true);
            break;
        case DUPLICATE_LINE: {
            cur.moveStartOfLine();
            cur.moveEndOfLine(true);
            std::string text = cur.selectedText();
            cur.splitLine();
            cur.moveDown(1);
            cur.moveStartOfLine();
            cur.insertText(text);
            break;
        }
        case DELETE_LINE:
            cur.moveStartOfLine();
            cur.eraseText(cur.block()->length());
            cur.mergeNextLine();
            break;
        case MOVE_LINE_UP:
            break;
        case MOVE_LINE_DOWN:
            break;

        case CLEAR_SELECTION:
            break;
        case DUPLICATE_SELECTION:
            break;

        case DELETE_SELECTION:
            if (cur.hasSelection()) {
                cursor_position_t pos = cur.selectionStart();
                cursor_position_t end = cur.selectionEnd();
                bool snap = (end.block->lineNumber - pos.block->lineNumber > 100);
                cur.eraseSelection();
                cur.setPosition(pos);
                cur.clearSelection();
                int count = end.position - pos.position;
                if (count > 0 && end.block == pos.block) {
                    count++;
                    cursor_util::advanceBlockCursors(cursors, cur, -count);
                }
                if (snap) {
                    createSnapshot();
                }
            }
            break;

        case MOVE_CURSOR:
        case MOVE_CURSOR_ANCHORED: {
            std::vector<std::string> strings;
            std::istringstream f(strParam);
            std::string s;
            while (getline(f, s, ':')) {
                strings.push_back(s);
            }

            if (strings.size() == 2) {
                size_t line = 0;
                size_t pos = 0;
                try {
                    line = std::stoi(strings[0]);
                    pos = std::stoi(strings[1]);
                    block_ptr block = document.blockAtLine(line);
                    if (!block) {
                        block = document.lastBlock();
                    }
                    cur.setPosition(block, pos, _op == MOVE_CURSOR_ANCHORED);
                } catch (std::exception e) {
                }
            }
            //
        } break;

        case MOVE_CURSOR_LEFT:
        case MOVE_CURSOR_LEFT_ANCHORED:
            cur.moveLeft(intParam, _op == MOVE_CURSOR_LEFT_ANCHORED);
            break;
        case MOVE_CURSOR_RIGHT:
        case MOVE_CURSOR_RIGHT_ANCHORED:
            cur.moveRight(intParam, _op == MOVE_CURSOR_RIGHT_ANCHORED);
            break;
        case MOVE_CURSOR_UP:
        case MOVE_CURSOR_UP_ANCHORED:
            cur.moveUp(intParam, _op == MOVE_CURSOR_UP_ANCHORED);
            break;
        case MOVE_CURSOR_DOWN:
        case MOVE_CURSOR_DOWN_ANCHORED:
            cur.moveDown(intParam, _op == MOVE_CURSOR_DOWN_ANCHORED);
            break;
        case MOVE_CURSOR_NEXT_WORD:
        case MOVE_CURSOR_NEXT_WORD_ANCHORED:
            cur.moveNextWord(_op == MOVE_CURSOR_NEXT_WORD_ANCHORED);
            break;
        case MOVE_CURSOR_PREVIOUS_WORD:
        case MOVE_CURSOR_PREVIOUS_WORD_ANCHORED:
            cur.movePreviousWord(_op == MOVE_CURSOR_PREVIOUS_WORD_ANCHORED);
            break;
        case MOVE_CURSOR_START_OF_LINE:
        case MOVE_CURSOR_START_OF_LINE_ANCHORED:
            cur.moveStartOfLine(_op == MOVE_CURSOR_START_OF_LINE_ANCHORED);
            break;
        case MOVE_CURSOR_END_OF_LINE:
        case MOVE_CURSOR_END_OF_LINE_ANCHORED:
            cur.moveEndOfLine(_op == MOVE_CURSOR_END_OF_LINE_ANCHORED);
            break;
        case MOVE_CURSOR_START_OF_DOCUMENT:
        case MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED:
            cur.moveStartOfDocument(_op == MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED);
            break;
        case MOVE_CURSOR_END_OF_DOCUMENT:
        case MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED:
            cur.moveEndOfDocument(_op == MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED);
            break;
        case MOVE_CURSOR_NEXT_PAGE:
        case MOVE_CURSOR_NEXT_PAGE_ANCHORED:
            cur.moveNextBlock(rows, _op == MOVE_CURSOR_NEXT_PAGE_ANCHORED);
            break;
        case MOVE_CURSOR_PREVIOUS_PAGE:
        case MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED:
            cur.movePreviousBlock(rows, _op == MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED);
            break;

        case TAB: {
            std::string tab = "";
            for (int i = 0; i < app->tabSize; i++) {
                tab += " ";
            }
            cur.insertText(tab);
            cursor_util::advanceBlockCursors(cursors, cur, tab.length());
            cur.moveRight(tab.length());
            break;
        }
        case ENTER:
            cur.splitLine();
            cur.moveDown(1);
            cur.moveStartOfLine();
            break;
        case DELETE:
            if (cur.position() == cur.block()->length() - 1) {
                cur.mergeNextLine();
                break;
            }
            cur.eraseText(1);
            cursor_util::advanceBlockCursors(cursors, cur, -1);
            break;
        case BACKSPACE:
            if (cur.position() == 0) {
                if (cur.movePreviousBlock(1)) {
                    int newPos = cur.block()->length() - 1;
                    cur.mergeNextLine();
                    cur.setPosition(cur.block(), newPos, false);
                }
                break;
            }
            cur.moveLeft(1);
            cur.eraseText(1);
            cursor_util::advanceBlockCursors(cursors, cur, -1);
            break;
        case INSERT:
            cur.insertText(strParam);
            cursor_util::advanceBlockCursors(cursors, cur, strParam.length());
            cur.moveRight(strParam.length());
            popup_t::instance()->completion();
            break;

        default:
            break;
        }

        document.updateCursors(cursors);
    }

    document.clearDuplicateCursors();

    _scrollToCursor = true;
}

void editor_t::update(int delta)
{
    while (operations.size()) {
        operation_t op = operations.front();
        operations.erase(operations.begin());
        runOp(op);
    }

    // handle the inputBuffer
    std::string str;
    operation_t op;

    bool snap = inputBuffer.size() > 1000;

    while (inputBuffer.size()) {
        char c = inputBuffer[0];

        inputBuffer.erase(inputBuffer.begin());

        op.op = INSERT;
        op.params = "";

        switch (c) {
        case '\n': {
            if (str.length()) {
                op.params = str;
                runOp(op);
                str = "";
            }
            op.op = ENTER;
            runOp(op);
            break;
        }
        case '\t': {
            if (str.length()) {
                op.params = str;
                runOp(op);
                str = "";
            }
            op.op = TAB;
            break;
        }
        default:
            str += c;
            break;
        }
    }

    if (str.length()) {
        op.params = str;
        runOp(op);
        str = "";
    }

    if (snap) {
        createSnapshot();
    }
}

void editor_t::matchBracketsUnderCursor()
{
    cursor_t cursor = document.cursor();
    if (cursor.position() != cursorBracket1.position || cursor.block()->lineNumber != cursorBracket1.line) {
        cursorBracket1 = bracketAtCursor(cursor);
        if (cursorBracket1.bracket != -1) {
            cursor_t matchCursor = findBracketMatchCursor(cursorBracket1, cursor);
            cursorBracket2 = bracketAtCursor(matchCursor);
        }
    }
}

struct bracket_info_t editor_t::bracketAtCursor(struct cursor_t& cursor)
{
    bracket_info_t b;
    b.bracket = -1;

    block_ptr block = cursor.block();
    if (!block) {
        return b;
    }

    struct blockdata_t* blockData = block->data.get();
    if (!blockData) {
        return b;
    }

    size_t p = cursor.position();
    size_t l = cursor.block()->lineNumber;
    for (auto bracket : blockData->brackets) {
        if (bracket.position == p && bracket.line == l) {
            return bracket;
        }
    }

    return b;
}

cursor_t editor_t::cursorAtBracket(struct bracket_info_t bracket)
{
    cursor_t cursor;

    block_ptr block = document.cursor().block();
    while (block) {
        if (block->lineNumber == bracket.line) {
            cursor = document.cursor();
            cursor.setPosition(block, bracket.position);
            break;
        }
        if (block->lineNumber > bracket.line) {
            block = block->next();
        } else {
            block = block->previous();
        }
    }
    return cursor;
}

cursor_t editor_t::findLastOpenBracketCursor(block_ptr block)
{
    if (!block->isValid()) {
        return cursor_t();
    }

    struct blockdata_t* blockData = block->data.get();
    if (!blockData) {
        return cursor_t();
    }

    cursor_t res;
    for (auto b : blockData->foldingBrackets) {
        if (b.open) {
            if (res.isNull()) {
                res = document.cursor();
            }
            res.setPosition(block, b.position);
        }
    }

    return res;
}

cursor_t editor_t::findBracketMatchCursor(struct bracket_info_t bracket, cursor_t cursor)
{
    cursor_t cs = cursor;

    std::vector<bracket_info_t> brackets;
    block_ptr block = cursor.block();

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
                        cursor.setPosition(block, b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }

            block = block->next();
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
                        cursor.setPosition(block, b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }

            block = block->previous();
        }
    }
    return cursor_t();
}

void editor_t::toggleFold(size_t line)
{
    document_t* doc = &document;
    block_ptr folder = doc->blockAtLine(line + 1);

    cursor_t openBracket = findLastOpenBracketCursor(folder);
    if (openBracket.isNull()) {
        // app_t::log("> %d", folder->lineNumber);
        return;
    }

    bracket_info_t bracket = bracketAtCursor(openBracket);
    if (bracket.bracket == -1) {
        return;
    }
    cursor_t endBracket = findBracketMatchCursor(bracket, openBracket);
    if (endBracket.isNull()) {
        return;
    }

    block_ptr block = openBracket.block();
    block_ptr endBlock = endBracket.block();

    blockdata_t* blockData = block->data.get();
    if (!blockData) {
        return;
    }

    blockData->folded = !blockData->folded;
    block = block->next();
    while (block) {
        blockdata_t* targetData = block->data.get();
        targetData->folded = blockData->folded;
        targetData->foldable = false;
        targetData->foldedBy = blockData->folded ? line : 0;
        if (blockData->folded) {
            // block->setVisible(false);
            // block->setLineCount(0);
        } else {
            targetData->dirty = true;
            // block->setVisible(true);
            // block->setLineCount(1);
        }
        if (block == endBlock) {
            break;
        }
        block = block->next();
    }
}

void editor_t::undo()
{
    snapshot_t& snapshot = snapshots.back();
    operation_list items = snapshot.history;

    if (items.size() == 0)
        return;

    while (items.size() > 0) {
        auto lastOp = items.back();
        items.pop_back();

        bool endPop = false;
        switch (lastOp.op) {
        case TAB:
        case ENTER:
        case DELETE:
        case BACKSPACE:
        case INSERT:
        case CUT:
        case DUPLICATE_LINE:
        case DELETE_LINE:
        case INDENT:
        case UNINDENT:
            endPop = true;
        default:
            break;
        }

        if (endPop)
            break;
    }

    snapshot.restore(document.blocks);
    document.clearCursors();

    for (auto op : items) {

        switch (op.op) {
        case OPEN:
        case COPY:
        case PASTE:
        case SAVE:
        case UNDO:
            continue;
        default:
            break;
        }

        pushOp(op);
        update(0);
    }

    snapshot.history = items;

    if (snapshots.size() > 1 && items.size() == 0) {
        snapshots.pop_back();
    }
}

void editor_t::createSnapshot()
{
    snapshot_t snapshot;
    snapshot.save(document.blocks);
    snapshots.emplace_back(snapshot);
}

bool editor_t::input(char ch, std::string keySequence)
{
    if (!isFocused())
        return false;

    editor_t* editor = this;
    popup_t* popup = popup_t::instance();

    operation_e op = operationFromKeys(keySequence);

    if (popup->isVisible() && !inputBuffer.length()) {
        if (!popup->isCompletion()) {
            return false;
        }
        if (op == MOVE_CURSOR_UP || op == MOVE_CURSOR_DOWN || op == ENTER || op == TAB) {
            return popup->input(ch, keySequence);
        }
        if (ch == K_ESC) {
            popup->hide();
            return true;
        }
    }

    if (op == UNDO) {
        editor->undo();
        popup->hide();
        return true;
    }
    if (op != UNKNOWN) {
        editor->pushOp(op);
        popup->hide();
        return true;
    }

    if (ch == K_ESC) {
        editor->pushOp(CLEAR_CURSORS);
        return true;
    }

    if (keySequence != "") {
        return true;
    }

    std::string s;
    s += (char)ch;
    editor->pushOp(INSERT, s);
    return true;
}

void editor_t::mouseDown(int x, int y, int button, int clicks)
{
    int fw = render_t::instance()->fw;
    int fh = render_t::instance()->fh;

    int col = (x - this->x - padding) / fw;
    int row = (y - this->y - padding) / fh;

    // scroll
    if (clicks == 0) {
        if (row + 2 >= rows) {
            if (scrollY < maxScrollY) {
                scrollY++;
                return;
            }
        }

        if (row < 2) {
            if (scrollY > 0) {
                scrollY--;
                return;
            }
        }
    }

    block_list::iterator it = document.blocks.begin();
    if (scrollY > 0) {
        it += scrollY;
    }
    int l = 0;
    bool rowHit = false;
    while (it != document.blocks.end()) {
        block_ptr b = *it++;
        for (int i = 0; i < b->lineCount; i++) {
            if (l == row) {
                rowHit = true;
                // app_t::log(">%d %d", b->lineNumber, 0);
                std::ostringstream ss;
                ss << (b->lineNumber + 1);
                ss << ":";
                ss << col + (i * cols);
                if (clicks == 0 || _keyMods()) {
                    pushOp(MOVE_CURSOR_ANCHORED, ss.str());
                } else if (clicks == 1) {
                    pushOp(MOVE_CURSOR, ss.str());
                } else {
                    pushOp(MOVE_CURSOR, ss.str());
                    pushOp(SELECT_WORD, "");
                }
            }
            l++;
        }
        if (l > rows)
            break;
    }

    // app_t::log("click %d %d %d", rowHit, clicks, l);
    if (!rowHit && clicks == 1) {
        if (_keyMods()) {
            pushOp(MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED, "");
        } else {
            pushOp(MOVE_CURSOR_END_OF_DOCUMENT, "");
        }
    }

    if (clicks > 0) {
        view_t::setFocus(this);
        popup_t::instance()->hide();
    }
}

void editor_t::mouseDrag(int x, int y, bool within)
{
    if (!within)
        return;
    mouseDown(x, y, 1, 0);
}

void editor_t::applyTheme()
{
    for (auto b : document.blocks) {
        if (b->data) {
            b->data->dirty = true;
        }
    }

    highlighter.theme = app_t::instance()->theme;
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

    rows = height / render_t::instance()->fh;
    cols = width / render_t::instance()->fw;

    maxScrollX = 0;
    maxScrollY = document.lastBlock()->lineNumber - (rows * 2 / 3);

    // app_t::log(">max %d", maxScrollY);

    if (maxScrollY < 0)
        scrollY = 0;
}

void editor_t::ensureVisibleCursor()
{
    if (width == 0 || height == 0 || !isVisible())
        return;

    cursor_t mainCursor = document.cursor();

    // update scroll
    block_ptr cursorBlock = mainCursor.block();

    int lookAheadX = (cols / 3);

    int adjust = 0;
    if (app_t::instance()->lineWrap) {
        block_list::iterator it = document.blocks.begin();
        if (scrollY > document.blocks.size()) scrollY = document.blocks.size() - 1;
        if (scrollY > 0) {
            it += scrollY;
        }
        int l = 0;
        while (it != document.blocks.end()) {
            block_ptr b = *it++;
            if (b == cursorBlock)
                break;
            if (l > rows)
                break;

            if (b->lineCount > 1) {
                adjust += (b->lineCount - 1);
            }
        }
    }

    // scrollY
    int lookAheadY = 0; // (cols/5);
    int screenY = cursorBlock->lineNumber;
    if (screenY - scrollY < lookAheadY) {
        scrollY = screenY - lookAheadY;
        if (scrollY < 0)
            scrollY = 0;
    } else if (screenY - scrollY + 1 + adjust > (rows + _foldedLines)) {
        scrollY = -((rows + _foldedLines) - screenY) + 1 + adjust;
    }

    if (scrollY < 0)
        scrollY = 0;

    // scrollX
    int screenX = mainCursor.position();
    if (screenX - scrollX < lookAheadX) {
        scrollX = screenX - lookAheadX;
        if (scrollX < 0)
            scrollX = 0;
    }
    if (screenX - scrollX + 2 > cols) {
        scrollX = -(cols - screenX) + 2;
    }

    if (app_t::instance()->lineWrap) {
        scrollX = 0;
    }

    document.setColumns(cols);

    if (scrollY > cursorBlock->lineNumber) {
        scrollY = cursorBlock->lineNumber - (rows * 2 / 3);
        if (scrollY < 0) {
            scrollY = 0;
        }
    }

    // app_t::log(">>line:%d scroll:%d rows:%d", cursorBlock->lineNumber, scrollY, rows);
}

void editor_t::preRender()
{
    if (_scrollToCursor) {
        ensureVisibleCursor();
        _scrollToCursor = false;
    }
}

void editor_t::render()
{
    if (!isVisible())
        return;

    struct cursor_t mainCursor = document.cursor();
    cursor_list cursors = document.cursors;

    matchBracketsUnderCursor();

    // app_t::log("scroll %d %d", scrollX, scrollY);

    size_t cx;
    size_t cy;

    int l = 0;
    block_list::iterator it = document.blocks.begin();

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
        block_ptr b = *it++;
        highlighter.highlightBlock(b);
        if (c++ > rows + preceedingBlocks)
            break;
    }

    //---------------
    it = document.blocks.begin();
    if (scrollY > 0) {
        it += scrollY;
    }

    _foldedLines = 0;

    bool hlMainCursor = document.cursors.size() == 1 && !mainCursor.hasSelection();
    bool firstLine = true;
    while (it != document.blocks.end()) {
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

            if (text.length() < scrollX) {
                continue;
            }

            if (blockData && blockData->folded && !blockData->foldable) {
                l--;
                _foldedLines += b->lineCount;
                continue;
            }

            if (l >= rows + 1)
                break;

            // app_t::instance()->log("%s", line);
            int col = 0;
            for (int i = scrollX;; i++) {
                size_t pos = i + (sl * cols);
                if (col++ >= cols) {
                    break;
                }

                char ch = line[pos];
                if (ch == 0)
                    break;

                bool hl = false;
                bool ul = false;
                bool bl = false; // bold
                bool il = false; // italic

                colorPair = color_pair_e::NORMAL;
                colorPairSelected = color_pair_e::SELECTED;

                // syntax here
                if (blockData) {
                    struct span_info_t span = spanAtBlock(blockData, pos);
                    bl = span.bold;
                    il = span.italic;
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
                        bl = c.hasSelection();
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
                    _underline(true);
                }
                if (bl) {
                    _bold(true);
                }
                if (il) {
                    _italic(true);
                }

                _attron(_color_pair(colorPair));
                _addch(ch);
                _attroff(_color_pair(colorPair));
                _bold(false);
                _italic(false);
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
