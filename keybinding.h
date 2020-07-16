#ifndef KEYBINDING_H
#define KEYBINDING_H

#include <string>

#include "command.h"

#define CTRL_KEY(k) ((k)&0x1f)

enum KEY_ACTION {
    KEY_NULL = 0, /* NULL */
    ENTER = 13,
    TAB = 9,
    ESC = 27,
    BACKSPACE = 127,
    RESIZE = 150,
    /* The following are just soft codes, not really reported by the terminal directly. */
    ALT_ = 1000,
    CTRL_,
    CTRL_ALT_,
    CTRL_SHIFT_,
    CTRL_SHIFT_ALT_,
    CTRL_UP,
    CTRL_DOWN,
    CTRL_LEFT,
    CTRL_RIGHT,
    CTRL_HOME,
    CTRL_END,
    CTRL_SHIFT_UP,
    CTRL_SHIFT_DOWN,
    CTRL_SHIFT_LEFT,
    CTRL_SHIFT_RIGHT,
    CTRL_SHIFT_HOME,
    CTRL_SHIFT_END,
    CTRL_ALT_UP,
    CTRL_ALT_DOWN,
    CTRL_ALT_LEFT,
    CTRL_ALT_RIGHT,
    CTRL_ALT_HOME,
    CTRL_ALT_END,
    CTRL_SHIFT_ALT_LEFT,
    CTRL_SHIFT_ALT_RIGHT,
    CTRL_SHIFT_ALT_HOME,
    CTRL_SHIFT_ALT_END,
    SHIFT_HOME,
    SHIFT_END,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

int kbhit(int timeout = 500);

int readKey(std::string& keySequence);
void bindDefaults();
void bindKeySequence(std::string keys, command_e command);
command_e commandKorKeys(std::string keys);

#endif // KEYBINDING_H
