#ifndef COMPLETER_H
#define COMPLETER_H

#include <map>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>

struct editor_t;
typedef std::vector<std::string> string_list;

struct completer_t {

    completer_t();
    ~completer_t();

    std::map<std::string, string_list> words;

    void addWord(std::string word);
    void addLine(std::string line);

    std::vector<std::string>& findWords(std::string prefix);

    void run(editor_t* editor);
    void cancel();

    editor_t* editor;
    pthread_t threadId;
};

#endif // COMPLETER_H
