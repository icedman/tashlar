#ifndef SEARCH_H
#define SEARCH_H

#include "pattern.h"

#include <string>
#include <vector>

struct cursor_t;

struct search_result_t {
    search_result_t()
        : begin(0)
        , end(0)
    {
    }

    int begin;
    int end;
    std::string text;

    bool isValid() { return begin != end; }
};

struct search_t {
    search_t();
    ~search_t();

    regexp::pattern_t words;
    regexp::pattern_t word;
    std::string lastWord;

    std::vector<search_result_t> findWords(std::string str, regexp::pattern_t* pattern = NULL);
    std::vector<search_result_t> find(std::string str, std::string word);
};

#endif // SEARCH_H