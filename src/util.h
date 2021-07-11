#ifndef UTIL_H
#define UTIL_H

#include <set>
#include <string>
#include <vector>

std::vector<size_t> split_path_to_indices(const std::string& str, const std::set<char> delimiters);
std::vector<std::string> split_path(const std::string& str, const std::set<char> delimiters);
std::vector<std::string> enumerate_dir(const std::string path);

bool expand_path(char** path);

void initLog();
void log(const char* format, ...);

#endif // UTIL_H