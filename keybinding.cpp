#include "keybinding.h"
#include "editor.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>
#include <map>

static std::map<std::string, command_e> keybindings;

void bindDefaults()
{
    bindKeySequence("ctrl+s", CMD_SAVE);
    bindKeySequence("ctrl+q", CMD_QUIT);

    bindKeySequence("ctrl+c", CMD_COPY);
    bindKeySequence("ctrl+x", CMD_CUT);
    bindKeySequence("ctrl+v", CMD_PASTE);
    bindKeySequence("ctrl+z", CMD_UNDO);

    bindKeySequence("ctrl+l", CMD_SELECT_LINE);

    // bindKeySequence("ctrl+shift+d",     CMD_DUPLICATE_LINE);
    // bindKeySequence("ctrl+shift+k",     CMD_DELETE_LINE);
    bindKeySequence("alt+shift+d", CMD_DUPLICATE_LINE);
    bindKeySequence("alt+shift+k", CMD_DELETE_LINE);

    bindKeySequence("ctrl+d", CMD_ADD_CURSOR_FOR_SELECTED_WORD);
    bindKeySequence("ctrl+alt+up", CMD_ADD_CURSOR_AND_MOVE_UP);
    bindKeySequence("ctrl+alt+down", CMD_ADD_CURSOR_AND_MOVE_DOWN);

    bindKeySequence("ctrl+left", CMD_MOVE_CURSOR_PREVIOUS_WORD);
    bindKeySequence("ctrl+right", CMD_MOVE_CURSOR_NEXT_WORD);
    bindKeySequence("ctrl+shift+left", CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED);
    bindKeySequence("ctrl+shift+right", CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED);

    bindKeySequence("left", CMD_MOVE_CURSOR_LEFT);
    bindKeySequence("right", CMD_MOVE_CURSOR_RIGHT);
    bindKeySequence("down", CMD_MOVE_CURSOR_DOWN);
    bindKeySequence("up", CMD_MOVE_CURSOR_UP);

    // bindKeySequence("ctrl+up",               CMD_MOVE_LINE_DOWN);
    // bindKeySequence("ctrl+down",             CMD_MOVE_LINE_UP);

    bindKeySequence("shift+left", CMD_MOVE_CURSOR_LEFT_ANCHORED);
    bindKeySequence("shift+right", CMD_MOVE_CURSOR_RIGHT_ANCHORED);
    bindKeySequence("shift+down", CMD_MOVE_CURSOR_DOWN_ANCHORED);
    bindKeySequence("shift+up", CMD_MOVE_CURSOR_UP_ANCHORED);

    bindKeySequence("ctrl+alt+left", CMD_MOVE_CURSOR_START_OF_LINE);
    bindKeySequence("ctrl+alt+right", CMD_MOVE_CURSOR_END_OF_LINE);
    bindKeySequence("ctrl+shift+alt+left", CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED);
    bindKeySequence("ctrl+shift+alt+right", CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED);

    // bindKeySequence("ctrl+shift+z",     CMD_REDO);

    // bindKeySequence("home",             CMD_MOVE_CURSOR_START_OF_DOCUMENT);
    // bindKeySequence("end",              CMD_MOVE_CURSOR_END_OF_DOCUMENT);

    bindKeySequence("pageup", CMD_MOVE_CURSOR_PREVIOUS_PAGE);
    bindKeySequence("pagedown", CMD_MOVE_CURSOR_NEXT_PAGE);

    bindKeySequence("enter", CMD_ENTER);
    bindKeySequence("delete", CMD_DELETE);
    bindKeySequence("backspace", CMD_BACKSPACE);
}

// if != 0, then there is data to be read on stdin
int kbhit(int timeout)
{
    // timeout structure passed into select
    struct timeval tv;
    // fd_set passed into select
    fd_set fds;
    // Set up the timeout.  here we can wait for 1 second
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    // Zero out the fd_set - make sure it's pristine
    FD_ZERO(&fds);
    // Set the FD that we want to read
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    // select takes the last file descriptor value + 1 in the fdset to check,
    // the fdset for reads, writes, and errors.  We are only passing in reads.
    // the last parameter is the timeout.  select will return if an FD is ready or
    // the timeout has occurred
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    // return 0 if STDIN is not ready to be read.

    return FD_ISSET(STDIN_FILENO, &fds);
}

int other_escape_sequence(int c, std::string& keySequence)
{
    char tmp[32];

    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z')) {
        sprintf(tmp, "alt+%c", c);
        keySequence = tmp;
        return ALT_;
    }

    if (c >= 'A' && c <= 'Z') {
        sprintf(tmp, "alt+shift+%c", (c + 'a' - 'A'));
        keySequence = tmp;
        return ALT_;
    }

    if ((c + 'a' - 1) >= 'a' && (c + 'a' - 1) <= 'z') {
        sprintf(tmp, "ctrl+alt+%c", (c + 'a' - 1));
        keySequence = tmp;
        return CTRL_ALT_;
    }

    app_t::log("escape+%d a:%d A:%d 0:%d 9:%d\n", c, 'a', 'A', '0', '9');

    return ESC;
}

int read_key_sequence(std::string& keySequence)
{
    keySequence = "";

    std::string sequence = "";

    char seq[4];
    int wait = 500;

    if (!kbhit(wait)) {
        return ESC;
    }
    read(STDIN_FILENO, &seq[0], 1);

    if (!kbhit(wait)) {
        return other_escape_sequence(seq[0], keySequence);
    }
    read(STDIN_FILENO, &seq[1], 1);

    /* ESC [ sequences. */
    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {

            /* Extended escape, read additional byte. */
            if (!kbhit(wait)) {
                return ESC;
            }
            read(STDIN_FILENO, &seq[2], 1);

            if (seq[2] == '~') {
                switch (seq[1]) {
                case '3':
                    keySequence = "delete";
                    return KEY_DC;
                case '5':
                    keySequence = "pageup";
                    return PAGE_UP;
                case '6':
                    keySequence = "pagedown";
                    return PAGE_DOWN;
                }
            }

            if (seq[2] == ';') {
                if (!kbhit(wait)) {
                    return ESC;
                }
                read(STDIN_FILENO, &seq[0], 1);
                if (!kbhit(wait)) {
                    return ESC;
                }
                read(STDIN_FILENO, &seq[1], 1);

                sequence = "shift+";
                if (seq[0] == '2') {
                    // app_t::log("shift+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return KEY_SR;
                    case 'B':
                        keySequence = sequence + "down";
                        return KEY_SF;
                    case 'C':
                        keySequence = sequence + "right";
                        return KEY_SRIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return KEY_SLEFT;
                    }
                }

                sequence = "ctrl+";
                if (seq[0] == '5') {
                    // app_t::log("ctrl+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return CTRL_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return CTRL_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return CTRL_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return CTRL_LEFT;
                    }
                }

                sequence = "ctrl+shift+";
                if (seq[0] == '6') {
                    // app_t::log("ctrl+shift+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return CTRL_SHIFT_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return CTRL_SHIFT_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return CTRL_SHIFT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return CTRL_SHIFT_LEFT;
                    }
                }

                sequence = "ctrl+alt+";
                if (seq[0] == '7') {
                    // app_t::log("ctrl+alt+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return CTRL_ALT_UP;
                    case 'B':
                        keySequence = sequence + "down";
                        return CTRL_ALT_DOWN;
                    case 'C':
                        keySequence = sequence + "right";
                        return CTRL_ALT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return CTRL_ALT_LEFT;
                    }
                }

                sequence = "ctrl+shift+alt+";
                if (seq[0] == '8') {
                    // app_t::log("ctrl+shift+alt+%d\n", seq[1]);
                    switch (seq[1]) {
                    case 'A':
                        keySequence = sequence + "up";
                        return KEY_SR;
                    case 'B':
                        keySequence = sequence + "down";
                        return KEY_SF;
                    case 'C':
                        keySequence = sequence + "right";
                        return CTRL_SHIFT_ALT_RIGHT;
                    case 'D':
                        keySequence = sequence + "left";
                        return CTRL_SHIFT_ALT_LEFT;
                    }
                }

                return ESC;
            }

        } else {
            // app_t::log("escape+[+%d\n", seq[1]);
            switch (seq[1]) {
            case 'A':
                keySequence = "up";
                return KEY_UP;
            case 'B':
                keySequence = "down";
                return KEY_DOWN;
            case 'C':
                keySequence = "right";
                return KEY_RIGHT;
            case 'D':
                keySequence = "left";
                return KEY_LEFT;
            case 'H':
                keySequence = "home";
                return HOME_KEY;
            case 'F':
                keySequence = "end";
                return END_KEY;
            }

            return ESC;
        }
    }

    /* ESC O sequences. */
    else if (seq[0] == 'O') {
        // app_t::log("escap+O+%d\n", seq[1]);
        switch (seq[1]) {
        case 'H':
            return HOME_KEY;
        case 'F':
            return END_KEY;
        }
    }

    return ESC;
}

int readKey(std::string& keySequence)
{
    if (kbhit(50) != 0) {
        int c;
        if (read(STDIN_FILENO, &c, 1) != 0) {

            // app_t::log("key:%d\n", c);

            if (c == ESC) {
                keySequence = "";
                return read_key_sequence(keySequence);
            }

            switch (c) {
            case ENTER:
                keySequence = "enter";
                return c;
            case BACKSPACE:
            case KEY_BACKSPACE:
                keySequence = "backspace";
                return c;
            case KEY_RESIZE:
                keySequence = "resize";
                return c;
            }

            if (CTRL_KEY(c) == c) {
                keySequence = "ctrl+";
                c = 'a' + (c - 1);
                if (c >= 'a' && c <= 'z') {
                    keySequence += c;
                    return c;
                } else {
                    keySequence += '?';
                }

                return c;
            }

            return c;
        }
    }
    return -1;
}

void bindKeySequence(std::string keys, command_e command)
{
    if (keybindings.find(keys) != keybindings.end()) {
        keybindings.erase(keys);
    }
    keybindings.emplace(keys, command);
}

command_e commandKorKeys(std::string keys)
{
    if (keybindings.find(keys) != keybindings.end()) {
        return keybindings[keys];
    }

    app_t::log(keys.c_str());

    return command_e::CMD_UNKNOWN;
}