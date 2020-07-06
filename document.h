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
    BLOCK_STATE_COMMENT = 1 << 1
};

struct blockdata_t {
    std::vector<span_info_t> spans;
    parse::stack_ptr parser_state;
    std::map<size_t, scope::scope_t> scopes;

    size_t lastPrevBlockRule;
    block_state_e state;
    bool dirty;
};

struct block_t {
public:
    block_t()
        : document(0)
        , file(0)
        , position(0)
        , data(0)
        , dirty(false)
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

    struct document_t* document;
    size_t position;

    std::string content;
    size_t length;

    std::ifstream* file;
    size_t filePosition;

    struct blockdata_t* data;
    struct block_t* next;
    struct block_t* previous;

    bool dirty;
};

struct document_t {
public:
    document_t()
        : file(0)
    {
        struct cursor_t cursor;
        cursor.document = this;
        cursor.position = 0;
        cursors.emplace_back(cursor);
    }

    ~document_t()
    {
        close();
    }

    std::vector<struct block_t> blocks;
    std::vector<struct cursor_t> cursors;

    std::ifstream file;

    bool open(const char* path);
    void close();
    void save();

    struct cursor_t cursor();
    void setCursor(struct cursor_t& cursor);

    void update();
    struct block_t& block(struct cursor_t& cursor);

    struct block_t nullBlock;
    std::string filePath;
    std::string tmpPath;
    std::string fileName;
};

std::vector<struct block_t>::iterator findBlock(std::vector<struct block_t>& blocks, struct block_t& block);
#endif // DOCUMENT_H