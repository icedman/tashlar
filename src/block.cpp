#include "block.h"
#include "document.h"
#include "util.h"

static size_t blocksCreated = 0;

blockdata_t::blockdata_t()
    : dirty(true)
    , folded(false)
    , foldable(false)
    , foldedBy(0)
    , dots(0)
    , indent(0)
    , lastPrevBlockRule(0)
{
}

blockdata_t::~blockdata_t()
{
    if (dots) {
        free(dots);
    }
}

block_t::block_t()
    : document(0)
    , file(0)
    , filePosition(0)
    , lineNumber(0)
    , lineCount(0)
    , data(0)
    , dirty(false)
    , cachedLength(0)
{
    blocksCreated++;
}

block_t::~block_t()
{
    blocksCreated--;
    // std::cout << "blocks: " << blocksCreated << " : " << text() << std::endl;
}

std::string block_t::text()
{
    if (dirty) {
        return content;
    }

    if (file) {
        file->seekg(filePosition, file->beg);
        size_t pos = file->tellg();
        std::string line;
        if (std::getline(*file, line)) {
            return line;
        }
    }

    return "";
}

void block_t::setText(std::string t)
{
    dirty = true;
    content = t;
    if (data) {
        data->dirty = true;
        // if (data->folded && data->foldable) {
        //     document->editor->toggleFold(lineNumber);
        // }
    }
    cachedLength = 0;
}

size_t block_t::length()
{
    if (cachedLength == 0) {
        cachedLength = text().length() + 1;
    }
    return cachedLength;
}

block_ptr block_t::next()
{
    return document->nextBlock(this);
}

block_ptr block_t::previous()
{
    return document->previousBlock(this);
}

void block_t::print()
{
    // std::cout << lineNumber << ": " << text() << std::endl;
    log("%d %s", lineNumber, text().c_str());
}

bool block_t::isValid()
{
    block_ptr b = document->blockAtLine(lineNumber+1);
    return (b.get() == this);
}
