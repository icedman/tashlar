#include "cursor.h"
#include "document.h"

void cursor_t::update()
{
    relativePosition = position;
    relativeAnchorPosition = anchorPosition;

    block = &document->block(*this);
    if (block) {
        relativePosition -= block->position;
        relativeAnchorPosition -= block->position;
    }
}

std::string cursor_t::selectedText()
{
    if (!hasSelection()) {
        return "";
    }

    return "heya!";
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

    switch (move) {
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
        break;
    }

    if (!keepAnchor) {
        cursor->anchorPosition = cursor->position;
    }

    cursor->update();
    if (count > 0) {
        return cursorMovePosition(cursor, move, keepAnchor, count);
    }

    return !cursor->isNull();
}

void cursorInsertText(struct cursor_t* cursor, std::string t)
{
    struct block_t& block = cursor->document->block(*cursor);
    if (!block.isValid()) {
        return;
    }

    std::string blockText = block.text();
    blockText.insert(cursor->position - block.position, t);
    block.setText(blockText);

    cursor->document->update();
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
    cursor->document->update();
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
