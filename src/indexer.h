#ifndef INDEXER_H
#define INDEXER_H

#include <map>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>

#include "editor.h"

struct indexer_t {

    indexer_t();
    ~indexer_t();

    void addEntry(block_ptr block, std::string prefix);
    void updateBlock(block_ptr block);
    void clear();

    std::map<std::string, block_list> indexMap;

    std::vector<std::string> findWords(std::string prefix);
};

#endif // INDEXER_H
