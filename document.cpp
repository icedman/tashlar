#include <cstring>
#include <stdio.h>

#include "app.h"
#include "cursor.h"
#include "document.h"
#include "editor.h"
#include "util.h"

#define TMP_BUFFER "/tmp/tmpfile.XXXXXX"
#define TMP_PASTE "/tmp/tmpfile.paste.XXXXXX"

#define WINDOWS_LINE_END "\r\n"
#define LINUX_LINE_END "\n"

static size_t blocksCount = 0;

block_t::block_t()
    : uid(0)
    , document(0)
    , file(0)
    , lineNumber(0)
    , lineCount(0)
    , position(0)
    , screenLine(0)
    , dirty(false)
    , next(0)
    , previous(0)
{
}

block_t::~block_t()
{
}

std::string block_t::text()
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

void block_t::setText(std::string t)
{
    content = t;
    length = content.length() + 1;
    dirty = true;
    if (data) {
        data->dirty = true;
    }

    if (document) {
        document->dirty = true;
    }
}

bool block_t::isValid()
{
    if (document == 0) {
        return false;
    }
    return true;
}

std::vector<struct block_t>::iterator findBlock(std::vector<struct block_t>& blocks, struct block_t& block)
{
    std::vector<struct block_t>::iterator it = blocks.begin();
    while (it != blocks.end()) {
        struct block_t& blk = *it;
        if (blk.uid == block.uid) {
            return it;
        }
        it++;
    }
    return it;
}

document_t::document_t()
    : file(0)
    , blockUid(1)
    , cursorUid(1)
    , dirty(true)
    , runOn(false)
    , windowsLineEnd(false)
{
}

document_t::~document_t()
{
    close();
}

bool document_t::open(const char* path)
{
    bool useStreamBuffer = true;

    filePath = path;
    std::string tmpPath = TMP_BUFFER;
    mkstemp((char*)tmpPath.c_str());

    app_t::instance()->log(tmpPath.c_str());

    file = std::ifstream(path, std::ifstream::in);
    std::ofstream tmp(tmpPath, std::ofstream::out);

    std::string line;
    size_t offset = 0;
    size_t pos = file.tellg();
    size_t lineNo = 0;
    while (std::getline(file, line)) {
        struct block_t b;
        b.document = this;
        b.file = &file;
        b.position = pos;
        b.originalLineNumber = lineNo++;
        b.filePosition = pos;

        if (line.length() && line[line.length()-1] == '\r') {
            line.pop_back();
            offset++;
            windowsLineEnd = true;
        }

        b.length = line.length() + 1;
        blocks.emplace_back(b);
        pos = file.tellg();
        pos -= offset;
        tmp << line << std::endl;
        if (b.length > 2000) {
            runOn = true;
        }
    }

    tmp.close();

    // if (runOn) {
    // blocks.clear();
    // }

    if (!blocks.size()) {
        addBlockAtLineNumber(0);
    }

    // reopen from tmp
    file = std::ifstream(tmpPath, std::ifstream::in);
    update(true);

    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = split_path(filePath, delims);
    fileName = spath.back();

    std::string _path = filePath;
    char* cpath = (char*)malloc(_path.length() + 1 * sizeof(char));
    strcpy(cpath, _path.c_str());
    expand_path((char**)(&cpath));
    fullPath = std::string(cpath);
    free(cpath);

    tmpPaths.push_back(tmpPath);

    addSnapshot();
    return true;
}

struct block_t& document_t::addBlockAtLineNumber(size_t line)
{
    std::vector<struct block_t>::iterator it = blocks.begin();
    if (line > 0) {
        if (line < blocks.size()) {
            it += line;
        } else {
            it = blocks.end();
        }
    }

    struct block_t b;
    b.document = this;
    b.setText("");
    if (it == blocks.end()) {
        blocks.emplace_back(b);
        return blocks.back();
    }

    return *blocks.emplace(it, b);
}

struct block_t& document_t::removeBlockAtLineNumber(size_t line, size_t count)
{
    if (blocks.size() < 2) {
        return nullBlock;
    }

    std::vector<struct block_t>::iterator it = blocks.begin();
    if (line > 0) {
        if (line >= blocks.size()) {
            line = blocks.size() - 1;
        }
        it += line;
    }

    if (it == blocks.end()) {
        return nullBlock;
    }

    struct block_t& block = *it;
    blocks.erase(it, it + count);
    return block;
}

void document_t::addSnapshot()
{
    // app_t::instance()->log("addSnapshot");

    // discard any unmarked edits
    if (snapShots.size()) {
        snapShots.back().editBatch.clear();
    }

    struct history_t _history;
    _history.initialize(this);
    snapShots.emplace_back(_history);
}

void document_t::undo()
{
    if (snapShots.size() > 1 && history().edits.size() == 0) {
        snapShots.pop_back();
    }

    struct history_t& _history = history();
    _history.mark();

    blocks = history().initialState;
    update(true);
    clearCursors();

    _history.replay();
    update(true);

    clearSelections();
}

void document_t::close()
{
    file.close();

    app_t::instance()->log("file closed %s", filePath.c_str());
    for (auto tmpPath : tmpPaths) {
        app_t::instance()->log("temporary file freed %s", tmpPath.c_str());
        remove(tmpPath.c_str());
    }
}

void document_t::save()
{
    std::string lineEnd = windowsLineEnd ? WINDOWS_LINE_END : LINUX_LINE_END;
    std::ofstream tmp(filePath, std::ofstream::out);
    for (auto b : blocks) {
        std::string text = b.text();
        tmp << text << lineEnd;
    }
}

void document_t::saveAs(const char* path, bool replacePath)
{
    std::ofstream tmp(path, std::ofstream::out);
    for (auto b : blocks) {
        std::string text = b.text();
        tmp << text << std::endl;
    }
}

struct cursor_t document_t::cursor()
{
    cursors[0].update();
    if (cursors[0].isNull()) {
        cursorSetPosition(&cursors[0], 0);
    }
    return cursors[0];
}

void document_t::setCursor(struct cursor_t& cursor)
{
    if (!cursors[0].uid) {
        cursors[0].uid = cursorUid++;
    }

    cursor.uid = cursors[0].uid;
    updateCursor(cursor);
}

void document_t::updateCursor(struct cursor_t& cursor)
{
    for (auto& c : cursors) {
        if (c.uid == cursor.uid) {
            c._position = cursor.position();
            c._anchorPosition = cursor.anchorPosition();
            c._preferredRelativePosition = cursor.preferredRelativePosition();
            c._block = cursor._block;
            c.update();
            // app_t::instance()->log("updateCursor %d", c.position());
            break;
        }
    }
}

void document_t::addCursor(struct cursor_t& cursor)
{
    struct cursor_t cur = cursor;
    cur.uid = cursorUid++;
    cursors.push_back(cur);
    // std::cout << "add cursor " << cursor.position << std::endl;
}

void document_t::clearCursors()
{
    if (!cursors.size()) {
        return;
    }

    struct cursor_t cursor = cursors[0];
    cursor._block = 0;
    cursor.clearSelection();
    cursor.update();

    cursors.clear();
    cursors.push_back(cursor);
}

void document_t::clearSelections()
{
    for (auto& c : cursors) {
        c.clearSelection();
    }
}

void document_t::update(bool force)
{
    // TODO: This is used all over.. perpetually improve (update only changed)
    if (!dirty && !force) {
        return;
    }

    app_t::instance()->log("document update %d", blockUid);
    dirty = false;

    // update block positions
    struct block_t* prev = NULL;
    int l = 0;
    size_t pos = 0;
    for (auto& b : blocks) {
        if (prev) {
            prev->next = &b;
        }
        b.previous = prev;
        b.next = NULL;

        if (b.content.length()) {
            b.length = b.content.length() + 1;
        }

        b.position = pos;
        b.lineNumber = l++;
        b.screenLine = 0;

        if (!b.uid) {
            b.uid += blockUid++;
        }

        pos += b.length;
        prev = &b;
    }

    if (!cursors.size()) {
        struct cursor_t cursor;
        cursor._document = this;
        cursor._position = 0;
        cursor._anchorPosition = 0;
        cursor.update();
        cursors.emplace_back(cursor);
    }

    app_t::instance()->log("cursor: %d block: %d/%d", cursors[0].position(), cursors[0].block()->lineNumber, blocks.size());
}

struct block_t& document_t::block(struct cursor_t& cursor)
{
    // TODO: This is used all over.. perpetually improve search (divide-conquer? index based?)!
    // app_t::instance()->log("block query");

    if (!blocks.size()) {
        return nullBlock;
    }

    std::vector<struct block_t>::iterator bit = blocks.begin();
    size_t idx = 0;

    while (bit != blocks.end()) {
        auto& b = *bit;
        if (b.length == 0 && cursor.position() == b.position) {
            return b;
        }
        if (b.position <= cursor.position() && cursor.position() < b.position + (b.length ? b.length : 1)) {
            return b;
        }
        idx++;
        bit++;
    }

    // app_t::instance()->log("null block returned");
    return nullBlock;
}

void document_t::addBufferDocument(const std::string& largeText)
{
    std::string tmpPath = TMP_PASTE;
    mkstemp((char*)tmpPath.c_str());

    app_t::instance()->log(tmpPath.c_str());

    std::ofstream tmp(tmpPath, std::ofstream::out);
    tmp << largeText;
    tmp.close();

    std::shared_ptr<document_t> doc = std::make_shared<document_t>();
    doc->open(tmpPath.c_str());
    buffers.emplace_back(doc);

    tmpPaths.push_back(tmpPath);
}

void document_t::insertFromBuffer(struct cursor_t& cursor, std::shared_ptr<document_t> buffer)
{
    std::vector<struct block_t> bufferBlocks;
    size_t ln = cursor.block()->lineNumber;
    for (auto bb : buffer->blocks) {
        //struct block_t& b = addBlockAtLineNumber(ln++); // too slow
        struct block_t b;
        b.document = buffer.get();
        b.file = &buffer->file;
        b.filePosition = bb.position;
        b.length = bb.length;
        bufferBlocks.push_back(b);
    }
    blocks.insert(blocks.begin()+ln, make_move_iterator(bufferBlocks.begin()), make_move_iterator(bufferBlocks.end()));
    update(true);
}


