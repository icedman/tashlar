#ifndef INDEXER_H
#define INDEXER_H

#include <map>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>

#include "editor.h"

#define INDEX_REQUEST_SIZE 256

struct indexer_t {

    indexer_t();
    ~indexer_t();

    void addEntry(block_ptr block, std::string prefix);
    void updateBlock(block_ptr block);
    void indexBlock(block_ptr block);
    void clear();

    std::map<std::string, block_list> indexMap;

    std::vector<std::string> findWords(std::string prefix);

    size_t indexingRequests[INDEX_REQUEST_SIZE];
    size_t requestIdx;

    void run();
    void cancel();
    pthread_t threadId;

    editor_t *editor;
    bool hasInvalidBlocks;
};

#endif // INDEXER_H
