#include "cursor.h"
#include "app.h"
#include "document.h"
#include "search.h"
#include "util.h"

#include <algorithm>

size_t cursor_position_t::absolutePosition()
{
    if (!block) {
        return 0;
    }
    return block->position + position;
}

cursor_t::cursor_t()
    : uid(0)
    , _document(0)
    , _preferredRelativePosition(0)
{
}

struct document_t* cursor_t::document()
{
    return _document;
}

struct block_t* cursor_t::block()
{
    return _position.block;
}

bool cursor_t::isNull()
{
    return _document == 0 || block() == 0 || !block()->isValid();
}

bool cursor_t::hasSelection()
{
    if (!_anchor.block) {
        return false;
    }

    return _anchor.block != _position.block || (_anchor.block == _position.block && _anchor.position != _position.position);
}

void cursor_t::flipAnchor()
{
    cursor_position_t t = _anchor;
    _anchor = _position;
    _position = t;
}

struct cursor_t cursor_t::selectionStartCursor()
{
    struct cursor_t cur1;
    cur1._position = _position;

    struct cursor_t cur2;
    cur2._position = _anchor;

    cur1._document = _document;
    cur2._document = _document;
    return _position.absolutePosition() < _anchor.absolutePosition() ? cur1 : cur2;
}

struct cursor_t cursor_t::selectionEndCursor()
{
    struct cursor_t cur1;
    cur1._position = _position;

    struct cursor_t cur2;
    cur2._position = _anchor;

    return _position.absolutePosition() < _anchor.absolutePosition() ? cur2 : cur1;
}

size_t cursor_t::selectionStart()
{
    return selectionStartCursor().position();
}

size_t cursor_t::selectionEnd()
{
    return selectionEndCursor().position();
}

std::vector<struct block_t*> cursor_t::selectedBlocks()
{
    std::vector<struct block_t*> blocks;

    if (!hasSelection()) {
        return blocks;
    }

    struct document_t* doc = document();
    struct cursor_t cur = selectionStartCursor();
    struct cursor_t curEnd = selectionEndCursor();

    // for selection spanning multiple blocks
    struct block_t* startBlock = cur.block();
    struct block_t* endBlock = curEnd.block();

    if (!startBlock->isValid()) {
        return blocks;
    }
    if (!endBlock->isValid()) {
        return blocks;
    }

    blocks.push_back(startBlock);
    while (startBlock != endBlock) {
        startBlock = startBlock->next;
        blocks.push_back(startBlock);
    }

    return blocks;
}

bool cursor_t::isMultiBlockSelection()
{
    return false;
}

void cursor_t::clearSelection()
{
    _anchor = _position;
    app_t::instance()->log("cursor clear selections");
}

size_t cursor_t::position()
{
    return _position.absolutePosition();
}

size_t cursor_t::anchorPosition()
{
    return _anchor.absolutePosition();
}

size_t cursor_t::relativePosition()
{
    return _position.position;
}

void cursor_t::setPosition(struct block_t* block, size_t pos)
{
    if (pos >= block->length) {
        pos = block->length - 1;
    }
    _position.block = block;
    _position.position = pos;
}

void cursor_t::setAnchor(struct block_t* block, size_t pos)
{
    if (pos >= block->length) {
        pos = block->length - 1;
    }
    _anchor.block = block;
    _anchor.position = pos;
}

bool cursorMovePosition(struct cursor_t* cursor, enum cursor_t::Move move, bool keepAnchor, int count)
{
    struct app_t* app = app_t::instance();
    int viewWidth = app->currentEditor->viewWidth;
    bool wrap = app->lineWrap;

    struct block_t& block = *cursor->block();
    if (!block.isValid()) {
        return false;
    }

    struct cursor_position_t prevPos = cursor->_position;

    count--;
    size_t relativePosition = cursor->relativePosition();

    if ((move == cursor_t::Move::Up || move == cursor_t::Move::Down) && cursor->_preferredRelativePosition != 0) {
        relativePosition = cursor->_preferredRelativePosition;
    }

    std::vector<search_result_t> search_results;
    bool extractWords = false;

    if (move == cursor_t::Move::WordLeft || move == cursor_t::Move::WordRight) {

        if (move == cursor_t::Move::WordLeft && relativePosition == 0) {
            if (cursorMovePosition(cursor, cursor_t::Move::Up, keepAnchor)) {
                cursorMovePosition(cursor, cursor_t::Move::EndOfLine, keepAnchor);
            }
            return true;
        }
        if (move == cursor_t::Move::WordRight && relativePosition + 1 == block.length) {
            if (cursorMovePosition(cursor, cursor_t::Move::Down, keepAnchor)) {
                cursorMovePosition(cursor, cursor_t::Move::StartOfLine, keepAnchor);
            }
            return true;
        }

        extractWords = true;
    }

    if (!(move == cursor_t::Move::Up || move == cursor_t::Move::Down)) {
        cursor->_preferredRelativePosition = 0;
    } else {
        if (cursor->_preferredRelativePosition == 0) {
            cursor->_preferredRelativePosition = relativePosition;
        }
    }

    if (extractWords) {
        search_results = search_t::instance()->findWords(block.text());
    }

    // app_t::instance()->log("block %d position: %d", cursor->block()->lineNumber, cursor->relativePosition());

    switch (move) {
    case cursor_t::Move::Left:
        if (cursor->_position.position > 0) {
            cursor->_position.position--;
        } else {
            if (block.previous) {
                cursor->setPosition(block.previous, block.previous->length - 1);
            }
        }
        break;

    case cursor_t::Move::Right:
        cursor->_position.position++;
        if (cursor->_position.position >= block.length) {
            cursor->_position.position--;
            if (block.next) {
                cursor->setPosition(block.next, 0);
            }
        }
        break;

    case cursor_t::Move::Up:
        if (wrap && block.length > viewWidth) {
            int p = cursor->_position.position - viewWidth;
            if (p >= 0) {
                cursor->setPosition(&block, p);
                break;
            }
        }
        if (!block.previous) {
            // app_t::instance()->log("previous missing!");
            return false;
        }

        while (relativePosition + viewWidth < block.previous->length) {
            relativePosition += viewWidth;
        }
        cursor->setPosition(block.previous, relativePosition);
        break;

    case cursor_t::Move::Down:
        if (wrap && block.length > viewWidth) {
            int p = cursor->_position.position + viewWidth;
            if (p < block.length) {
                cursor->setPosition(&block, p);
                break;
            }
        }

        if (!block.next) {
            // app_t::instance()->log("next missing!");
            return false;
        }

        while (relativePosition > viewWidth) {
            relativePosition -= viewWidth;
        }
        cursor->setPosition(block.next, relativePosition);
        break;

    case cursor_t::Move::StartOfLine:
        cursor->setPosition(cursor->block(), 0);
        break;

    case cursor_t::Move::EndOfLine:
        cursor->setPosition(cursor->block(), cursor->block()->length - 1);
        break;

    case cursor_t::Move::StartOfDocument: {
        struct block_t& front = cursor->document()->blocks.front();
        cursor->setPosition(&front, 0);
        break;
    }

    case cursor_t::Move::EndOfDocument: {
        struct block_t& end = cursor->document()->blocks.back();
        cursor->setPosition(&end, end.length);
        break;
    }

    case cursor_t::Move::WordLeft: {
        bool found = false;
        std::reverse(std::begin(search_results), std::end(search_results));
        for (auto i : search_results) {
            cursor->_position.position = i.begin;
            if (i.begin + 1 < relativePosition) {
                found = true;
                break;
            }
        }
        break;
    }

    case cursor_t::Move::WordRight: {
        bool found = false;
        for (auto i : search_results) {
            cursor->_position.position = i.begin;
            if (i.begin > relativePosition) {
                found = true;
                break;
            }
        }
        break;
    }

    case cursor_t::Move::PrevBlock:
        if (block.previous) {
            cursor->setPosition(block.previous, relativePosition);
        }
        break;

    case cursor_t::Move::NextBlock:
        if (block.next) {
            cursor->setPosition(block.next, relativePosition);
        }
        break;

    default:
        break;
    }

    if (!keepAnchor) {
        cursor->_anchor = cursor->_position;
    }

    if (count > 0) {
        cursorMovePosition(cursor, move, keepAnchor, count);
    }

    // app_t::instance()->log("new block %d position: %d", cursor->block()->lineNumber, cursor->relativePosition());
    return true;
}

void cursorSelectWord(struct cursor_t* cursor)
{
    struct block_t* block = cursor->block();
    size_t relativePosition = cursor->relativePosition();

    std::vector<search_result_t> search_results = search_t::instance()->findWords(block->text());

    for (auto i : search_results) {
        if (i.begin <= relativePosition && relativePosition < i.end) {
            cursor->setPosition(block, i.end - 1);
            cursor->setAnchor(block, i.begin);
            return;
        }
    }
}

int cursorInsertText(struct cursor_t* cursor, std::string t)
{
    struct block_t* block = cursor->block();
    if (!block->isValid()) {
        return 0;
    }

    cursor->document()->history()._addInsert(*cursor, t);

    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    std::string blockText = block->text();

    if (blockText.length() > 1) {
        int relativePosition = cursor->relativePosition();
        if (relativePosition < 0 || relativePosition >= blockText.length()) {
            relativePosition = blockText.length() - 1;
        }
        if (relativePosition < 0) {
            relativePosition = 0;
        }

        // overlapping cursors?
        if (relativePosition >= blockText.length()) {
            cursor->document()->clearCursors();
            return 0;
        }

        // app_t::instance()->log("insert at %d/%d text length: %d", cursor->position(), cursor->relativePosition(), t.length());
        blockText.insert(cursor->relativePosition(), t);
    } else {
        blockText += t;
    }

    block->setText(blockText);
    return blockText.length();
}

int cursorEraseText(struct cursor_t* cursor, int c)
{
    struct block_t* block = cursor->block();
    if (!block->isValid()) {
        return 0;
    }

    cursor->document()->history()._addDelete(*cursor, c);

    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    std::string blockText = block->text();

    // at end
    if (cursor->relativePosition() >= blockText.length()) {
        if (block->next) {
            blockText += block->next->text();
            cursor->document()->removeBlockAtLineNumber(block->next->lineNumber);
            cursor->document()->update(true);
        }
    } else {
        blockText.erase(cursor->relativePosition(), c);
    }

    block->setText(blockText);
    return c;
}

void cursorSplitBlock(struct cursor_t* cursor)
{
    struct block_t* block = cursor->block();
    if (!block->isValid()) {
        return;
    }

    cursor->document()->history()._addSplit(*cursor);

    if (!block->length) {
        block->setText("");
    }

    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    int relativePosition = cursor->relativePosition();
    std::string blockText = block->text();
    std::string block1 = blockText.substr(0, relativePosition);
    std::string block2 = blockText.substr(relativePosition, std::string::npos);
    block->setText(block1);

    cursor->document()->update(true);

    struct block_t& newBlock = cursor->document()->addBlockAtLineNumber(block->lineNumber + 1);
    newBlock.setText(block2);
    cursor->setPosition(&newBlock, 0);
    cursor->setAnchor(&newBlock, 0);

    struct cursor_edit_t& edit = cursor->document()->history().editBatch.back();
    edit.newBlockUid = newBlock.uid;
}

int cursorDeleteSelection(struct cursor_t* cursor)
{
    if (!cursor->hasSelection()) {
        return 0;
    }

    struct document_t* doc = cursor->document();
    struct cursor_t cur = cursor->selectionStartCursor();
    struct cursor_t curEnd = cursor->selectionEndCursor();

    // for selection spanning multiple blocks
    struct block_t* startBlock = cur.block();
    struct block_t* endBlock = curEnd.block();

    if (!startBlock->isValid()) {
        return 0;
    }
    if (!endBlock->isValid()) {
        return 0;
    }

    cursor->document()->history()._addDeleteSelection(cur, curEnd);

    if (startBlock != endBlock) {
        int count = 0;

        struct block_t* next = startBlock->next;

        std::string startText = startBlock->text() + " ";
        int startRel = cur.relativePosition() + 1;
        std::string newText = endBlock->text();
        int endRel = curEnd.relativePosition();
        endRel = endRel < newText.length() ? endRel : newText.length();
        endRel++;

        app_t::instance()->log("execute delete %d %d", startRel, endRel);

        if (startRel == 0) {
            startText = "";
        } else {
            startText = startText.substr(0, startRel );
        }

        newText = (newText + "  ").substr(endRel);

        if (startText.length()) {
            startText.pop_back(); // drop newline
        }
        cur.block()->setText(startText + newText);

        int linesToDelete = endBlock->lineNumber - startBlock->lineNumber;
        int lineNumber = startBlock->lineNumber + 1;
        //app_t::instance()->log("lines to delete %d %d", lineNumber, linesToDelete);;
        doc->removeBlockAtLineNumber(lineNumber, linesToDelete);
        // warning these deletes are not yet counted

        cursor->_position = cur._position;
        cursor->_anchor = cur._position;

        doc->update(true);
        return count;
    }

    int count = curEnd.relativePosition() - cur.relativePosition() + 1;

    std::string startText = startBlock->text();
    int startRel = cur.relativePosition();
    startText.erase(startRel, count);
    startBlock->setText(startText);

    // app_t::instance()->log("execute delete %d %d", cur.position(), count);
    cursor->_position = cur._position;
    cursor->_anchor = cursor->_position;

    cursor->document()->update(true);
    return count;
}

std::string cursor_t::selectedText()
{
    if (!hasSelection()) {
        return "";
    }

    struct cursor_t start = selectionStartCursor();
    struct cursor_t end = selectionEndCursor();

    std::vector<struct block_t*> blocks = selectedBlocks();
    std::string res = "";
    for (auto b : blocks) {
        if (res != "") {
            res += "\n";
        }
        std::string t = b->text();
        if (b == start.block()) {
            t = t.c_str() + start.relativePosition();
        }
        if (b == end.block()) {
            int trunc = end.block()->length - end.relativePosition() - 2;
            if (trunc > 0) {
                t = t.substr(0, t.length() - trunc);
                // app_t::instance()->log(">trunc %d", trunc);
            }
        }
        res += t;
    }

    return res;
}

bool cursorFindWord(struct cursor_t* cursor, std::string t, int direction)
{
    bool firstCursor = true;
    struct cursor_t cur = *cursor;

    size_t prevPos = cur.position();
    for (;;) {
        struct block_t& block = *cur.block();
        if (!block.isValid()) {
            return false;
        }
        size_t relativePosition = cur.relativePosition();

        std::string text = block.text();
        std::vector<search_result_t> res = search_t::instance()->find(text, t);
        if (res.size()) {
            search_result_t found;
            if (firstCursor) {
                for (auto& r : res) {
                    if (r.end <= relativePosition || r.begin <= relativePosition) {
                        continue;
                    }
                    found = r;
                    break;
                }
            } else {
                found = res[0];
            }

            if (found.isValid()) {
                cursor->setPosition(cur.block(), found.end - 1);
                cursor->setAnchor(cur.block(), found.begin);
                // app_t::instance()->log("found word! %d", cursor->position());
                return true;
            }
        }

        // app_t::instance()->log("find next");
        if (direction == 1) {
            if (!cursorMovePosition(&cur, cursor_t::Move::PrevBlock)) {
                return false;
            }
        } else {
            if (!cursorMovePosition(&cur, cursor_t::Move::NextBlock)) {
                return false;
            }
        }

        cursorMovePosition(&cur, cursor_t::Move::StartOfLine);
        firstCursor = false;
        if (cur.position() == prevPos) {
            // app_t::instance()->log("this shouldn't have happened: %s", text.c_str());
            break;
        }
        prevPos = cur.position();
    }

    return false;
}

static size_t countIndentSize(std::string s)
{
    for (int i = 0; i < s.length(); i++) {
        if (s[i] != ' ') {
            return i;
        }
    }
    return 0;
}

int countToTabStop(struct cursor_t* cursor)
{
    int tab_size = 4;
    bool tab_to_spaces = true;
    if (!tab_to_spaces) {
        // cursor.insertText("\t");
        return 1;
    }

    int ts = tab_size;
    struct cursor_t cs = *cursor;
    cursorMovePosition(&cs, cursor_t::Move::StartOfLine, true);
    size_t ws = countIndentSize(cs.selectedText() + "?");
    if (ws != 0 && ws == cursor->position() - cursor->block()->position) {
        // magic! (align to tab)
        ts = (((ws / ts) + 1) * ts) - ws;
    }

    return ts;
}

static int _cursorIndent(struct cursor_t* cursor)
{
    std::string text = cursor->block()->text();
    int tab_size = 4;
    int currentIndent = countIndentSize(text + "?");
    int newIndent = ((currentIndent / tab_size) + 1) * tab_size;
    int inserted = newIndent - currentIndent;
    struct cursor_t cur = *cursor;

    std::string tab = "";
    for (int i = 0; i < inserted; i++) {
        tab += " ";
    }

    cur._position.position = 0;
    cursorInsertText(&cur, tab);
    cursor->_position.position += inserted;
    cursor->_anchor.position += inserted;
    return inserted;
}

int cursorIndent(struct cursor_t* cursor)
{
    std::vector<struct block_t*> blocks = cursor->selectedBlocks();
    if (blocks.size() > 1) {

        struct cursor_t posCur = cursor->selectionStartCursor();
        struct cursor_t anchorCur = cursor->selectionEndCursor();

        for (auto b : blocks) {
            struct cursor_t cur = *cursor;
            cur.setPosition(b, 0);

            bool updatePos = b->uid == posCur.block()->uid;
            bool updateAnchor = b->uid == anchorCur.block()->uid;
            int count = _cursorIndent(&cur);

            if (updatePos) {
                app_t::instance()->log("indent update pos %d", cursor->position());
                cursor->_position.position += count;
            }
            if (updateAnchor) {
                app_t::instance()->log("indent update anchor %d", cursor->anchorPosition());
                cursor->_anchor.position += count;
            }
        }

        return 1;
    }

    int count = _cursorIndent(cursor);
    return count;
}

int _cursorUnindent(struct cursor_t* cursor)
{
    std::string text = cursor->block()->text();
    int tab_size = 4;
    int currentIndent = countIndentSize(text + "?");
    int newIndent = ((currentIndent / tab_size) - 1) * tab_size;
    int deleted = currentIndent - newIndent;
    struct cursor_t cur = *cursor;

    if (newIndent < 0) {
        deleted = deleted % tab_size;
    }

    if (deleted > 0) {
        cur._position.position = 0;
        cursorEraseText(&cur, deleted);
        cursor->_position.position -= deleted;
        cursor->_anchor.position -= deleted;
    }

    return deleted;
}

int cursorUnindent(struct cursor_t* cursor)
{
    std::vector<struct block_t*> blocks = cursor->selectedBlocks();
    if (blocks.size() > 1) {

        struct cursor_t posCur = cursor->selectionStartCursor();
        struct cursor_t anchorCur = cursor->selectionEndCursor();

        for (auto b : blocks) {
            struct cursor_t cur = *cursor;
            cur.setPosition(b, 0);

            bool updatePos = b->uid == posCur.block()->uid;
            bool updateAnchor = b->uid == anchorCur.block()->uid;
            int count = _cursorUnindent(&cur);

            if (updatePos) {
                app_t::instance()->log("unindent update pos %d", cursor->position());
                cursor->_position.position -= count;
            }
            if (updateAnchor) {
                app_t::instance()->log("unindent update anchor %d", cursor->anchorPosition());
                cursor->_anchor.position -= count;
            }
        }

        return 1;
    }

    int count = _cursorUnindent(cursor);
    return count;
    ;
}
