#include "snapshot.h"
#include "document.h"

void snapshot_t::save(block_list& blocks)
{
    snapshot.clear();
    for (auto block : blocks) {
        block_ptr b = std::make_shared<block_t>();
        b->document = block->document;
        b->originalLineNumber = block->originalLineNumber;
        b->file = block->file;
        b->filePosition = block->filePosition;
        b->lineNumber = block->lineNumber;
        b->screenLine = block->screenLine;
        b->lineCount = block->lineCount;
        b->dirty = block->dirty;
        b->content = block->content;
        b->data = nullptr;
        b->cachedLength = 0;
        snapshot.push_back(b);
    }
}

void snapshot_t::restore(block_list& blocks)
{
    while (blocks.size() > snapshot.size()) {
        blocks.pop_back();
    }

    while (blocks.size() < snapshot.size()) {
        block_ptr b = std::make_shared<block_t>();
        blocks.push_back(b);
    }

    block_list::iterator it = blocks.begin();
    for (auto block : snapshot) {
        auto b = *it++;
        b->document = block->document;
        b->originalLineNumber = block->originalLineNumber;
        b->file = block->file;
        b->filePosition = block->filePosition;
        b->lineNumber = block->lineNumber;
        b->screenLine = block->screenLine;
        b->lineCount = block->lineCount;
        b->dirty = block->dirty;
        b->content = block->content;
        b->data = nullptr;
        b->cachedLength = 0;
    }

    // document_t::updateBlocks(snapshot, 0);
}
