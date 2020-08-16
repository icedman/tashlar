#include "keybinding.h"

typedef struct {
    const char* name;
    operation_e op;
    const char* keys;
} operation_name;

static const operation_name operation_names[] = {
    { "UNKNOWN", UNKNOWN, "" },
    { "CANCEL", CANCEL, "escape" },

    { "TAB", TAB, "tab" },
    { "ENTER", ENTER, "enter" },
    { "DELETE", DELETE, "delete" },
    { "BACKSPACE", BACKSPACE, "backspace" },
    { "INSERT", INSERT, "" },

    { "CUT", CUT, "ctrl+x" },
    { "COPY", COPY, "ctrl+c" },
    { "PASTE", PASTE, "ctrl+v" },
    { "SELECT_WORD", SELECT_WORD, "" },

    { "INDENT", INDENT, "ctrl+l+ctrl+k" },
    { "UNINDENT", UNINDENT, "ctrl+l+ctrl+j" },

    { "SELECT_ALL", SELECT_ALL, "ctrl+a" },
    { "SELECT_LINE", SELECT_LINE, "ctrl+l+ctrl+l" },
    { "DUPLICATE_LINE", DUPLICATE_LINE, "ctrl+l+ctrl+v" },
    { "DELETE_LINE", DELETE_LINE, "ctrl+l+ctrl+x" },
    { "MOVE_LINE_UP", MOVE_LINE_UP, "" },
    { "MOVE_LINE_DOWN", MOVE_LINE_DOWN, "" },

    { "CLEAR_SELECTION", CLEAR_SELECTION, "" },
    { "DUPLICATE_SELECTION", DUPLICATE_SELECTION, "" },
    { "DELETE_SELECTION", DELETE_SELECTION, "" },
    { "ADD_CURSOR_AND_MOVE_UP", ADD_CURSOR_AND_MOVE_UP, "ctrl+up" },
    { "ADD_CURSOR_AND_MOVE_DOWN", ADD_CURSOR_AND_MOVE_DOWN, "ctrl+down" },
    { "ADD_CURSOR_FOR_SELECTED_WORD", ADD_CURSOR_FOR_SELECTED_WORD, "ctrl+d" },
    { "CLEAR_CURSORS", CLEAR_CURSORS, "" },

    { "MOVE_CURSOR", MOVE_CURSOR, "" },
    { "MOVE_CURSOR_LEFT", MOVE_CURSOR_LEFT, "left" },
    { "MOVE_CURSOR_RIGHT", MOVE_CURSOR_RIGHT, "right" },
    { "MOVE_CURSOR_UP", MOVE_CURSOR_UP, "up" },
    { "MOVE_CURSOR_DOWN", MOVE_CURSOR_DOWN, "down" },
    { "MOVE_CURSOR_NEXT_WORD", MOVE_CURSOR_NEXT_WORD, "ctrl+right" },
    { "MOVE_CURSOR_PREVIOUS_WORD", MOVE_CURSOR_PREVIOUS_WORD, "ctrl+left" },
    { "MOVE_CURSOR_START_OF_LINE", MOVE_CURSOR_START_OF_LINE, "home" },
    { "MOVE_CURSOR_END_OF_LINE", MOVE_CURSOR_END_OF_LINE, "end" },
    { "MOVE_CURSOR_START_OF_DOCUMENT", MOVE_CURSOR_START_OF_DOCUMENT, "ctrl+home" },
    { "MOVE_CURSOR_END_OF_DOCUMENT", MOVE_CURSOR_END_OF_DOCUMENT, "ctrl+end" },
    { "MOVE_CURSOR_NEXT_PAGE", MOVE_CURSOR_NEXT_PAGE, "pagedown" },
    { "MOVE_CURSOR_PREVIOUS_PAGE", MOVE_CURSOR_PREVIOUS_PAGE, "pageup" },

    { "MOVE_CURSOR_ANCHORED", MOVE_CURSOR_ANCHORED, "" },
    { "MOVE_CURSOR_LEFT_ANCHORED", MOVE_CURSOR_LEFT_ANCHORED, "shift+left" },
    { "MOVE_CURSOR_RIGHT_ANCHORED", MOVE_CURSOR_RIGHT_ANCHORED, "shift+right" },
    { "MOVE_CURSOR_UP_ANCHORED", MOVE_CURSOR_UP_ANCHORED, "shift+up" },
    { "MOVE_CURSOR_DOWN_ANCHORED", MOVE_CURSOR_DOWN_ANCHORED, "shift+down" },
    { "MOVE_CURSOR_NEXT_WORD_ANCHORED", MOVE_CURSOR_NEXT_WORD_ANCHORED, "ctrl+shift+right" },
    { "MOVE_CURSOR_PREVIOUS_WORD_ANCHORED", MOVE_CURSOR_PREVIOUS_WORD_ANCHORED, "ctrl+shift+left" },
    { "MOVE_CURSOR_START_OF_LINE_ANCHORED", MOVE_CURSOR_START_OF_LINE_ANCHORED, "shift+home" },
    { "MOVE_CURSOR_END_OF_LINE_ANCHORED", MOVE_CURSOR_END_OF_LINE_ANCHORED, "shift+end" },
    { "MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED", MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED, "ctrl+shift+home" },
    { "MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED", MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED, "ctrl+shift+end" },
    { "MOVE_CURSOR_NEXT_PAGE_ANCHORED", MOVE_CURSOR_NEXT_PAGE_ANCHORED, "shift+pagedown" },
    { "MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED", MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED, "shift+pageup" },

    { "MOVE_FOCUS_LEFT", MOVE_FOCUS_LEFT, "ctrl+alt+left" },
    { "MOVE_FOCUS_RIGHT", MOVE_FOCUS_RIGHT, "ctrl+alt+right" },
    { "MOVE_FOCUS_UP", MOVE_FOCUS_UP, "ctrl+alt+up" },
    { "MOVE_FOCUS_DOWN", MOVE_FOCUS_DOWN, "ctrl+alt+down" },

    { "SPLIT_VIEW", SPLIT_VIEW, "ctrl+k" },
    { "TOGGLE_FOLD", TOGGLE_FOLD, "ctrl+l+ctrl+f" },

    { "OPEN", OPEN, "" },
    { "SAVE", SAVE, "ctrl+s" },
    { "SAVE_AS", SAVE_AS, "" },
    { "SAVE_COPY", SAVE_COPY, "" },
    { "UNDO", UNDO, "ctrl+z" },
    { "REDO", REDO, "" },
    { "CLOSE", CLOSE, "alt+w" },
    { "QUIT", QUIT, "ctrl+q" },

    { "POPUP_SEARCH", POPUP_SEARCH, "ctrl+f" },
    { "POPUP_SEARCH_LINE", POPUP_SEARCH_LINE, "ctrl+g" },
    { "POPUP_COMMANDS", POPUP_COMMANDS, "ctrl+o" },
    { "POPUP_FILES", POPUP_FILES, "ctrl+p" },
    { "POPUP_COMPLETION", POPUP_COMPLETION, "" },

    { 0 }
};

operation_e operationFromName(std::string name)
{
    for (int i = 0; operation_names[i].name; i++) {
        if (name == operation_names[i].name) {
            return operation_names[i].op;
        }
    }
    return UNKNOWN;
}

operation_e operationFromKeys(std::string keys)
{
    for (int i = 0; operation_names[i].name; i++) {
        if (keys == operation_names[i].keys) {
            return operation_names[i].op;
        }
    }
    return UNKNOWN;
}

std::string nameFromOperation(operation_e op)
{
    for (int i = 0; operation_names[i].name; i++) {
        if (op == operation_names[i].op) {
            return operation_names[i].name;
        }
    }
    return "unknown";
}
