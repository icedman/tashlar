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
    , _document(0)
    , _block(0)
    , _position(0)
    , _anchorPosition(0)
    , _preferredRelativePosition(0)
{}

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
 
    if (!_block) {
        // app_t::instance()->log("initial query");
        _block = &_document->block(*this);
    }

    if (_position < _block->position || _position >= _block->position + _block->length) {
        // app_t::instance()->log("reposition query %d %d", _position, _block->position);
        _block = &_document->block(*this);
    }
    _relativePosition =  _position - _block->position;
}

bool cursor_t::isNull()
{
    return _document == 0 || _block == 0 || !_block->isValid();
}

bool cursor_t::hasSelection()
{
    return _anchorPosition != _position;
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
        start =_position;
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
        start =_position;
        end = _anchorPosition;
    }

    _position = end;
    _anchorPosition = start;
    return _anchorPosition;; 
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
    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    std::string blockText = block->text();
    blockText.insert(cursor->relativePosition(), t);
    block->setText(blockText);

    // app_t::instance()->log("insert at %d %s", cursor->relativePosition(), blockText.c_str());
    return blockText.length();
}

int cursorEraseText(struct cursor_t* cursor, int c)
{
    struct block_t* block = cursor->block();
    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    std::string blockText = block->text();

    // at end
    if (cursor->relativePosition() == blockText.length()) {
        if (block->next) {
            blockText += block->next->text();
            // todo add delete next line
            std::vector<struct block_t>::iterator it = findBlock(cursor->document()->blocks, *block->next);
            if (it != cursor->document()->blocks.end()) {
                cursor->document()->blocks.erase(it);
            }
        }
    } else {
        blockText.erase(cursor->relativePosition(), c);
    }

    block->setText(blockText);
    return 1;
}

void cursorSplitBlock(struct cursor_t* cursor)
{
    struct block_t* block = cursor->block();
    struct blockdata_t* data = block->data.get();
    if (data && data->folded && data->foldable) {
        app_t::instance()->currentEditor->toggleFold(block->lineNumber);
    }

    std::string blockText = block->text();
    std::string block1 = blockText.substr(0, cursor->relativePosition());
    std::string block2 = blockText.substr(cursor->relativePosition(), std::string::npos);
    block->setText(block2);

    struct block_t b;
    b.document = cursor->document();
    b.setText(block1);
    
    std::vector<struct block_t>::iterator it = findBlock(cursor->document()->blocks, *block);
    if (it != cursor->document()->blocks.end()) {
        cursor->document()->blocks.emplace(it, b);
    }

    cursor->document()->update();
}

// todo .. this needs history replay
int cursorDeleteSelection(struct cursor_t* cursor)
{
    if (!cursor->hasSelection()) {
        return 0;
    }

    size_t start;
    size_t end;
    if (cursor->position() > cursor->anchorPosition()) {
        start = cursor->anchorPosition();
        end = cursor->position();
    } else {
        start = cursor->position();
        end = cursor->anchorPosition();
    }

    struct document_t* doc = cursor->document();
    struct cursor_t cur = *cursor;
    struct cursor_t curEnd = *cursor;
    cur._position = start;
    curEnd._position = end;
    cur.update();
    curEnd.update();
    
    // for selection spanning multiple blocks
    struct block_t& startBlock = *cur.block();
    struct block_t& endBlock = *curEnd.block();
    if (startBlock.position != endBlock.position) {
        int count = 0;

        struct block_t* next = startBlock.next;
        struct block_t* firstNext = next;

        int d = curEnd.position() - endBlock.position;
        curEnd._position = endBlock.position;
        curEnd.update();
        cursorEraseText(&curEnd, d);
        count += d;

        app_t::instance()->log("end line %d", endBlock.lineNumber);

        d = startBlock.position + startBlock.length - cur.position();
        if (d > 1) {
            cur.update();
            app_t::instance()->log("start line %d %d", startBlock.lineNumber, d);
            cursorEraseText(&cur, d);
            count += d;
        }

        cursor->document()->update();

        int linesToDelete = 0;
        while (next && next != &endBlock) {
            linesToDelete++;
            count += next->length;
            next = next->next;
        }

        // delete blocks in-between
        if (firstNext && linesToDelete-- > 0) {
            std::vector<struct block_t>::iterator it = findBlock(doc->blocks, *firstNext);
            if (it != doc->blocks.end()) {
                doc->blocks.erase(it, it + linesToDelete + 1);
                doc->update();
            }
        }

        // merge two block
        doc->update();
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

    cursor->_position = cur.position();
    cursor->_anchorPosition = cur.position();
    cursor->update();
    return count;
}

std::string cursor_t::selectedText()
{
    if (!hasSelection()) {
        return "";
    }

    size_t start;
    size_t end;
    if (position() > anchorPosition()) {
        start = anchorPosition();
        end = position();
    } else {
        start = position();
        end = anchorPosition();
    }

    struct cursor_t cursor = *this;
    cursor._position = start;
    cursor.update();
    struct block_t* block = cursor.block();
    
    std::string res = "";
    std::string text = block->text();

    int count = end - start + 1;

    for (int i = 0; i < count; i++) {
        size_t idx = start + i;
        int blockIdx = idx - block->position;

        char c = text[blockIdx];
        if (c == 0) {

            if (!block->next) {
                return res;
            }
            block = block->next;
            if (!block || !block->isValid()) {
                return res;
            }
            text = block->text();

            res += "\n";
            continue;
        }
        res += c;
    }

    return res;
}

bool cursorFindWord(struct cursor_t* cursor, std::string t)
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

        if (!cursorMovePosition(&cur, cursor_t::Move::NextBlock)) {
            return false;
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
    /*
    std::string text = cursor->block->text();
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
    cursor->block->setText(newText);
    // app_t::instance()->log("indent %d  %d %s\n", inserted, cursor->block->lineNumber, cursor->block->text().c_str());
    return inserted;
    */
    return 0;
}

int cursorIndent(struct cursor_t* cursor)
{
    /*
    if (cursor->hasSelection()) {
        std::vector<struct block_t*> blocks = cursor->selectedBlocks();
        int count = 0;
        int offset = 0;
        // app_t::instance()->log("blocks: %d", blocks.size());
        for (auto b : blocks) {
            struct cursor_t cur = *cursor;
            cur.block = b;
            count += _cursorIndent(&cur);
        }
        return count;
    }
    return _cursorIndent(cursor);
    */
    return 0;
}

int _cursorUnindent(struct cursor_t* cursor)
{
    /*
    std::string text = cursor->block->text();
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
        cursor->block->setText(newText);
    }
    return deleted;
    */
    return 0;
}

int cursorUnindent(struct cursor_t* cursor)
{
    /*
    if (cursor->hasSelection()) {
        std::vector<struct block_t*> blocks = cursor->selectedBlocks();
        int count = 0;
        int offset = 0;
        // app_t::instance()->log("blocks: %d", blocks.size());
        for (auto b : blocks) {
            struct cursor_t cur = *cursor;
            cur.block = b;
            count += _cursorUnindent(&cur);
        }
        return count;
    }
    return _cursorUnindent(cursor);
    */
    return 0;
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
