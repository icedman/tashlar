#ifndef EDITOR_H
#define EDITOR_H

#include "document.h"
#include "completer.h"
#include "highlighter.h"
#include "operation.h"
#include "snapshot.h"

typedef std::vector<operation_t> operation_list;

struct view_t;
struct editor_t {

    std::string name;

    editor_t();
    ~editor_t();

    void pushOp(std::string op, std::string params = "");
    void pushOp(operation_e op, std::string params = "");
    void pushOp(operation_t op);
    void runOp(operation_t op);
    void runAllOps();

    void undo();

    void matchBracketsUnderCursor();
    bracket_info_t bracketAtCursor(cursor_t& cursor);
    cursor_t cursorAtBracket(bracket_info_t bracket);
    cursor_t findLastOpenBracketCursor(block_ptr block);
    cursor_t findBracketMatchCursor(bracket_info_t bracket, cursor_t cursor);

    void toggleFold(size_t line);

    void applyTheme();
    bool input(char ch, std::string keys);

    void highlight(int startingLine, int count);

    void toMarkup();

    document_t document;
    operation_list operations;

    void createSnapshot();

    snap_list snapshots;

    std::string inputBuffer;

    highlighter_t highlighter;
    bracket_info_t cursorBracket1;
    bracket_info_t cursorBracket2;

    completer_t completer;

    bool _scrollToCursor;
    int _foldedLines;

    view_t *view;
};

typedef std::shared_ptr<struct editor_t> editor_ptr;
typedef std::vector<editor_ptr> editor_list;

#endif // EDITOR_H
