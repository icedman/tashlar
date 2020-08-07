#ifndef COMPLETER_H
#define COMPLETER_H

#include <map>
#include <queue>
#include <string>
#include <vector>
// #include <pthread.h>

typedef std::vector<std::string> string_list;

// #define MAX_COMPLETER_THREADS 32

// struct completer_thread_info {
// completer_thread_info() : state(0), id(0) {}
// std::string line;
// int id;
// int state;
// string_list result;
// pthread_t thread;
// };

struct completer_t {

    completer_t();
    ~completer_t();

    std::map<std::string, string_list> words;

    void addWord(std::string word);
    void addLine(std::string line);

    std::vector<std::string>& findWords(std::string prefix);
};

#endif // COMPLETER_H