#include <cstring>
#include <stdio.h>

#include "app.h"
#include "cursor.h"
#include "document.h"
#include "editor.h"
#include "util.h"

static size_t cursor_uid = 1;

block_t::block_t()
        : document(0)
        , file(0)
        , lineNumber(0)
        , position(0)
        , screenLine(0)
        , dirty(false)
        , next(0)
        , previous()
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
    int skip = (block.lineNumber > 0 ? (block.lineNumber * 4 / 5) : 0) + 1;
    it += skip;
    while (it != blocks.end()) {
        struct block_t& blk = *it;
        if (&blk == &block) {
            return it;
        }
        it++;
    }
    return it;
}

bool document_t::open(const char* path)
{
    bool useStreamBuffer = true;

    filePath = path;
    std::string tmpPath = "/tmp/tmpfile.XXXXXX";
    mkstemp((char*)tmpPath.c_str());

    app_t::instance()->log(tmpPath.c_str());

    file = std::ifstream(path, std::ifstream::in);
    std::ofstream tmp(tmpPath, std::ofstream::out);

    std::string line;
    size_t pos = file.tellg();
    while (std::getline(file, line)) {
        struct block_t b;
        b.document = this;
        b.file = &file;
        b.position = pos;
        b.filePosition = pos;
        b.length = line.length() + 1;
        blocks.emplace_back(b);
        pos = file.tellg();
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
        struct block_t b;
        b.document = this;
        b.position = 0;
        b.filePosition = 0;
        b.length = 1;
        blocks.emplace_back(b);
    }

    // reopen from tmp
    file = std::ifstream(tmpPath, std::ifstream::in);
    update();

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

void document_t::addSnapshot()
{
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

    clearCursors();

    blocks = _history.initialState;
    update();

    _history.replay();
    update();

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
    if (runOn) {
        return;
    }

    std::ofstream tmp(filePath, std::ofstream::out);
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
        cursors[0].uid = cursor_uid++;
    }

    cursor.uid = cursors[0].uid;
    updateCursor(cursor);
}

void document_t::updateCursor(struct cursor_t& cursor)
{
    for (auto& c : cursors) {
        if (c.uid == cursor.uid) {
            c._block = cursor.block();
            c._position = cursor.position();
            c._anchorPosition = cursor.anchorPosition();
            c._preferredRelativePosition = cursor.preferredRelativePosition();
            c.update();
            break;
        }
    }
}

void document_t::addCursor(struct cursor_t& cursor)
{
    struct cursor_t cur = cursor;
    cur.uid = cursor_uid++;
    cursors.push_back(cur);
    // std::cout << "add cursor " << cursor.position << std::endl;
}

void document_t::clearCursors()
{
    if (!cursors.size()) {
        return;
    }

    struct cursor_t cursor = cursors[0];
    cursor.clearSelection();
    cursors.clear();
    cursors.push_back(cursor);
}

void document_t::clearSelections()
{
    for (auto& c : cursors) {
        c.clearSelection();
    }
}

void document_t::update()
{
    if (!dirty) {
        return;
    }

    static int updates = 0;
    app_t::instance()->log("update %d", updates++);
    dirty = false;
    
    // TODO: This is used all over.. perpetually improve (update only changed)
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
}

struct block_t& document_t::block(struct cursor_t& cursor)
{
    // TODO: This is used all over.. perpetually improve search (divide-conquer? index based?)!
    app_t::instance()->log("block query");
    
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
        if (b.position <= cursor.position() && cursor.position() < b.position + b.length) {
            return b;
        }
        idx++;
        bit++;
    }

    return nullBlock;
}

void document_t::addBufferDocument(const std::string& largeText)
{
    std::string tmpPath = "/tmp/tmpfile.paste.XXXXXX";
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
    for (auto bb : buffer->blocks) {
        struct block_t b;
        b.document = buffer.get();
        b.file = &buffer->file;
        b.position = bb.position;
        b.filePosition = bb.position;
        b.length = bb.length;
        blocks.emplace_back(b);
    }
    update();
}
