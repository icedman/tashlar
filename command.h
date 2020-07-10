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

    CMD_SELECT_ALL,
    CMD_SELECT_LINE,
    CMD_DUPLICATE_LINE,
    CMD_DELETE_LINE,
    CMD_SPLIT_LINE, // ENTER

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

    CMD_SAVE,
    CMD_QUIT,
    CMD_UNDO,
    CMD_REDO,

    CMD_ENTER,
    CMD_DELETE,
    CMD_BACKSPACE,
    CMD_INSERT
};

bool processCommand(command_e cmd, struct app_t* app, char ch);

#endif // COMMAND_H