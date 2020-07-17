#ifndef CURSOR_H
#define CURSOR_H

#include <cstddef>
#include <string>

struct document_t;
struct block_t;
struct cursor_t;

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

    cursor_t()
        : uid(0)
        , document(0)
        , block(0)
        , position(0)
        , anchorPosition(0)
        , preferredRelativePosition(0)
    {
    }

    struct document_t* document;
    struct block_t* block;

    size_t position;
    size_t anchorPosition;
    size_t uid;

    size_t preferredRelativePosition;

    bool hasSelection()
    {
        return anchorPosition != position;
    }

    bool isNull();

    void update();
    void setPosition(size_t pos)
    {
        position = pos;
        anchorPosition = pos;
    }
    void clearSelection() { anchorPosition = position; }

    std::string selectedText();
};

bool cursorMovePosition(struct cursor_t* cursor, enum cursor_t::Move move, bool keepAnchor = false, int count = 1);
int cursorInsertText(struct cursor_t* cursor, std::string t);
int cursorEraseText(struct cursor_t* cursor, int c);
void cursorSplitBlock(struct cursor_t* cursor);
void cursorSelectWord(struct cursor_t* cursor);
bool cursorFindWord(struct cursor_t* cursor, std::string t);
int cursorDeleteSelection(struct cursor_t* cursor);

#endif // CURSOR_H
