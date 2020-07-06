#include "document.h"
#include "cursor.h"
#include "util.h"

static size_t cursor_uid = 1;

std::vector<struct block_t>::iterator findBlock(std::vector<struct block_t>& blocks, struct block_t& block)
{
    std::vector<struct block_t>::iterator it = blocks.begin();
    while (it != blocks.end()) {
        struct block_t& blk = *it;
        if (&blk == &block) {
            return it;
        }
        it++;
    }
    return it;
}

void history_t::mark() {
    if (editBatch.size()) {
        edits.push_back(editBatch);
        editBatch.clear();
    }
}

void history_t::addInsert(struct cursor_t& cur, std::string t)
{
    editBatch.push_back({ .cursor = cur,
        .text = t,
        .edit = cursor_edit_e::EDIT_INSERT
    });
}

void history_t::addDelete(struct cursor_t& cur, int c)
{
    editBatch.push_back({ .cursor = cur,
        .count = c,
        .edit = cursor_edit_e::EDIT_DELETE
    });
}

void history_t::addSplit(struct cursor_t& cur)
{
    editBatch.push_back({ .cursor = cur,
        .edit = cursor_edit_e::EDIT_SPLIT
    });
}

void history_t::replay()
{
    if (!edits.size()) {
        return;
    }
    
    edit_batch_t last = edits.back();
    edits.pop_back();
    
    for (auto batch : edits) {
        for(auto e : batch) {
            switch (e.edit) {           
                
            case cursor_edit_e::EDIT_INSERT:
                cursorInsertText(&e.cursor, e.text);
                break;
            case cursor_edit_e::EDIT_DELETE:
                cursorEraseText(&e.cursor, e.count);
                break;
            case cursor_edit_e::EDIT_SPLIT:
                cursorSplitBlock(&e.cursor);
                break;
            }

            e.cursor.document->update();
        }
    }

    for(auto e : last) {
        e.cursor.document->setCursor(e.cursor);
        break;
    }
}

bool document_t::open(const char* path)
{
    bool useStreamBuffer = true;

    filePath = path;
    tmpPath = "/tmp/tmpfile.XXXXXX";
    mkstemp((char*)tmpPath.c_str());

    std::cout << tmpPath << std::endl;

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

        b.setText(line);
    }

    tmp.close();

    // reopen from tmp
    file = std::ifstream(tmpPath, std::ifstream::in);
    update();

    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = split_path(filePath, delims);
    fileName = spath.back();

    history.initialState = blocks;
    return true;
}

void document_t::undo()
{
    history.mark();
    if (!history.edits.size()) {
        return;
    }
    
    cursorBlockCache.clear();
    clearCursors();
      
    blocks = history.initialState;
    update();

    history.replay();

    update();
}

void document_t::close()
{
    file.close();
    std::cout << "unlink " << tmpPath;
    remove(tmpPath.c_str());
}

void document_t::save()
{
    std::ofstream tmp(filePath, std::ofstream::out);
    for (auto b : blocks) {
        std::string text = b.text();
        tmp << text << std::endl;
    }
}

struct cursor_t document_t::cursor()
{        
    return cursors[0];
}

void document_t::setCursor(struct cursor_t& cursor)
{
    if (!cursors[0].uid) {
        cursors[0].uid = cursor_uid++;
    }
    cursors[0].position = cursor.position;
    cursors[0].anchorPosition = cursor.anchorPosition;
    cursors[0].update();
}

void document_t::updateCursor(struct cursor_t& cursor)
{
    for(auto &c : cursors) {
        if (c.uid == cursor.uid) {
            c.position = cursor.position;
            c.anchorPosition = cursor.anchorPosition;
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
    cursors.clear();
    cursors.push_back(cursor);
}
    
void document_t::update()
{
    if (!cursors.size()) {
        struct cursor_t cursor;
        cursor.document = this;
        cursor.position = 0;
        cursors.emplace_back(cursor);
    }

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
        b.update();
        b.position = pos;
        b.lineNumber = l++;
        pos += b.length;
        prev = &b;
    }

    cursorBlockCache.clear();
}

struct block_t& document_t::block(struct cursor_t& cursor, bool skipCache)
{
    // TODO: This is used all over.. perpetually improve search (divide-conquer? index based?)!

    if (!blocks.size()) {
        return nullBlock;
    }

    std::map<size_t, struct block_t &>::iterator it = cursorBlockCache.find(cursor.position);
    if (!skipCache && it != cursorBlockCache.end()) {
        return it->second;
    }

    std::vector<struct block_t>::iterator bit = blocks.begin();
    size_t idx = 0;

    while (bit != blocks.end()) {
        auto& b = *bit;
        if (b.length == 0 && cursor.position == b.position) {
            if (!skipCache) { cursorBlockCache.emplace(cursor.position, b); }
            return b;
        }
        if (b.position <= cursor.position && cursor.position < b.position + b.length) {
            if (!skipCache) { cursorBlockCache.emplace(cursor.position, b); }
            return b;
        }
        idx++;
        bit++;
    }

    return nullBlock;
}