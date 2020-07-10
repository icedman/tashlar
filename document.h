#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <fstream>
#include <iostream>
#include <istream>
#include <set>
#include <string>
#include <vector>

#include <cstddef>

#include "cursor.h"
#include "grammar.h"

struct document_t;
struct block_t;

struct span_info_t {
    int start;
    int length;
    int red;
    int green;
    int blue;
    int colorIndex;
};

enum block_state_e {
    BLOCK_STATE_UNKNOWN = 0,
    BLOCK_STATE_COMMENT = 1 << 1,
    BLOCK_STATE_STRING = 1 << 2
};

enum cursor_edit_e {
    EDIT_NONE = 0,
    EDIT_INSERT = 1,
    EDIT_DELETE = 2,
    EDIT_SPLIT = 3,
    EDIT_STOP = 100
};

struct blockdata_t {
    blockdata_t()
        : dirty(true)
    {
    }

    std::vector<span_info_t> spans;
    parse::stack_ptr parser_state;
    std::map<size_t, scope::scope_t> scopes;

    size_t lastPrevBlockRule;
    block_state_e state;
    bool dirty;
};

struct block_t {
    block_t()
        : document(0)
        , file(0)
        , position(0)
        , screenLine(0)
        , data(0)
        , dirty(false)
        , next(0)
        , previous()
    {
    }

    std::string text()
    {
        if (dirty) {
            return content;
        }

        std::string text;
        if (file) {
            file->seekg(filePosition, file->beg);
            size_t pos = file->tellg();
            if (std::getline(*file, text)) {
                length = text.length() + 1;
                return text;
            }
        }

        return text;
    }

    void setText(std::string t)
    {
        content = t;
        length = content.length() + 1;
        dirty = true;
        if (data) {
            data->dirty = true;
        }
    }

    bool isValid()
    {
        if (document == 0) {
            return false;
        }
        return true;
    }

    void update()
    {
        if (content.length()) {
            length = content.length() + 1;
        }
    }

    int lineNumber;
    int lineCount;
    
    struct document_t* document;
    size_t position;
    size_t screenLine;

    std::string content;
    size_t length;

    std::ifstream* file;
    size_t filePosition;

    struct blockdata_t* data;
    struct block_t* next;
    struct block_t* previous;

    bool dirty;
};

struct cursor_edit_t {
    struct cursor_t cursor;
    std::string text;
    int count;
    cursor_edit_e edit;
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

    void begin();
    void end();
    void mark();

    void addInsert(struct cursor_t& c, std::string text);
    void addDelete(struct cursor_t& c, int count);
    void addSplit(struct cursor_t& c);
    void replay();

    int frames;
    bool paused;
};

struct document_t {
    document_t()
        : file(0)
    {}

    ~document_t()
    {
        close();
    }

    std::vector<struct block_t> blocks;
    std::vector<struct cursor_t> cursors;

    std::ifstream file;
    std::vector<std::shared_ptr<document_t>> buffers;

    bool open(const char* path);
    void close();
    void save();

    struct cursor_t cursor();
    void setCursor(struct cursor_t& cursor);
    void updateCursor(struct cursor_t& cursor);
    void addCursor(struct cursor_t& cursor);
    void clearCursors();
    void clearSelections();

    void addBufferDocument(const std::string &largeText);
    void insertLastBuffer(struct cursor_t& cursor);

    void undo();
    void update();
    struct block_t& block(struct cursor_t& cursor, bool skipCache = false);

    struct block_t nullBlock;
    std::string filePath;
    std::string fileName;

    std::vector<std::string> tmpPaths;

    std::map<size_t, struct block_t&> cursorBlockCache;
    struct history_t history;
};

std::vector<struct block_t>::iterator findBlock(std::vector<struct block_t>& blocks, struct block_t& block);
#endif // DOCUMENT_H
