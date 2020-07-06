#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

#include "util.h"
#include <dirent.h>

std::vector<std::string> split_path(const std::string& str, const std::set<char> delimiters)
{
    std::vector<std::string> result;

    char const* pch = str.c_str();
    char const* start = pch;
    for (; *pch; ++pch) {
        if (delimiters.find(*pch) != delimiters.end()) {
            if (start != pch) {
                std::string str(start, pch);
                result.push_back(str);
            } else {
                result.push_back("");
            }
            start = pch + 1;
        }
    }
    result.push_back(start);

    return result;
}

std::vector<size_t> split_path_to_indices(const std::string& str, const std::set<char> delimiters)
{
    std::vector<size_t> result;

    char const* s = str.c_str();
    char const* ws = s;
    char const* pch = s;
    for (; *pch; ++pch) {
        if (delimiters.find(*pch) != delimiters.end()) {
            if (ws < pch) {
                result.push_back(ws - s);
            }
            ws = pch + 1;
        }
    }
    result.push_back(ws - s);

    return result;
}

std::vector<std::string> enumerate_dir(const std::string path)
{
    std::cout << path << std::endl;

    std::vector<std::string> res;
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fullPath = path;
            fullPath += ent->d_name;
            res.push_back(fullPath);
        }
        closedir(dir);
    }

    return res;
}

int editor_read_key()
{
    int fd = STDIN_FILENO;

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return -1;
    }

    if (c == ESC) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        /* ESC [ sequences. */
        if (seq[0] == '[') {

            if (seq[1] >= '0' && seq[1] <= '9') {

                // std::cout << seq[0] << seq[1] << std::endl;
                
                /* Extended escape, read additional byte. */
                if (read(fd, &seq[2], 1) == 0)
                    return ESC;
                
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '3':
                        return KEY_DC;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    }
                }

                if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[0], 1) != 1)
                        return '\x1b';
                    if (read(STDIN_FILENO, &seq[1], 1) != 1)
                        return '\x1b';

                    // std::cout << seq[0] << seq[1] << std::endl;

                    // shift +
                    if (seq[0] == '2') {
                        switch (seq[1]) {
                        case 'A':
                            return KEY_SR;
                        case 'B':
                            return KEY_SF;
                        case 'C':
                            return KEY_SRIGHT;
                        case 'D':
                            return KEY_SLEFT;
                        }
                    }

                    // ctrl +
                    if (seq[0] == '5') {
                        switch (seq[1]) {
                        case 'A':
                            return KEY_UP;
                        case 'B':
                            return KEY_DOWN;
                        case 'C':
                            return CTRL_RIGHT;
                        case 'D':
                            return CTRL_LEFT;
                        }
                    }

                    // ctrl + shift +
                    if (seq[0] == '6') {
                        switch (seq[1]) {
                        case 'A':
                            return CTRL_SHIFT_UP;
                        case 'B':
                            return CTRL_SHIFT_DOWN;
                        case 'C':
                            return CTRL_SHIFT_RIGHT;
                        case 'D':
                            return CTRL_SHIFT_LEFT;
                        }
                    }

                    // ctrl + alt +
                    if (seq[0] == '7') {
                        switch (seq[1]) {
                        case 'A':
                            return CTRL_ALT_UP;
                        case 'B':
                            return CTRL_ALT_DOWN;
                        case 'C':
                            return CTRL_ALT_RIGHT;
                        case 'D':
                            return CTRL_ALT_LEFT;
                        }
                    }

                    // ctrl + shift + alt +
                    if (seq[0] == '8') {
                        switch (seq[1]) {
                        case 'A':
                            return KEY_SR;
                        case 'B':
                            return KEY_SF;
                        case 'C':
                            return CTRL_SHIFT_ALT_RIGHT;
                        case 'D':
                            return CTRL_SHIFT_ALT_LEFT;
                        }
                    }
                }

            } else {
                switch (seq[1]) {
                case 'A':
                    return KEY_UP;;
                case 'B':
                    return KEY_DOWN;;
                case 'C':
                    return KEY_RIGHT;;
                case 'D':
                    return KEY_LEFT;;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        }

        /* ESC O sequences. */
        else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }
    }

    return c;
}