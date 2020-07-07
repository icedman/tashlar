#ifndef SEARCH_H
#define SEARCH_H

#include "pattern.h"

#include <string>
#include <vector>

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

    std::vector<search_result_t> findWords(std::string str);
};

#endif // SEARCH_H