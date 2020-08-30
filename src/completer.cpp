#include "completer.h"
#include "app.h"
#include "editor.h"
#include "search.h"

#include <algorithm>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <string>

completer_t::completer_t()
    : threadId(0)
{
}

completer_t::~completer_t()
{
}

void completer_t::addWord(std::string word)
{
    if (word.length() < 3) {
        return;
    }

    std::string prefix = word.substr(0, 3);
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return std::tolower(c); });

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
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return std::tolower(c); });

    return words[prefix];
}

void completer_t::addLine(std::string line)
{
    std::vector<search_result_t> result = search_t::instance()->findWords(line);
    for (auto r : result) {
        addWord(r.text);
    }
}

void* completeThread(void* arg)
{
    completer_t* completer = (completer_t*)arg;
    editor_t* editor = completer->editor;

    completer_t tmp;

    // std::ifstream file = std::ifstream(editor->document.tmpPaths[0], std::ifstream::in);
    std::ifstream file = std::ifstream(editor->document.filePath, std::ifstream::in);
    std::string line;

    while (std::getline(file, line)) {
        tmp.addLine(line);
    }

    // mutex? todo!
    completer->words = tmp.words;
    completer->threadId = 0;
    return NULL;
}

void completer_t::run(editor_t* editor)
{
    this->editor = editor;
    pthread_create(&threadId, NULL, &completeThread, this);
}

void completer_t::cancel()
{
    if (threadId) {
        pthread_cancel(threadId);
        threadId = 0;
    }
}
