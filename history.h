#ifndef HISTORY_H
#define HISTORY_H

#include <memory>
#include <string>
#include <vector>

#include "cursor.h"

struct document_t;
struct block_t;
struct cursor_t;

enum cursor_edit_e {
    EDIT_NONE = 0,
    EDIT_INSERT = 100,
    EDIT_DELETE,
    EDIT_SPLIT,
    EDIT_PASTE_BUFFER,
    EDIT_BLOCK_SNAPSHOT,
    EDIT_STOP
};

struct cursor_edit_t {
    struct cursor_t cursor;
    std::string text;
    int count;
    cursor_edit_e edit;
    std::shared_ptr<document_t> buffer;
};

typedef std::vector<cursor_edit_t> edit_batch_t;

struct history_t {
    history_t()
        : frames(0)
        , paused(false)
    {
    }

    std::vector<struct block_t> initialState;
    // std::vector<struct cursor_edit_t> edits;

    std::vector<edit_batch_t> edits;
    edit_batch_t editBatch;

    void initialize(struct document_t* document);

    void begin();
    void end();
    void mark();

    void addInsert(struct cursor_t& c, std::string text);
    void addDelete(struct cursor_t& c, int count);
    void addSplit(struct cursor_t& c);
    void addBlockSnapshot(struct cursor_t& c);
    void addPasteBuffer(struct cursor_t& c, std::shared_ptr<document_t> buffer);
    void replay();

    int frames;
    bool paused;
};

#endif // HISTORY_H
