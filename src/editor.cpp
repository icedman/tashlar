#include "editor.h"
#include "app.h"
#include "keyinput.h"

#include <iostream>
#include <sstream>

editor_t::editor_t()
    : view_t("editor")
{
    canFocus = true;
}

editor_t::~editor_t()
{
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

    history.push_back(op);

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
        document.open(strParam);
        createSnapshot();
        return;
    case SAVE: {
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
        app_t::log("paste %s", app_t::instance()->clipboard().c_str());
        inputBuffer = app_t::instance()->clipboard();
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
        if (mainCursor.hasSelection()) {
            cursor_t res = document.findNextOccurence(mainCursor, mainCursor.selectedText());
            if (!res.isNull()) {
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
        document.clearCursors();
        document.setCursor(mainCursor);
        break;

    case SELECT_ALL:
        document.clearCursors();
        mainCursor = document.cursor();
        mainCursor.moveStartOfDocument();
        mainCursor.moveEndOfDocument(true);
        document.setCursor(mainCursor);
        return;

    default:
        break;
    }

    for (auto& cur : cursors) {
        switch (_op) {

        case SELECT_WORD:
            cur.selectWord();
            break;

        case INDENT:
            break;
        case UNINDENT:
            break;

        case SELECT_LINE:
            cur.moveStartOfLine();
            cur.moveEndOfLine(true);
            break;
        case DUPLICATE_LINE:
            break;
        case DELETE_LINE:
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
                cur.eraseSelection();
                cur.setPosition(pos);
                cur.clearSelection();
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
                    cur.setPosition(document.blockAtLine(line), pos, _op == MOVE_CURSOR_ANCHORED);
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
            cur.moveNextBlock(height, _op == MOVE_CURSOR_NEXT_PAGE_ANCHORED);
            break;
        case MOVE_CURSOR_PREVIOUS_PAGE:
        case MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED:
            cur.movePreviousBlock(height, _op == MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED);
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
}

void editor_t::update(int delta)
{
    while (operations.size()) {
        operation_t op = operations.front();
        operations.erase(operations.begin());
        runOp(op);
    }

    while (inputBuffer.size()) {
        char c = inputBuffer[0];
        inputBuffer.erase(inputBuffer.begin());
        std::string t;
        t += c;
        operation_t op = { .op = INSERT, .params = t };
        if (c == '\n') {
            op.op = ENTER;
        }
        if (c == '\t') {
            op.op = TAB;
        }
        runOp(op);
    }
}

void editor_t::matchBracketsUnderCursor()
{
    cursor_t cursor = document.cursor();
    if (cursor.position() != cursorBracket1.absolutePosition) {
        cursorBracket1 = bracketAtCursor(cursor);
        if (cursorBracket1.bracket != -1) {
            cursor_t matchCursor = findBracketMatchCursor(cursorBracket1, cursor);
            cursorBracket2 = bracketAtCursor(matchCursor);
            cursorBracket1.absolutePosition = cursor.position();
            cursorBracket2.absolutePosition = matchCursor.position();
            cursorBracket1.line = 0;
            cursorBracket2.line = 0;
            // app_t::instance()->log("brackets %d %d", cursorBracket1.absolutePosition, cursorBracket2.absolutePosition);
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
    for (auto bracket : blockData->brackets) {
        if (bracket.position == p) {
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
    block_ptr folder = doc->blockAtLine(line);

    cursor_t openBracket = findLastOpenBracketCursor(folder);
    if (openBracket.isNull()) {
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
    operation_list items = history;

    if (items.size() == 0)
        return;
    while (items.size() > 1) {
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
        case PASTE:
        case UNDO:
            continue;
        default:
            break;
        }

        pushOp(op);
        update(0);
    }

    history = items;
}

void editor_t::createSnapshot()
{
    snapshot.save(document.blocks);
}

bool editor_t::input(char ch, std::string keySequence)
{
    if (!isFocused())
        return false;

    editor_t* editor = this;
    popup_t* popup = popup_t::instance();

    operation_e op = operationFromKeys(keySequence);

    if (popup->isVisible()) {
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

    if (ch == K_ESC || keySequence != "") {
        cursor_t mainCursor = editor->document.cursor();
        mainCursor.clearSelection();
        editor->document.clearCursors();
        editor->document.setCursor(mainCursor, true);
        return true;
    }

    std::string s;
    s += (char)ch;
    editor->pushOp("INSERT", s);
    return true;
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
