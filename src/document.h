#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <fstream>
#include <iostream>
#include <istream>
#include <set>
#include <string>
#include <vector>

#include <cstddef>

#include "block.h"
#include "cursor.h"
#include "grammar.h"
#include "history.h"

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
