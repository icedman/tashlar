#ifndef UTIL_H
#define UTIL_H

#include <set>
#include <string>
#include <vector>

#define CTRL_KEY(k) ((k)&0x1f)

enum KEY_ACTION {
    KEY_NULL = 0, /* NULL */
    ENTER = 13, /* Enter */
    ESC = 27, /* Escape */
    BACKSPACE = 127, /* Backspace */
    /* The following are just soft codes, not really reported by the
         * terminal directly. */
    CTRL_UP = 1000,
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

std::vector<size_t> split_path_to_indices(const std::string& str, const std::set<char> delimiters);
std::vector<std::string> split_path(const std::string& str, const std::set<char> delimiters);
std::vector<std::string> enumerate_dir(const std::string path);

int kbhit();
int editor_read_key();

bool expand_path(char** path);

#endif // UTIL_H