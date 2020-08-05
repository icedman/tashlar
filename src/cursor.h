#ifndef CURSOR_H
#define CURSOR_H

#include "block.h"

struct cursor_position_t {
    cursor_position_t();

    block_ptr block;
    size_t position;
};

struct cursor_t {

    cursor_t();
    ~cursor_t();

    size_t uid;

    cursor_position_t cursor;
    cursor_position_t anchor;

    bool isNull();

    void setPosition(block_ptr block, size_t pos, bool keepAnchor = false);
    void setPosition(cursor_position_t pos, bool keepAnchor = false);
    void setAnchor(block_ptr block, size_t pos);
    void clearSelection();
    bool hasSelection();
    bool isMultiBlockSelection();
    void selectWord();

    cursor_position_t selectionStart();
    cursor_position_t selectionEnd();
    std::string selectedText();
    block_list selectedBlocks();
    bool eraseSelection();

    bool moveStartOfLine(bool keepAnchor = false);
    bool moveEndOfLine(bool keepAnchor = false);
    bool moveStartOfDocument(bool keepAnchor = false);
    bool moveEndOfDocument(bool keepAnchor = false);
    bool moveLeft(int count = 1, bool keepAnchor = false);
    bool moveRight(int count = 1, bool keepAnchor = false);
    bool moveUp(int count = 1, bool keepAnchor = false);
    bool moveDown(int count = 1, bool keepAnchor = false);
    bool moveNextBlock(int count = 1, bool keepAnchor = false);
    bool movePreviousBlock(int count = 1, bool keepAnchor = false);
    bool moveNextWord(bool keepAnchor = false);
    bool movePreviousWord(bool keepAnchor = false);
    bool findWord(std::string word, int direction);

    bool insertText(std::string);
    bool eraseText(int count = 1);
    bool splitLine();
    bool mergeNextLine();

    block_ptr block();
    size_t position();
    block_ptr anchorBlock();
    size_t anchorPosition();

    void print();
};

typedef std::vector<cursor_t> cursor_list;

struct cursor_util {
    static void sortCursors(cursor_list& cursors);
    static void advanceBlockCursors(cursor_list& cursors, cursor_t cur, int advance);
};

#endif // CURSOR_H