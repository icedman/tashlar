#include "search.h"

#include <iostream>

search_t::search_t()
{
    words = regexp::pattern_t("\\w+", "is");
    // words = regexp::pattern_t("[a-zA-Z0-9]+", "is");
}

search_t::~search_t()
{}

std::vector<search_result_t> search_t::findWords(std::string str)
{
    std::vector<search_result_t> result;

    char *cstr = (char*)str.c_str();
    char *start = cstr;
    char *end = start + str.length();
    
    for( ;start<end; ) {
        regexp::match_t m = regexp::search(words, start, end);
        if (!m.did_match()) {
            break;
        }

        std::string o(start+m.begin(), m.end()-m.begin());

        search_result_t res = {
            .begin = (start + m.begin() - cstr),
            .end = (start + m.end() - cstr),
            .text = o
        };

        result.emplace_back(res);
        
        start += m.begin() + o.length();
        // start ++;
    }

    return result;
}

search_result_t search_t::find(struct cursor_t *cursor, std::string str)
{
    search_result_t res = {
        .begin = 0,
        .end = 0
    };
    
    return res;
}