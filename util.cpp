#include "util.h"
#include <dirent.h>

#include <iostream>

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
