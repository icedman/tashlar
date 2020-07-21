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
#include "history.h"

struct document_t;
struct block_t;

enum block_state_e {
    BLOCK_STATE_UNKNOWN = 0,
    BLOCK_STATE_COMMENT = 1 << 4,
    BLOCK_STATE_STRING = 1 << 5
};

struct span_info_t {
    int start;
    int length;
    int colorIndex;
    block_state_e state;
};

struct bracket_info_t {
    long line;
    long position;
    long absolutePosition;
    int bracket;
    bool open;
    bool unpaired;
};

struct blockdata_t {
    blockdata_t()
        : dirty(true)
        , folded(false)
        , foldable(false)
        , foldedBy(0)
        , dots(0)
        , indent(0)
        , lastPrevBlockRule(0)
    {
    }

    ~blockdata_t()
    {
        if (dots) {
            free(dots);
        }
    }

    std::vector<span_info_t> spans;
    std::vector<bracket_info_t> foldingBrackets;
    std::vector<bracket_info_t> brackets;
    parse::stack_ptr parser_state;
    std::map<size_t, scope::scope_t> scopes;

    int* dots;

    size_t lastRule;
    size_t lastPrevBlockRule;
    block_state_e state;

    bool dirty;
    bool folded;
    bool foldable;
    size_t foldedBy;
    int indent;
};

struct block_t {
    block_t();
    ~block_t();

    size_t uid;
    size_t lineNumber;
    size_t originalLineNumber;
    size_t lineCount;

    struct document_t* document;
    size_t position;
    size_t screenLine;
    size_t renderedLine;

    std::string content;
    size_t length;

    std::ifstream* file;
    size_t filePosition;

    struct block_t* next;
    struct block_t* previous;

    bool dirty;

    std::shared_ptr<blockdata_t> data;

    std::string text();
    void setText(std::string t);
    bool isValid();
};

struct document_t {

    document_t();
    ~document_t();

    std::vector<struct block_t> blocks;
    std::vector<struct cursor_t> cursors;

    std::ifstream file;
    std::vector<std::shared_ptr<document_t>> buffers;

    bool open(const char* path);
    void close();
    void save();
    void saveAs(const char* path, bool replacePath = false);

    struct cursor_t cursor();
    void setCursor(struct cursor_t& cursor);
    void updateCursor(struct cursor_t& cursor);
    void addCursor(struct cursor_t& cursor);
    void clearCursors();
    void clearSelections();

    struct block_t& addBlockAtLineNumber(size_t line);
    struct block_t& removeBlockAtLineNumber(size_t line, size_t count = 1);

    void addBufferDocument(const std::string& largeText);
    void insertFromBuffer(struct cursor_t& cursor, std::shared_ptr<document_t> buffer);

    void addSnapshot();
    void undo();
    struct history_t& history() { return snapShots.back(); }

    void update(bool force = false);

    struct block_t& block(struct cursor_t& cursor);

    struct block_t nullBlock;
    std::string filePath;
    std::string fileName;
    std::string fullPath;

    std::vector<std::string> tmpPaths;

    std::vector<struct history_t> snapShots;

    bool runOn;
    bool dirty;

    size_t blockUid;
    size_t cursorUid;
};

std::vector<struct block_t>::iterator findBlock(std::vector<struct block_t>& blocks, struct block_t& block);

#endif // DOCUMENT_H
