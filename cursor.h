#ifndef CURSOR_H
#define CURSOR_H

#include <cstddef>
#include <string>

struct document_t;
struct block_t;
struct cursor_t;

struct cursor_t {
public:
    enum Move {
        NoMove,
        Up,
        Down,
        Left,
        Right,
        StartOfLine,
        EndOfLine,
        StartOfDocument,
        EndOfDocument
    };

    cursor_t()
        : document(0)
        , block(0)
        , position(0)
        , anchorPosition(0)
    {}

    struct document_t* document;
    struct block_t* block;

    size_t position;
    size_t anchorPosition;

    size_t relativePosition;
    size_t relativeAnchorPosition;

    bool hasSelection()
    {
        return anchorPosition != position;
    }

    bool isNull();

    void update();

    std::string selectedText();
};

bool cursorMovePosition(struct cursor_t* cursor, enum cursor_t::Move move, bool keepAnchor = false, int count = 1);
void cursorInsertText(struct cursor_t* cursor, std::string t);
void cursorEraseText(struct cursor_t* cursor, int c);
void cursorSplitBlock(struct cursor_t* cursor);

#endif // CURSOR_H