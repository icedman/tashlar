#include "document.h"
#include "cursor.h"

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

std::vector<std::string> splitpath(const std::string& str, const std::set<char> delimiters)
{
    std::vector<std::string> result;

    char const* pch = str.c_str();
    char const* start = pch;
    for (; *pch; ++pch) {
        if (delimiters.find(*pch) != delimiters.end()) {
            if (start != pch) {
                std::string str(start, pch);
                result.push_back(str);
            } else {
                result.push_back("");
            }
            start = pch + 1;
        }
    }
    result.push_back(start);

    return result;
}

bool document_t::open(const char* path)
{
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
    }

    tmp.close();

    // reopen from tmp
    file = std::ifstream(tmpPath, std::ifstream::in);
    update();

    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = splitpath(filePath, delims);
    fileName = spath.back();

    return true;
}

void document_t::close()
{
    save();

    file.close();
    std::cout << "unlink " << tmpPath;
    remove(tmpPath.c_str());
}

void document_t::save()
{
    std::ofstream tmp("out.cpp", std::ofstream::out);
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
    cursors[0].position = cursor.position;
    cursors[0].anchorPosition = cursor.anchorPosition;
    cursors[0].update();
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
    for (auto &b : blocks) {
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
}

struct block_t& document_t::block(struct cursor_t& cursor)
{
    for (auto& b : blocks) {
        if (b.length == 0 && cursor.position == b.position) {
            return b;
        }
        if (b.position <= cursor.position && cursor.position < b.position + b.length) {
            return b;
        }
    }
    return nullBlock;
}