#ifndef SEARCH_H
#define SEARCH_H

#include <string>
#include <vector>

#include "pattern.h"

struct cursor_t;
struct document_t;

struct search_result_t {
    search_result_t()
        : begin(0)
        , end(0)
    {
    }

    size_t begin;
    size_t end;
    std::string text;

    bool isValid() { return begin != end; }
};

struct search_t {

    enum {
        SEARCH_FORWARD = 0,
        SEARCH_BACKWARD = 1
    };

    search_t();
    ~search_t();

    static search_t* instance();

    regexp::pattern_t words;
    regexp::pattern_t word;
    std::string lastWord;

    std::vector<search_result_t> findWords(std::string str, regexp::pattern_t* pattern = NULL);
    std::vector<search_result_t> find(std::string str, std::string word);
    std::vector<search_result_t> findCompletion(document_t* document, std::string str);
};

int levenshtein_distance(char* s1, char* s2);

#endif // SEARCH_H
