#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

struct editor_t;

enum command_e {
    CMD_UNKNOWN = 2000,
    CMD_CANCEL,
    CMD_CUT,
    CMD_COPY,
    CMD_PASTE,
    CMD_SELECT_WORD,
    CMD_SELECT_LINE,
    CMD_CLEAR_SELECTION,
    CMD_ADD_CURSOR_ABOVE,
    CMD_ADD_CURSOR_BELOW,
    CMD_ADD_CURSOR_SELECTED_WORD,
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
    CMD_SAVE,
    CMD_QUIT,
    CMD_UNDO,
    CMD_REDO,
    CMD_DELETE,
    CMD_BACKSPACE,
    CMD_INSERT
};

void processCommand(struct editor_t *editor, command_e cmd, std::string args);

#endif // COMMAND_H