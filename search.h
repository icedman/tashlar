#ifndef SEARCH_H
#define SEARCH_H

#include "pattern.h"

#include <string>
#include <vector>

struct cursor_t;

struct search_result_t {
    int begin;
    int end;
    std::string text;
};

struct search_t
{
public:

    search_t();
    ~search_t();
    
    regexp::pattern_t words;
    regexp::pattern_t word;
    std::string lastWord;
    
    std::vector<search_result_t> findWords(std::string str);

    search_result_t find(struct cursor_t *cursor, std::string str);
};

#endif // SEARCH_H