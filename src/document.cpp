#include "document.h"
#include "cursor.h"
#include "search.h"
#include "util.h"
#include "app.h"

#include "indexer.h"

#include <cstring>
#include <set>

#define TMP_BUFFER "/tmp/tmpfile.XXXXXX"
#define TMP_PASTE "/tmp/tmpfile.paste.XXXXXX"

#define WINDOWS_LINE_END "\r\n"
#define LINUX_LINE_END "\n"

document_t::document_t()
    : cursorId(1)
    , blockId(1)
    , columns(0)
    , rows(0)
    , windowsLineEnd(false)
{
}

document_t::~document_t()
{
    cursors.clear();
    blocks.clear();

    // for (auto& b : blocks) {
    // std::cout << ":<<" << b.use_count() << std::endl;
    // }

    close();
}

void document_t::updateBlocks(block_list& blocks, size_t lineNumber, size_t count)
{
    if (lineNumber > 0) {
        lineNumber--;
    }
    std::vector<block_ptr>::iterator it = blocks.begin();
    it += lineNumber;

    while (it != blocks.end()) {
        block_ptr block = *it;
        block->lineNumber = lineNumber++;
        block->lineCount = 1;
        if (app_t::instance()->lineWrap && columns > 0) {
            block->lineCount = 1 + ((block->length() - 1) / columns);
        }
        it++;

        if (count > 0) {
            if (count-- == 0)
                break;
        }
    }
}

block_ptr document_t::firstBlock()
{
    if (!blocks.size()) {
        return nullptr;
    }
    return blocks.front();
}

block_ptr document_t::lastBlock()
{
    if (!blocks.size()) {
        return nullptr;
    }
    return blocks.back();
}

block_ptr document_t::nextBlock(block_t* block)
{
    if (!block) {
        return nullptr;
    }

    size_t line = block->lineNumber;
    block_list::iterator it = blocks.begin();
    if (line + 1 >= blocks.size()) {
        return nullptr;
    }
    line++;
    it += line;
    return *it;
}

block_ptr document_t::previousBlock(block_t* block)
{
    if (!block || blocks.size() < 2) {
        return nullptr;
    }

    size_t line = block->lineNumber;
    block_list::iterator it = blocks.begin();
    if (line < 1) {
        return nullptr;
    }
    line--;
    it += line;
    return *it;
}

std::string _tabsToSpaces(std::string line) {
    int tabSize = app_t::instance()->tabSize;
    if (tabSize < 2) {
        return line;
    }
    std::string tab = " ";
    for (int i=1; i<tabSize; i++) {
        tab += " ";
    }
    std::string t;
    for(auto c : line) {
        if (c == '\t') {
            t += tab;
        } else {
            t += c;
        }
    }
    return t;
}

bool document_t::open(std::string path, bool enableBuffer)
{
    blocks.clear();
    cursors.clear();

    filePath = path;
    std::string tmpPath = TMP_BUFFER;
    mkstemp((char*)tmpPath.c_str());

    file = std::ifstream(path, std::ifstream::in);
    std::ofstream tmp;
    if (enableBuffer) {
        tmp = std::ofstream(tmpPath, std::ofstream::out);
    }

    std::string line;
    size_t offset = 0;
    size_t pos = file.tellg();
    size_t lineNo = 0;
    while (std::getline(file, line)) {
        block_ptr b = std::make_shared<block_t>();

        // tabs to spaces
        if (app_t::instance()->tabsToSpaces) {
            line = _tabsToSpaces(line);
        }

        // b->uid = blockId++;
        b->document = this;
        b->originalLineNumber = lineNo++;

        b->file = &file;
        b->filePosition = pos;

        //--------------------------
        b->lineNumber = b->originalLineNumber;
        b->lineCount = 1;
        if (columns) {
            b->lineCount = line.length() / columns;
        }
        if (line.length() && line[line.length() - 1] == '\r') {
            line.pop_back();
            offset++;
            windowsLineEnd = true;
        }
        //--------------------------

        blocks.push_back(b);
        pos = file.tellg();
        pos -= offset;

        if (enableBuffer) {
            tmp << line << std::endl;
        } else {
            b->setText(line);
        }
    }

    if (enableBuffer) {
        tmp.close();
    }

    if (!blocks.size()) {
        addBlockAtLine(0);
    }

    // reopen from tmp
    file = std::ifstream(tmpPath, std::ifstream::in);

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

    clearCursors();
    return true;
}

void document_t::save()
{
    std::string lineEnd = windowsLineEnd ? WINDOWS_LINE_END : LINUX_LINE_END;
    std::ofstream tmp(filePath, std::ofstream::out);
    for (auto b : blocks) {
        std::string text = b->text();
        tmp << text << lineEnd;
    }
}

void document_t::saveAs(const char* path, bool replacePath)
{
    std::ofstream tmp(path, std::ofstream::out);
    for (auto b : blocks) {
        std::string text = b->text();
        tmp << text << std::endl;
    }
}

void document_t::close()
{
    file.close();
}

void document_t::print()
{
    for (auto b : blocks) {
        b->print();
    }
}

block_ptr document_t::blockAtLine(size_t line)
{
    block_list::iterator it = blocks.begin();
    if (line >= blocks.size()) {
        return nullptr;
    }
    if (line > 0) {
        it += (line - 1);
    }
    return *it;
}

block_ptr document_t::addBlockAtLine(size_t line)
{
    block_list::iterator beg = blocks.begin();

    block_ptr b = std::make_shared<block_t>();
    b->document = this;
    b->originalLineNumber = 0;

    size_t lineNumber = 0;
    if (!blocks.size() || line >= blocks.size()) {
        blocks.push_back(b);
        beg = blocks.begin();
        if (!blocks.size()) {
            lineNumber = 0;
        } else {
            lineNumber = blocks.size() - 1;
        }
    } else {
        blocks.insert(beg + line, b);
        lineNumber = line;
    }

    updateBlocks(blocks, lineNumber);
    return b;
}

void document_t::removeBlockAtLine(size_t lineNumber, size_t count)
{
    block_list::iterator beg = blocks.begin();
    if (!blocks.size() || lineNumber >= blocks.size()) {
        return;
    }

    if (lineNumber + count >= blocks.size()) {
        count = blocks.size() - lineNumber;
    }

    // std::cout << lineNumber << " : " << count << std::endl;

    blocks.erase(beg + lineNumber, beg + lineNumber + count);
    updateBlocks(blocks, lineNumber);
}

cursor_t document_t::cursor()
{
    if (!cursors.size()) {
        cursor_t cursor;
        cursor.setPosition(firstBlock(), 0);
        addCursor(cursor);
    }
    return *cursors.begin();
}

void document_t::addCursor(cursor_t cur)
{
    cur.uid = cursorId++;
    cursors.emplace_back(cur);
}

void document_t::setCursor(cursor_t cur, bool mainCursor)
{
    if (!cursors.size()) {
        addCursor(cur);
        return;
    }

    // replace main cursor
    if (mainCursor) {
        cur.uid = cursor().uid;
    }

    for (auto& c : cursors) {
        if (c.uid == cur.uid) {
            c = cur;
            return;
        }
    }
}

void document_t::updateCursors(cursor_list& curs)
{
    for (auto c : curs) {
        for (auto& cc : cursors) {
            if (cc.uid == c.uid) {
                cc = c;
                break;
            }
        }
    }
}

void document_t::clearCursors()
{
    cursors.clear();
    cursor();
}

bool document_t::hasSelections()
{
    for (auto& c : cursors) {
        if (c.hasSelection()) {
            return true;
        }
    }
    return false;
}

void document_t::clearSelections()
{
    for (auto& c : cursors) {
        c.clearSelection();
    }
}

void document_t::clearDuplicateCursors()
{
    if (cursors.size() == 1)
        return;
    cursor_list cs;
    for (auto _c : cursors) {
        bool duplicate = false;
        cursor_t c = _c;
        c.normalizeSelection(true);
        for (auto cc : cs) {
            cc.normalizeSelection(true);
            if (cc.cursor.block == c.cursor.block && cc.cursor.position == c.cursor.position && cc.anchor.block == c.anchor.block && cc.anchor.position == c.anchor.position) {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;
        cs.push_back(_c);
    }
    cursors = cs;
}

cursor_t document_t::findNextOccurence(cursor_t cur, std::string word)
{
    // log("finding %s<<", word.c_str());
    block_ptr block = cur.block();
    while (block) {
        std::vector<search_result_t> search_results = search_t::instance()->find(block->text(), word);
        for (auto& i : search_results) {
            if (block == cur.block() && (i.end - 1 <= cur.cursor.position || i.end - 1 <= cur.anchor.position))
                continue;
            cursor_t res;
            res.setPosition(block, i.end - 1);
            res.setAnchor(block, i.begin);
            // log("found! %s %d %d %d ", block->text().c_str(), i.begin, i.end, cur.cursor.position);
            return res;
        }
        block = block->next();
    }
    return cursor_t();
}

void document_t::addBufferDocument(const std::string& largeText)
{
    std::string tmpPath = TMP_PASTE;
    mkstemp((char*)tmpPath.c_str());

    log(tmpPath.c_str());

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
    std::vector<block_ptr> bufferBlocks;
    size_t ln = cursor.block()->lineNumber;
    for (auto bb : buffer->blocks) {

        block_ptr b = std::make_shared<block_t>();

        b->document = this;
        b->originalLineNumber = 0;
        b->file = bb->file;
        b->filePosition = bb->filePosition;
        b->lineNumber = 0;
        b->lineCount = 1;
        b->dirty = false;
        b->content = "";
        b->data = nullptr;

        bufferBlocks.push_back(b);
    }
    blocks.insert(blocks.begin() + ln, make_move_iterator(bufferBlocks.begin()), make_move_iterator(bufferBlocks.end()));
    updateBlocks(blocks, ln > 0 ? ln - 1 : ln);
}

void document_t::setColumns(int c)
{
    bool update = columns != c;
    columns = c;
    if (update) {
        updateBlocks(blocks, 0);
    }
}

void document_t::setRows(int r)
{
    rows = r;
}
