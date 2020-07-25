#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <wordexp.h>

#include <iostream>

#include "util.h"

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
