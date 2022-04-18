#include "search.h"
#include "document.h"

#include <cstring>

static struct search_t* searchInstance = 0;

// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int levenshtein_distance(char* s1, char* s2)
{
    unsigned int s1len, s2len, x, y, lastdiag, olddiag;
    s1len = strlen(s1);
    s2len = strlen(s2);
    unsigned int column[s1len + 1];
    for (y = 1; y <= s1len; y++)
        column[y] = y;
    for (x = 1; x <= s2len; x++) {
        column[0] = x;
        for (y = 1, lastdiag = x - 1; y <= s1len; y++) {
            olddiag = column[y];
            column[y] = MIN3(column[y] + 1, column[y - 1] + 1, lastdiag + (s1[y - 1] == s2[x - 1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
    return (column[s1len]);
}

struct search_t* search_t::instance()
{
    return searchInstance;
}

search_t::search_t()
{
    searchInstance = this;
    // words = regexp::pattern_t("\\w+", "is");
    words = regexp::pattern_t("[a-zA-Z0-9_']+", "is");
}

search_t::~search_t()
{
}

std::vector<search_result_t> search_t::findWords(std::string str, regexp::pattern_t* pattern)
{
    if (!pattern) {
        pattern = &words;
    }

    std::vector<search_result_t> result;
    if (!str.length()) {
        return result;
    }

    for (int i = 0; i < str.length(); i++) {
        if (str[i] == '(' || str[i] == ')') {
            str[i] = '.';
        }
    }

    char* cstr = (char*)str.c_str();
    char* start = cstr;
    char* end = start + str.length();

    for (; start < end;) {
        regexp::match_t m = regexp::search(*pattern, start, end);
        if (!m.did_match()) {
            break;
        }

        // log("%d %d", m.begin(), m.end());

        std::string o(start + m.begin(), m.end() - m.begin());

        search_result_t res;
        res.begin = (start + m.begin() - cstr);
        res.end = (start + m.end() - cstr);
        res.text = o;

        result.emplace_back(res);
        start += m.begin() + o.length();
    }

    return result;
}

std::vector<search_result_t> search_t::find(std::string str, std::string pat)
{
    if (lastWord != pat) {
        lastWord = pat;

        // cleanup - non-regex
        for (int i = 0; i < pat.length(); i++) {
            if (pat[i] == '(' || pat[i] == ')' || pat[i] == '[' || pat[i] == ']' ||
                pat[i] == '/' || pat[i] == '*' || pat[i] == '{' || pat[i] == '}') {
                pat[i] = '.';
            }
        }

        word = regexp::pattern_t(pat, "is");
    }

    return findWords(str, &word);
}

std::vector<search_result_t> search_t::findCompletion(document_t* document, std::string str)
{
    std::string pat = "\\b";
    pat += str;
    pat += "[a-zA-Z_]+";
    word = regexp::pattern_t(pat, "is");
    lastWord = pat;

    std::vector<search_result_t> result;
    document_t* doc = document;
    cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

    int skipWatch = 0;
    for (auto b : doc->blocks) {

        /*
        if (b.data && b.data->state == BLOCK_STATE_COMMENT) {
            continue;
        }

        // search only words within the first 1000 lines
        if (!b.data && skipWatch++ > 1000) {
            break;
        }
        */

        std::vector<search_result_t> res = findWords(b->text(), &word);
        for (int j = 0; j < res.size(); j++) {
            auto r = res[j];
            bool duplicate = false;
            for (int i = 0; i < result.size(); i++) {
                if (result[i].text == r.text) {
                    duplicate = true;
                    break;
                }
            }

            if (duplicate) {
                continue;
            }

            result.push_back(r);
            if (result.size() > 40)
                break;
        }
    }

    return result;
}
