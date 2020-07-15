#include "cursor.h"
#include "app.h"
#include "document.h"
#include "search.h"
#include "util.h"

#include <algorithm>

void cursor_t::update()
{
    block = &document->block(*this);
}

bool cursor_t::isNull()
{
    return document == 0 || block == 0 || !block->isValid();
}

bool cursorMovePosition(struct cursor_t* cursor, enum cursor_t::Move move, bool keepAnchor, int count)
{
    struct block_t& block = cursor->document->block(*cursor);
    if (!block.isValid()) {
        return false;
    }

    count--;
    size_t relativePosition = cursor->position - block.position;

    if ((move == cursor_t::Move::Up || move == cursor_t::Move::Down) && cursor->preferredRelativePosition != 0) {
        relativePosition = cursor->preferredRelativePosition;
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
        cursor->preferredRelativePosition = 0;
    } else {
        if (cursor->preferredRelativePosition == 0) {
            cursor->preferredRelativePosition = relativePosition;
        }
    }

    if (extractWords) {
        search_results = search_t::instance()->findWords(block.text());
    }

    switch (move) {
    case cursor_t::Move::StartOfLine:
        cursor->position = block.position;
        break;
    case cursor_t::Move::EndOfLine:
        cursor->position = block.position + (block.length - 1);
        break;

    case cursor_t::Move::StartOfDocument:
        cursor->position = 0;
        break;
    case cursor_t::Move::EndOfDocument: {
        struct block_t& end = cursor->document->blocks.back();
        cursor->position = end.position + end.length - 1;
        break;
    }

    case cursor_t::Move::WordLeft: {
        bool found = false;
        std::reverse(std::begin(search_results), std::end(search_results));
        for (auto i : search_results) {
            cursor->position = block.position + i.begin;
            if (i.begin + 1 < relativePosition) {
                found = true;
                break;
            }
        }
        if (!found) {
            cursor->position = block.position;
        }
        break;
    }

    case cursor_t::Move::WordRight: {
        bool found = false;
        for (auto i : search_results) {
            cursor->position = block.position + i.begin;
            if (i.begin > relativePosition) {
                found = true;
                break;
            }
        }

        if (!found) {
            cursor->position = block.position + (block.length - 1);
        }
        break;
    }

    case cursor_t::Move::Up:
        if (!block.previous) {
            return false;
        }
        if (relativePosition > block.previous->length - 1) {
            relativePosition = block.previous->length - 1;
        }
        cursor->position = block.previous->position + relativePosition;
        break;
    case cursor_t::Move::Down:
        if (!block.next) {
            return false;
        }
        if (relativePosition > block.next->length - 1) {
            relativePosition = block.next->length - 1;
        }
        cursor->position = block.next->position + relativePosition;
        break;
    case cursor_t::Move::Left:
        if (cursor->position > 0) {
            cursor->position--;
        }
        break;
    case cursor_t::Move::Right:
        cursor->position++;
        if (cursor->position >= block.position + block.length && !block.next) {
            cursor->position--;
        }
        break;
    }

    if (!keepAnchor) {
        cursor->anchorPosition = cursor->position;
    }

    if (count > 0) {
        return cursorMovePosition(cursor, move, keepAnchor, count);
    }

    cursor->update();
    return !cursor->isNull();
}

void cursorSelectWord(struct cursor_t* cursor)
{
    struct block_t& block = cursor->document->block(*cursor);
    if (!block.isValid()) {
        return;
    }

    size_t relativePosition = cursor->position - block.position;

    std::vector<search_result_t> search_results = search_t::instance()->findWords(block.text());
    for (auto i : search_results) {
        if (i.begin <= relativePosition && relativePosition < i.end) {
            cursor->anchorPosition = block.position + i.begin;
            cursor->position = block.position + i.end - 1;
            break;
        }
    }
}

int cursorInsertText(struct cursor_t* cursor, std::string t)
{
    struct block_t& block = cursor->document->block(*cursor);
    if (!block.isValid()) {
        return 0;
    }

    std::string blockText = block.text();
    blockText.insert(cursor->position - block.position, t);
    block.setText(blockText);

    return blockText.length();
}

void cursorEraseText(struct cursor_t* cursor, int c)
{
    struct block_t& block = cursor->document->block(*cursor);
    if (!block.isValid()) {
        return;
    }

    std::string blockText = block.text();

    // at end
    if (cursor->position - block.position == blockText.length()) {
        if (block.next) {
            blockText += block.next->text();
            std::vector<struct block_t>::iterator it = findBlock(cursor->document->blocks, *block.next);
            if (it != cursor->document->blocks.end()) {
                cursor->document->blocks.erase(it);
            }
        }
    } else {
        blockText.erase(cursor->position - block.position, c);
    }

    block.setText(blockText);
}

void cursorSplitBlock(struct cursor_t* cursor)
{
    struct block_t& block = cursor->document->block(*cursor);
    if (!block.isValid()) {
        return;
    }

    std::string blockText = block.text();
    std::string block1 = blockText.substr(0, cursor->position - block.position);
    std::string block2 = blockText.substr(cursor->position - block.position, std::string::npos);
    block.setText(block2);

    struct block_t b;
    b.document = cursor->document;
    b.position = block.position + block.length;
    b.setText(block1);

    std::vector<struct block_t>::iterator it = findBlock(cursor->document->blocks, block);
    if (it != cursor->document->blocks.end()) {
        cursor->document->blocks.emplace(it, b);
    }

    cursor->document->update();
}

// todo .. broken for selections spanning several lines, way too slow
int cursorDeleteSelection(struct cursor_t* cursor)
{
    if (!cursor->hasSelection()) {
        return 0;
    }

    size_t start;
    size_t end;
    if (cursor->position > cursor->anchorPosition) {
        start = cursor->anchorPosition;
        end = cursor->position;
    } else {
        start = cursor->position;
        end = cursor->anchorPosition;
    }

    struct cursor_t cur = *cursor;
    struct cursor_t curEnd = *cursor;
    cur.position = start;
    curEnd.position = end;

    // for selection spanning multiple blocks
    struct block_t& startBlock = cursor->document->block(cur);
    struct block_t& endBlock = cursor->document->block(curEnd);
    if (startBlock.position != endBlock.position) {
        int count = 0;

        int d = curEnd.position - endBlock.position;
        curEnd.position = endBlock.position;
        cursorEraseText(&curEnd, d);
        count += d;

        d = startBlock.position + startBlock.length - cur.position;
        cursorEraseText(&cur, d);
        count += d;

        cursor->document->update();

        struct block_t* next = startBlock.next;
        int linesToDelete = 0;
        while (next && next != &endBlock) {
            linesToDelete++;
            count += next->length;
            next = next->next;
        }

        // delete blocks in-between
        next = startBlock.next;
        if (next && linesToDelete-- > 0) {
            std::vector<struct block_t>::iterator it = findBlock(cursor->document->blocks, *next);
            if (it != cursor->document->blocks.end()) {
                cursor->document->blocks.erase(it, it + linesToDelete + 1);
                cursor->document->update();
            }
        }

        // merge two block
        cursor->document->update();
        cursorEraseText(&cur, 1);
        count++;

        cursor->position = cur.position;
        cursor->anchorPosition = cur.position;
        return count;
    }

    int count = end - start + 1;
    for (int i = 0; i < count; i++) {
        cursorEraseText(&cur, 1);
    }

    cursor->position = cur.position;
    cursor->anchorPosition = cur.position;

    return count;
}

std::string cursor_t::selectedText()
{
    if (!hasSelection()) {
        return "";
    }

    size_t start;
    size_t end;
    if (position > anchorPosition) {
        start = anchorPosition;
        end = position;
    } else {
        start = position;
        end = anchorPosition;
    }

    struct cursor_t cursor = *this;
    cursor.position = start;
    struct block_t block = document->block(cursor);
    if (!block.isValid()) {
        return "";
    }

    std::string res = "";
    std::string text = block.text();

    int count = end - start + 1;

    for (int i = 0; i < count; i++) {
        size_t idx = start + i;
        int blockIdx = idx - block.position;

        char c = text[blockIdx];
        if (c == 0) {

            if (!block.next) {
                return res;
            }
            block = *block.next;
            if (!block.isValid()) {
                return res;
            }
            text = block.text();

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

    for (;;) {
        struct block_t& block = cur.document->block(cur);
        if (!block.isValid()) {
            return false;
        }
        size_t relativePosition = cur.position - block.position;

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
                cursor->anchorPosition = block.position + found.begin;
                cursor->position = block.position + found.end - 1;
                return true;
            }
        }

        if (!cursorMovePosition(&cur, cursor_t::Move::Down)) {
            return false;
        }

        cursorMovePosition(&cur, cursor_t::Move::StartOfLine);
        firstCursor = false;
    }

    return true;
}
