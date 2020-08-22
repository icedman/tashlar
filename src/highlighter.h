#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "block.h"
#include "cursor.h"
#include "extension.h"
#include "grammar.h"
#include "theme.h"

#include <pthread.h>
#include <string>
#include <vector>

struct editor_t;
struct highlighter_t {
    language_info_ptr lang;
    theme_ptr theme;

    highlighter_t();

    void gatherBrackets(block_ptr block, char* first, char* last);
    void highlightBlock(block_ptr block);
    void run(editor_t* editor);
    void cancel();

    editor_t* editor;
    pthread_t threadId;
};

span_info_t spanAtBlock(struct blockdata_t* blockData, int pos);

#endif // HIGHLIGHTER_H
