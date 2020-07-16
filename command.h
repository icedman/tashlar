#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

struct editor_t;

enum command_e {
    CMD_UNKNOWN = 0,
    CMD_CANCEL = 1000,
    CMD_CUT,
    CMD_COPY,
    CMD_PASTE,
    CMD_SELECT_WORD,

    CMD_TAB_0,
    CMD_TAB_1,
    CMD_TAB_2,
    CMD_TAB_3,
    CMD_TAB_4,
    CMD_TAB_5,
    CMD_TAB_6,
    CMD_TAB_7,
    CMD_TAB_8,
    CMD_TAB_9,

    CMD_SELECT_ALL,
    CMD_SELECT_LINE,
    CMD_DUPLICATE_LINE,
    CMD_DELETE_LINE,
    CMD_MOVE_LINE_UP,
    CMD_MOVE_LINE_DOWN,
    CMD_SPLIT_LINE, // ENTER
    
    CMD_TOGGLE_FOLD,

    CMD_CLEAR_SELECTION,
    CMD_DUPLICATE_SELECTION,
    CMD_DELETE_SELECTION,
    CMD_ADD_CURSOR_AND_MOVE_UP,
    CMD_ADD_CURSOR_AND_MOVE_DOWN,
    CMD_ADD_CURSOR_FOR_SELECTED_WORD,
    CMD_CLEAR_CURSORS,

    CMD_MOVE_CURSOR_LEFT,
    CMD_MOVE_CURSOR_RIGHT,
    CMD_MOVE_CURSOR_UP,
    CMD_MOVE_CURSOR_DOWN,
    CMD_MOVE_CURSOR_NEXT_WORD,
    CMD_MOVE_CURSOR_PREVIOUS_WORD,
    CMD_MOVE_CURSOR_START_OF_LINE,
    CMD_MOVE_CURSOR_END_OF_LINE,
    CMD_MOVE_CURSOR_START_OF_DOCUMENT,
    CMD_MOVE_CURSOR_END_OF_DOCUMENT,
    CMD_MOVE_CURSOR_NEXT_PAGE,
    CMD_MOVE_CURSOR_PREVIOUS_PAGE,

    CMD_MOVE_CURSOR_LEFT_ANCHORED,
    CMD_MOVE_CURSOR_RIGHT_ANCHORED,
    CMD_MOVE_CURSOR_UP_ANCHORED,
    CMD_MOVE_CURSOR_DOWN_ANCHORED,
    CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED,
    CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED,
    CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED,
    CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED,
    CMD_MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED,
    CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED,
    CMD_MOVE_CURSOR_NEXT_PAGE_ANCHORED,
    CMD_MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED,

    CMD_FOCUS_WINDOW_LEFT,
    CMD_FOCUS_WINDOW_RIGHT,
    CMD_FOCUS_WINDOW_UP,
    CMD_FOCUS_WINDOW_DOWN,

    CMD_NEW_TAB,
    CMD_CLOSE_TAB,

    CMD_SAVE,
    CMD_QUIT,
    CMD_UNDO,
    CMD_REDO,

    CMD_RESIZE,
    CMD_TAB,
    CMD_ENTER,
    CMD_DELETE,
    CMD_BACKSPACE,
    CMD_INSERT,

    CMD_CYCLE_FOCUS,
    CMD_TOGGLE_EXPLORER,

    CMD_POPUP_SEARCH,
    CMD_POPUP_SEARCH_LINE,
    CMD_POPUP_COMMANDS,
    CMD_POPUP_FILES,
    CMD_POPUP_COMPLETION,

    CMD_HISTORY_SNAPSHOT
};

bool processEditorCommand(command_e cmd, char ch);

#endif // COMMAND_H
