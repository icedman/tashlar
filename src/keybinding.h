#ifndef KEYBINDING_H
#define KEYBINDING_H

#include <string>
#include <vector>

enum operation_e {
    UNKNOWN = 0,
    CANCEL,
    CUT,
    COPY,
    PASTE,
    SELECT_WORD,

    INDENT,
    UNINDENT,
    TOGGLE_COMMENT,

    SELECT_ALL,
    SELECT_LINE,
    DUPLICATE_LINE,
    DELETE_LINE,
    MOVE_LINE_UP,
    MOVE_LINE_DOWN,

    CLEAR_SELECTION,
    DUPLICATE_SELECTION,
    DELETE_SELECTION,
    ADD_CURSOR_AND_MOVE_UP,
    ADD_CURSOR_AND_MOVE_DOWN,
    ADD_CURSOR_FOR_SELECTED_WORD,
    CLEAR_CURSORS,

    NEW_TAB,
    TAB_1,
    TAB_2,
    TAB_3,
    TAB_4,
    TAB_5,
    TAB_6,
    TAB_7,
    TAB_8,
    TAB_9,

    MOVE_CURSOR,
    MOVE_CURSOR_LEFT,
    MOVE_CURSOR_RIGHT,
    MOVE_CURSOR_UP,
    MOVE_CURSOR_DOWN,
    MOVE_CURSOR_NEXT_WORD,
    MOVE_CURSOR_PREVIOUS_WORD,
    MOVE_CURSOR_START_OF_LINE,
    MOVE_CURSOR_END_OF_LINE,
    MOVE_CURSOR_START_OF_DOCUMENT,
    MOVE_CURSOR_END_OF_DOCUMENT,
    MOVE_CURSOR_NEXT_PAGE,
    MOVE_CURSOR_PREVIOUS_PAGE,

    MOVE_CURSOR_ANCHORED,
    MOVE_CURSOR_LEFT_ANCHORED,
    MOVE_CURSOR_RIGHT_ANCHORED,
    MOVE_CURSOR_UP_ANCHORED,
    MOVE_CURSOR_DOWN_ANCHORED,
    MOVE_CURSOR_NEXT_WORD_ANCHORED,
    MOVE_CURSOR_PREVIOUS_WORD_ANCHORED,
    MOVE_CURSOR_START_OF_LINE_ANCHORED,
    MOVE_CURSOR_END_OF_LINE_ANCHORED,
    MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED,
    MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED,
    MOVE_CURSOR_NEXT_PAGE_ANCHORED,
    MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED,

    MOVE_FOCUS_LEFT,
    MOVE_FOCUS_RIGHT,
    MOVE_FOCUS_UP,
    MOVE_FOCUS_DOWN,

    SPLIT_VIEW,
    TOGGLE_FOLD,

    OPEN,
    SAVE,
    SAVE_AS,
    SAVE_COPY,
    UNDO,
    REDO,
    CLOSE,
    QUIT,

    TAB,
    ENTER,
    DELETE,
    BACKSPACE,
    INSERT,

    POPUP_SEARCH,
    POPUP_SEARCH_LINE,
    POPUP_COMMANDS,
    POPUP_FILES,
    POPUP_COMPLETION
};

struct operation_t {
    operation_e op;
    std::string params;
    std::string name;
    std::string keys;
};

typedef std::vector<operation_t> operation_list;

operation_e operationFromName(std::string name);
operation_e operationFromKeys(std::string keys);
std::string nameFromOperation(operation_e op);

struct keybinding_t {

    keybinding_t();
    ~keybinding_t();

    std::vector<operation_t> binding;

    static keybinding_t* instance();

    void initialize();
};

#endif // KEYBINDING_H
