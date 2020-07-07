#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <wordexp.h>

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

char* join_args(char** argv, int argc)
{
    // if (!sway_assert(argc > 0, "argc should be positive")) {
    //     return NULL;
    // }
    int len = 0, i;
    for (i = 0; i < argc; ++i) {
        len += strlen(argv[i]) + 1;
    }
    char* res = (char*)malloc(len);
    len = 0;
    for (i = 0; i < argc; ++i) {
        strcpy(res + len, argv[i]);
        len += strlen(argv[i]);
        res[len++] = ' ';
    }
    res[len - 1] = '\0';
    return res;
}

bool expand_path(char** path)
{
    wordexp_t p = { 0 };
    while (strstr(*path, "  ")) {
        *path = (char*)realloc(*path, strlen(*path) + 2);
        char* ptr = strstr(*path, "  ") + 1;
        memmove(ptr + 1, ptr, strlen(ptr) + 1);
        *ptr = '\\';
    }
    if (wordexp(*path, &p, 0) != 0 || p.we_wordv[0] == NULL) {
        wordfree(&p);
        return false;
    }
    free(*path);
    *path = join_args(p.we_wordv, p.we_wordc);
    wordfree(&p);
    return true;
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

int editor_read_key()
{
    int fd = STDIN_FILENO;

    int c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return -1;
    }

    if (c == ESC) {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return ESC;

        /* ESC [ sequences. */
        if (seq[0] == '[') {

            if (seq[1] >= '0' && seq[1] <= '9') {

                // std::cout << seq[0] << seq[1] << std::endl;

                /* Extended escape, read additional byte. */
                if (read(fd, &seq[2], 1) != 1)
                    return ESC;

                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '3':
                        return KEY_DC;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    default:
                        return -1;
                    }
                }

                if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[0], 1) != 1)
                        return ESC;
                    if (read(STDIN_FILENO, &seq[1], 1) != 1)
                        return ESC;

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
                        default:
                            return -1;
                        }
                    }

                    // ctrl +
                    if (seq[0] == '5') {
                        switch (seq[1]) {
                        case 'A':
                            return CTRL_UP;
                        case 'B':
                            return CTRL_DOWN;
                        case 'C':
                            return CTRL_RIGHT;
                        case 'D':
                            return CTRL_LEFT;
                        default:
                            return -1;
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
                        default:
                            return -1;
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
                        default:
                            return -1;
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
                        default:
                            return -1;
                        }
                    }
                }

            } else {
                switch (seq[1]) {
                case 'A':
                    return KEY_UP;
                    ;
                case 'B':
                    return KEY_DOWN;
                    ;
                case 'C':
                    return KEY_RIGHT;
                    ;
                case 'D':
                    return KEY_LEFT;
                    ;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                default:
                    return -1;
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
            default:
                return -1;
            }
        }
    }

    return c;
}