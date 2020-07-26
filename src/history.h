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
    EDIT_SPLIT,
    EDIT_DELETE,
    EDIT_DELETE_SELECTION,
    EDIT_DELETE_BLOCK, 
    EDIT_PASTE_BUFFER,
    EDIT_BLOCK_SNAPSHOT,
    EDIT_STOP
};

struct cursor_edit_t {
    size_t blockUid;
    struct cursor_t cursor;
    size_t blockEndUid;
    struct cursor_t cursorEnd;
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
        , inReplay(false)
        , didMark(false)
    {
    }

    std::vector<struct block_t> initialState;

    std::vector<edit_batch_t> edits;
    edit_batch_t editBatch;

    void initialize(struct document_t* document);

    void begin();
    void end();
    void mark();

    void _addInsert(struct cursor_t& c, std::string text);
    void _addSplit(struct cursor_t& c);
    void _addDelete(struct cursor_t& c, int count);
    void _addDeleteSelection(struct cursor_t& cur, struct cursor_t& curEnd);
    void _addDeleteBlock(struct cursor_t& cur, size_t blockUid, size_t count); 
    void _addBlockSnapshot(struct cursor_t& c);
    void addPasteBuffer(struct cursor_t& c, std::shared_ptr<document_t> buffer);
    void replay();

    int frames;
    bool paused;
    bool inReplay;
    bool didMark;
};

#endif // HISTORY_H
