/*
 todo: decide fixed rules from cursor manipulation
 todo: move history at cursor level instead of at command level<<?
 
1. moving, navigation ... updates the absolute positions .. and so too the relative positions
2. editing a block -- use the relative position 
3. position - prevails over anchor when determining block
*/

#include "cursor.h"
#include "app.h"
#include "document.h"
#include "search.h"
#include "util.h"

#include <algorithm>

cursor_t::cursor_t()
    : uid(0)
    , _docUpdateUid(0)
    , _document(0)
    , _block(0)
    , _position(0)
    , _anchorPosition(0)
    , _relativePosition(0)
    , _preferredRelativePosition(0)
{
}

struct document_t* cursor_t::document()
{
    return _document;
}

struct block_t* cursor_t::block()
{
    update();
    return _block;
}

void cursor_t::update()
{
    if (!_document) {
        return;
    }

    if (!_block || _docUpdateUid != _document->blockUid) {
        // app_t::instance()->log("initial query");
        _block = &_document->block(*this);
    }

    if (_position < _block->position || _position >= _block->position + _block->length) {
        // app_t::instance()->log("reposition query %d %d", _position, _block->position);
        _block = &_document->block(*this);
    }
    _relativePosition = _position - _block->position;
    _docUpdateUid = _document->blockUid;
}

bool cursor_t::isNull()
{
    return _document == 0 || _block == 0 || !_block->isValid();
}

bool cursor_t::hasSelection()
{
    return _anchorPosition != _position;
}

struct cursor_t cursor_t::selectionStartCursor()
{
    size_t start;
    size_t end;

    if (_position > _anchorPosition) {
        start = _anchorPosition;
        end = _position;
    } else {
        start = _position;
        end = _anchorPosition;
    }

    struct cursor_t cur = *this;
    cursorSetPosition(&cur, start);
    return cur;
}

struct cursor_t cursor_t::selectionEndCursor()
{
    size_t start;
    size_t end;

    if (_position > _anchorPosition) {
        start = _anchorPosition;
        end = _position;
    } else {
        start = _position;
        end = _anchorPosition;
    }

    struct cursor_t curEnd = *this;
    cursorSetPosition(&curEnd, end);
    return curEnd;
}

std::vector<struct block_t*> cursor_t::selectedBlocks()
{
    std::vector<struct block_t*> blocks;

    size_t start;
    size_t end;

    if (_position > _anchorPosition) {
        start = _anchorPosition;
        end = _position;
    } else {
        start = _position;
        end = _anchorPosition;
    }

    _position = end;
    _anchorPosition = start;

    struct cursor_t cur = *this;
    struct cursor_t curEnd = *this;
    cursorSetPosition(&cur, start);
    cursorSetPosition(&curEnd, end);

    struct block_t* startBlock = cur.block();
    struct block_t* endBlock = curEnd.block();

    blocks.push_back(startBlock);
    while (startBlock != endBlock) {
        startBlock = startBlock->next;
        blocks.push_back(startBlock);
    }

    return blocks;
}

bool cursor_t::isMultiBlockSelection()
{
    if (hasSelection()) {
        struct cursor_t cur2 = *this;
        cur2._position = cur2.anchorPosition();
        cur2.update();
        if (block() != cur2.block()) {
            return true;
        }
    }
    return false;
}

void cursor_t::clearSelection()
{
    _anchorPosition = _position;
}

size_t cursor_t::selectionStart()
{
    size_t start;
    size_t end;

    if (_position > _anchorPosition) {
        start = _anchorPosition;
        end = _position;
    } else {
        start = _position;
        end = _anchorPosition;
    }

    _position = end;
    _anchorPosition = start;
    return _anchorPosition;
    ;
}

size_t cursor_t::selectionEnd()
{
    selectionStart();
    return _position;
}

size_t cursor_t::position()
{
    return _position;
}

size_t cursor_t::anchorPosition()
{
    return _anchorPosition;
}

size_t cursor_t::relativePosition()
{
    update(); // always try update
    return _relativePosition;
}

size_t cursor_t::preferredRelativePosition()
{
    return _preferredRelativePosition;
}

void cursorSetPosition(struct cursor_t* cursor, size_t position)
{
    cursor->_position = position;
    cursor->_anchorPosition = position;
    cursor->relativePosition(); // recompute relative
    // app_t::instance()->log("set cursor>%d", position);
}

void cursorSetAnchorPosition(struct cursor_t* cursor, size_t position)
{
    cursor->_anchorPosition = position;
    // no need to recompute
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

    // app_t::instance()->log("move %d", cursor->position());

    count--;
    size_t relativePosition = cursor->_position - block.position;

    if ((move == cursor_t::Move::Up || move == cursor_t::Move::Down) && cursor->preferredRelativePosition() != 0) {
        relativePosition = cursor->preferredRelativePosition();
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
        if (cursor->preferredRelativePosition() == 0) {
            cursor->_preferredRelativePosition = relativePosition;
        }
    }

    if (extractWords) {
        search_results = search_t::instance()->findWords(block.text());
    }

    switch (move) {
    case cursor_t::Move::StartOfLine:
        cursor->_position = block.position;
        break;
    case cursor_t::Move::EndOfLine:
        cursor->_position = block.position + (block.length - 1);
        break;

    case cursor_t::Move::StartOfDocument:
        cursor->_position = 0;
        break;
    case cursor_t::Move::EndOfDocument: {
        struct block_t& end = cursor->document()->blocks.back();
        cursor->_position = end.position + end.length - 1;
        break;
    }

    case cursor_t::Move::WordLeft: {
        bool found = false;
        std::reverse(std::begin(search_results), std::end(search_results));
        for (auto i : search_results) {
            cursor->_position = block.position + i.begin;
            if (i.begin + 1 < relativePosition) {
                found = true;
                break;
            }
        }
        if (!found) {
            cursor->_position = block.position;
        }
        break;
    }

    case cursor_t::Move::WordRight: {
        bool found = false;
        for (auto i : search_results) {
            cursor->_position = block.position + i.begin;
            if (i.begin > relativePosition) {
                found = true;
                break;
            }
        }

        if (!found) {
            cursor->_position = block.position + (block.length - 1);
        }
        break;
    }

    case cursor_t::Move::Up:
        if (wrap && block.length > viewWidth) {
            int p = cursor->_position - viewWidth;
            if (p >= block.position && p > 0) {
                cursor->_position = p;
                break;
            }
        }
        if (!block.previous) {
            return false;
        }
        if (relativePosition > block.previous->length - 1) {
            relativePosition = block.previous->length - 1;
        }
        cursor->_position = block.previous->position + relativePosition;
        cursor->_block = block.previous;
        break;

    case cursor_t::Move::Down:
        if (wrap && block.length > viewWidth) {
            int p = cursor->_position + viewWidth;
            if (p < block.position + block.length) {
                cursor->_position = p;
                break;
            }
        }

        if (!block.next) {
            return false;
        }
        if (relativePosition > block.next->length - 1) {
            relativePosition = block.next->length - 1;
        }
        cursor->_position = block.next->position + relativePosition;
        cursor->_block = block.next;
        break;

    case cursor_t::Move::Left:
        if (cursor->_position > 0) {
            cursor->_position--;
        }
        break;

    case cursor_t::Move::Right:
        cursor->_position++;
        if (cursor->_position >= block.position + block.length && !block.next) {
            cursor->_position--;
        }
        break;

    case cursor_t::Move::PrevBlock:
        if (cursor->block() && cursor->block()->previous) {
            cursor->_position = cursor->block()->previous->position;
            cursor->_block = cursor->block()->previous;
        }
        break;

    case cursor_t::Move::NextBlock:
        if (cursor->block() && cursor->block()->next) {
            cursor->_position = cursor->block()->next->position;
            cursor->_block = cursor->block()->next;
        }
        break;

    default:
        break;
    }

    if (!keepAnchor) {
        cursor->_anchorPosition = cursor->position();
    }

    if (count > 0) {
        return cursorMovePosition(cursor, move, keepAnchor, count);
    }

    cursor->update();
    // app_t::instance()->log("block %d relative %d", cursor->block()->lineNumber, cursor->relativePosition());

    return !cursor->isNull();
}

void cursorSelectWord(struct cursor_t* cursor)
{
    struct block_t* block = cursor->block();
    size_t relativePosition = cursor->relativePosition();

    std::vector<search_result_t> search_results = search_t::instance()->findWords(block->text());

    for (auto i : search_results) {
        if (i.begin <= relativePosition && relativePosition < i.end) {
            cursor->_anchorPosition = block->position + i.begin;
            cursor->_position = block->position + i.end - 1;
            cursor->update();
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

        // app_t::instance()->log("insert at %d/%d text length: %d", cursor->position(), cursor->relativePosition(), t.length());

        blockText.insert(cursor->relativePosition(), t);
        block->setText(blockText);
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

    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    std::string blockText = block->text();

    // at end
    if (cursor->relativePosition() == blockText.length()) {
        if (block->next) {
            blockText += block->next->text();
            cursor->document()->removeBlockAtLineNumber(block->next->lineNumber);
            cursor->document()->update(true);
        }
    } else {
        // hack for multi-cursor overlap
        if (c + cursor->relativePosition() > blockText.length() && cursor->document()->cursors.size() > 1) {
            return 0;
        }
        blockText.erase(cursor->relativePosition(), c);
    }

    block->setText(blockText);
    return 1;
}

void cursorSplitBlock(struct cursor_t* cursor)
{
    struct block_t* block = cursor->block();
    if (!block->isValid()) {
        return;
    }

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
    block->setText(block2);

    struct block_t& newBlock = cursor->document()->addBlockAtLineNumber(block->lineNumber);
    newBlock.setText(block1);
    cursor->document()->update(true);
}

// todo .. this needs history replay for multiple line deletes
int cursorDeleteSelection(struct cursor_t* cursor)
{
    if (!cursor->hasSelection()) {
        return 0;
    }

    size_t start = cursor->selectionStart();
    size_t end = cursor->selectionEnd();

    struct document_t* doc = cursor->document();
    struct cursor_t cur = *cursor;
    struct cursor_t curEnd = *cursor;
    cur._position = start;
    curEnd._position = end;
    cur.update();
    curEnd.update();

    // for selection spanning multiple blocks
    struct block_t* startBlock = cur.block();
    struct block_t* endBlock = curEnd.block();

    if (!startBlock->isValid()) {
        return 0;
    }
    if (!endBlock->isValid()) {
        return 0;
    }

    if (startBlock->position != endBlock->position) {
        int count = 0;

        struct block_t* next = startBlock->next;

        int d = curEnd.position() - endBlock->position;
        curEnd._position = endBlock->position;
        curEnd.update();
        cursorEraseText(&curEnd, d);
        count += d;

        app_t::instance()->log("end line %d", endBlock->lineNumber);

        d = startBlock->position + startBlock->length - cur.position();
        if (d > 1) {
            cur.update();
            app_t::instance()->log("start line %d %d", startBlock->lineNumber, d);
            cursorEraseText(&cur, d);
            count += d;
        }

        int linesToDelete = endBlock->lineNumber - startBlock->lineNumber;
        // delete blocks in-between
        app_t::instance()->log("lines to delete %d", linesToDelete);

        int lineNumber = startBlock->lineNumber + 1;
        // for (size_t i = 0; i < linesToDelete - 1; i++) {
        doc->removeBlockAtLineNumber(lineNumber, linesToDelete - 1);
        // }

        // merge two block
        doc->update(true);
        cursorEraseText(&cur, 1);
        count++;

        cursor->_position = cur.position();
        cursor->_anchorPosition = cur.position();
        return count;
    }

    int count = end - start + 1;
    for (int i = 0; i < count; i++) {
        cursorEraseText(&cur, 1);
    }

    cursor->_position = start;
    cursor->_anchorPosition = end;
    cursor->update();
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
        if (b == start._block) {
            t = t.c_str() + start._relativePosition;
        }
        if (b == end._block) {
            int trunc = end._block->length - end._relativePosition - 2;
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
                cursor->_anchorPosition = block.position + found.begin;
                cursor->_position = block.position + found.end - 1;
                cursor->update();
                return true;
            }
        }

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
            app_t::instance()->log("this shouldn't have happened: %s", text.c_str());
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

    std::string newText = "";
    for (int i = 0; i < inserted; i++) {
        newText += " ";
    }
    newText += text;
    cursor->block()->setText(newText);
    // app_t::instance()->log("indent %d  %d %s\n", inserted, cursor->block->lineNumber, cursor->block->text().c_str());
    return inserted;
}

int cursorIndent(struct cursor_t* cursor)
{
    std::vector<struct block_t*> blocks = cursor->selectedBlocks();
    if (blocks.size() > 1) {

        struct cursor_t posCur = *cursor;
        posCur._position = cursor->_position;
        posCur.update();

        struct cursor_t anchorCur = *cursor;
        anchorCur._position = cursor->_anchorPosition;
        anchorCur.update();

        for (auto b : blocks) {
            struct cursor_t cur = *cursor;
            cur._position = b->position;
            cur._block = b;

            bool updatePos = b->uid == posCur._block->uid;
            bool updateAnchor = b->uid == anchorCur._block->uid;
            ;
            int count = _cursorIndent(&cur);
            cursor->document()->update();

            app_t::instance()->log("indent %d %d", updatePos, updateAnchor);
            if (updatePos) {
                cursor->_position = posCur._block->position + posCur._relativePosition + count;
            }
            if (updateAnchor) {
                cursor->_anchorPosition = anchorCur._block->position + posCur._relativePosition + count;
            }
        }

        cursor->update();
        return 1;
    }

    int count = _cursorIndent(cursor);
    cursor->_position += count;
    cursor->_anchorPosition += count;
    cursor->update();
    return count;
}

int _cursorUnindent(struct cursor_t* cursor)
{
    std::string text = cursor->block()->text();
    int tab_size = 4;
    int currentIndent = countIndentSize(text + "?");
    int newIndent = ((currentIndent / tab_size) - 1) * tab_size;
    struct cursor_t cur = *cursor;
    int deleted = currentIndent - newIndent;
    // app_t::instance()->log("<<%d %d %d", currentIndent, newIndent, deleted);
    if (newIndent < 0) {
        deleted = deleted % tab_size;
    }
    if (deleted > 0) {

        std::string newText = (text.c_str() + deleted);
        cursor->block()->setText(newText);
    }
    return deleted;
}

int cursorUnindent(struct cursor_t* cursor)
{
    std::vector<struct block_t*> blocks = cursor->selectedBlocks();
    if (blocks.size() > 1) {

        struct cursor_t posCur = *cursor;
        posCur._position = cursor->_position;
        posCur.update();

        struct cursor_t anchorCur = *cursor;
        anchorCur._position = cursor->_anchorPosition;
        anchorCur.update();

        for (auto b : blocks) {
            struct cursor_t cur = *cursor;
            cur._position = b->position;
            cur._block = b;

            bool updatePos = b->uid == posCur._block->uid;
            bool updateAnchor = b->uid == anchorCur._block->uid;
            ;
            int count = _cursorUnindent(&cur);
            cursor->document()->update();

            app_t::instance()->log("unindent %d %d", updatePos, updateAnchor);
            if (updatePos) {
                cursor->_position = posCur._block->position + posCur._relativePosition - count;
            }
            if (updateAnchor) {
                cursor->_anchorPosition = anchorCur._block->position + posCur._relativePosition - count;
            }
        }

        cursor->update();
        return 1;
    }

    int count = _cursorUnindent(cursor);
    cursor->_position -= count;
    cursor->_anchorPosition -= count;
    cursor->update();
    return count;
}

int autoIndent(struct cursor_t cursor)
{
    /*
    editor_settings_ptr settings = MainWindow::instance()->editor_settings;
    int white_spaces = 0;

    HighlightBlockData* blockData;
    bool beginsWithCloseBracket = false;

    QTextCursor cs(cursor);
    QTextBlock block = cursor.block();

    if (block.isValid()) {
        blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
        if (blockData->brackets.size()) {
            beginsWithCloseBracket = !blockData->brackets[0].open;
        }
    }

    while (block.isValid()) {

        // qDebug() << block.text();

        cs.setPosition(block.position());
        cs.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
        QString blockText = block.text();
        size_t ws = countIndentSize(blockText + "?");
        if (white_spaces < ws) {
            white_spaces = ws;
        }
        if (blockText.length() && ws != 0) {
            break;
        }
        // is line whitespace
        if (ws == 0) {
            block = block.previous();
            continue;
        }

        break;
    }

    if (!block.isValid()) {
        return;
    }

    blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (blockData && blockData->brackets.size()) {
        auto b = blockData->brackets.back();
        if (b.open) {
            if (settings->tab_to_spaces) {
                white_spaces += settings->tab_size;
            } else {
                white_spaces++;
            }
        }
    }

    if (beginsWithCloseBracket) {
        if (settings->tab_to_spaces) {
            white_spaces -= settings->tab_size;
        } else {
            white_spaces--;
        }
    }

    // qDebug() << white_spaces;

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfLine);
    for (int i = 0; i < white_spaces; i++) {
        if (settings->tab_to_spaces) {
            cursor.insertText(" ");
        } else {
            cursor.insertText("\t");
            i += (settings->tab_size - 1);
        }
    }
    cursor.endEditBlock();
    */
}
