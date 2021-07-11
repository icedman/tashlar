#ifndef EDITOR_H
#define EDITOR_H

#include "completer.h"
#include "document.h"
#include "highlighter.h"
#include "keybinding.h"
#include "snapshot.h"
#include "view.h"

typedef std::vector<operation_t> operation_list;

struct config_t {
    config_t();
    
    static config_t* instance();

    bool lineWrap;
};

struct editor_t : view_t {
    editor_t();
    ~editor_t();

    void pushOp(std::string op, std::string params = "");
    void pushOp(operation_e op, std::string params = "");
    void pushOp(operation_t op);
    void runOp(operation_t op);

    void undo();

    void matchBracketsUnderCursor();
    bracket_info_t bracketAtCursor(cursor_t& cursor);
    cursor_t cursorAtBracket(bracket_info_t bracket);
    cursor_t findLastOpenBracketCursor(block_ptr block);
    cursor_t findBracketMatchCursor(bracket_info_t bracket, cursor_t cursor);

    void toggleFold(size_t line);
    void ensureVisibleCursor();

    // view
    void update(int delta) override;
    void layout(int x, int y, int width, int height) override;
    void render() override;
    void preRender() override;
    bool input(char ch, std::string keys) override;
    void applyTheme() override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseDrag(int x, int y, bool within) override;

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
};

typedef std::shared_ptr<struct editor_t> editor_ptr;
typedef std::vector<editor_ptr> editor_list;

#endif // EDITOR_H
