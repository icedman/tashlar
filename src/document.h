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
    size_t line;
    size_t position;
    size_t absolutePosition;
    int bracket;
    bool open;
    bool unpaired;
};

struct blockcontent_t {
    blockcontent_t()
        : file(0)
        , filePosition(0)
        , dirty(false)
    {}

    std::string text;
    std::ifstream* file;
    size_t filePosition;
    bool dirty;
};

struct blockdata_t {
    blockdata_t();
    ~blockdata_t();

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
 
    struct block_t* next;
    struct block_t* previous; 
 
    std::shared_ptr<blockdata_t> data;
    std::shared_ptr<blockcontent_t> _content;
    size_t _length;

    std::string text();
    void setText(std::string t);
    bool isValid();
    size_t length();
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
    void setCursor(struct cursor_t cursor);
    void updateCursor(struct cursor_t cursor);
    void addCursor(struct cursor_t cursor);
    void clearCursors();
    void clearSelections();

    struct block_t& addBlockAtLineNumber(size_t line);
    struct block_t removeBlockAtLineNumber(size_t line, size_t count = 1);

    void addBufferDocument(const std::string& largeText);
    void insertFromBuffer(struct cursor_t& cursor, std::shared_ptr<document_t> buffer);

    void addSnapshot();
    void undo();
    struct history_t& history() { return snapShots.back(); }

    void update(bool force = false);

    struct block_t nullBlock;
    std::string filePath;
    std::string fileName;
    std::string fullPath;

    std::vector<std::string> tmpPaths;
    std::vector<struct history_t> snapShots;

    bool dirty;
    bool windowsLineEnd;

    size_t blockUid;
    size_t cursorUid;
};

std::vector<struct block_t>::iterator findBlock(std::vector<struct block_t>& blocks, struct block_t& block);

#endif // DOCUMENT_H
