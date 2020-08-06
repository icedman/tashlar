#include "completer.h"
#include "app.h"
#include "search.h"

#include <algorithm>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <string>

completer_t::completer_t()
{}

completer_t::~completer_t()
{
}

void completer_t::addWord(std::string word)
{
    if (word.length() < 3) {
        return;
    }
    
    std::string prefix = word.substr(0, 3);
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c){ return std::tolower(c); });

    string_list& strings = words[prefix];
    if (std::find(strings.begin(), strings.end(), word) == strings.end()) {
        strings.push_back(word);
    }
}
    

std::vector<std::string>& completer_t::findWords(std::string word)
{
    std::vector<std::string> res;
    if (word.length() < 2) {
        return res;
    }

    std::string prefix = word.substr(0, 3);
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c){ return std::tolower(c); });
    
    return words[prefix];
}

void completer_t::addLine(std::string line)
{
    std::vector<search_result_t> result = search_t::instance()->findWords(line);
    for(auto r : result) {
        addWord(r.text);
    }
}
