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
        // std::cout << start << "??";
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

        // std::cout << o << ":" << o.length() << "  ";
        
        result.emplace_back(res);
        
        start += m.begin() + o.length();
        // start ++;
    }

    return result;
}
