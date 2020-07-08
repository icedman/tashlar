#include "search.h"

#include <iostream>

search_t::search_t()
{
    words = regexp::pattern_t("\\w+", "is");
    // words = regexp::pattern_t("[a-zA-Z0-9]+", "is");
}

search_t::~search_t()
{}

std::vector<search_result_t> search_t::findWords(std::string str, regexp::pattern_t *pattern)
{
    if (!pattern) {
        pattern = &words;
    }
    
    std::vector<search_result_t> result;

    char *cstr = (char*)str.c_str();
    char *start = cstr;
    char *end = start + str.length();
    
    for( ;start<end; ) {
        regexp::match_t m = regexp::search(*pattern, start, end);
        if (!m.did_match()) {
            break;
        }

        std::string o(start+m.begin(), m.end()-m.begin());

        search_result_t res;
        res.begin = (start + m.begin() - cstr);
        res.end = (start + m.end() - cstr);
        res.text = o;

        result.emplace_back(res);
        
        start += m.begin() + o.length();
        // start ++;
    }

    return result;
}

std::vector<search_result_t> search_t::find(std::string str, std::string pat)
{
    if (lastWord != pat) {
        lastWord = pat;
        word = regexp::pattern_t(pat, "is");
    }

    return findWords(str, &word);
}