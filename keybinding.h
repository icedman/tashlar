#ifndef KEYBINDING_H
#define KEYBINDING_H

#include <string>

#include "command.h"

#define CTRL_KEY(k) ((k)&0x1f)

enum KEY_ACTION {
    KEY_NULL = 0, /* NULL */
    ENTER = 13, /* Enter */
    ESC = 27, /* Escape */
    BACKSPACE = 127, /* Backspace */
    /* The following are just soft codes, not really reported by the
         * terminal directly. */
    ALT_ = 1000,
    CTRL_,
    CTRL_ALT_,
    CTRL_SHIFT_,
    CTRL_SHIFT_ALT_,
    CTRL_UP,
    CTRL_DOWN,
    CTRL_LEFT,
    CTRL_RIGHT,
    CTRL_SHIFT_UP,
    CTRL_SHIFT_DOWN,
    CTRL_SHIFT_LEFT,
    CTRL_SHIFT_RIGHT,
    CTRL_ALT_UP,
    CTRL_ALT_DOWN,
    CTRL_ALT_LEFT,
    CTRL_ALT_RIGHT,
    CTRL_SHIFT_ALT_LEFT,
    CTRL_SHIFT_ALT_RIGHT,
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