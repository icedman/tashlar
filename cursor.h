#ifndef CURSOR_H
#define CURSOR_H

#include <cstddef>
#include <string>
#include <vector>

struct document_t;
struct block_t;
struct cursor_t;

struct cursor_position_t
{
    cursor_position_t() : block(0), position(0) {}

    struct block_t *block;
    int position;

    size_t absolutePosition();
};

struct cursor_t {
    enum Move {
        NoMove,
        Up,
        Down,
        Left,
        Right,
        PrevBlock,
        NextBlock,
        WordLeft,
        WordRight,
        StartOfLine,
        EndOfLine,
        StartOfDocument,
        EndOfDocument
    };

    cursor_t();

    size_t uid;
    size_t _docUpdateUid;

    struct document_t* _document;

    cursor_position_t _position;
    cursor_position_t _anchor;
    size_t _preferredRelativePosition;

    bool hasSelection();
    bool isNull();

    void setPosition(struct block_t *block, size_t position);
    void setAnchor(struct block_t *block, size_t anchor);

    size_t position();
    size_t anchorPosition();
    size_t relativePosition();

    void clearSelection();
    size_t selectionStart();
    size_t selectionEnd();

    struct cursor_t selectionStartCursor();
    struct cursor_t selectionEndCursor();

    struct document_t* document();
    struct block_t* block();

    std::vector<struct block_t*> selectedBlocks();
    std::string selectedText();
    bool isMultiBlockSelection();
};

bool cursorMovePosition(struct cursor_t* cursor, enum cursor_t::Move move, bool keepAnchor = false, int count = 1);
int cursorInsertText(struct cursor_t* cursor, std::string t);
int cursorEraseText(struct cursor_t* cursor, int c);
void cursorSplitBlock(struct cursor_t* cursor);
void cursorSelectWord(struct cursor_t* cursor);
bool cursorFindWord(struct cursor_t* cursor, std::string t, int direction = 0);
int cursorDeleteSelection(struct cursor_t* cursor);
int cursorIndent(struct cursor_t* cursor);
int cursorUnindent(struct cursor_t* cursor);

int countToTabStop(struct cursor_t* cursor);

#endif // CURSOR_H
