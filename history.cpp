#include "history.h"
#include "document.h"
#include "cursor.h"

void history_t::begin()
{
    paused = true;
}

void history_t::end()
{
    paused = false;
    mark();
}

void history_t::mark()
{
    if (paused) {
        return;
    }

    if (editBatch.size()) {
        edits.push_back(editBatch);
        editBatch.clear();
    }
}

void history_t::addInsert(struct cursor_t& cur, std::string t)
{
    editBatch.push_back({ .cursor = cur,
        .text = t,
        .edit = cursor_edit_e::EDIT_INSERT });
}

void history_t::addDelete(struct cursor_t& cur, int c)
{
    editBatch.push_back({ .cursor = cur,
        .count = c,
        .edit = cursor_edit_e::EDIT_DELETE });
}

void history_t::addSplit(struct cursor_t& cur)
{
    editBatch.push_back({ .cursor = cur,
        .edit = cursor_edit_e::EDIT_SPLIT });
}

void history_t::addPasteBuffer(struct cursor_t& cur, std::shared_ptr<document_t> buffer)
{
    paused = false;
    mark();

    editBatch.push_back({ .cursor = cur,
        .edit = cursor_edit_e::EDIT_PASTE_BUFFER,
        .buffer = buffer });
}

void history_t::replay()
{
    if (!edits.size()) {
        return;
    }

    edit_batch_t last = edits.back();
    edits.pop_back();

    for (auto batch : edits) {
        for (auto e : batch) {
            switch (e.edit) {

            case EDIT_INSERT:
                cursorInsertText(&e.cursor, e.text);
                break;
            case EDIT_DELETE:
                cursorEraseText(&e.cursor, e.count);
                break;
            case EDIT_SPLIT:
                cursorSplitBlock(&e.cursor);
                break;
            case EDIT_PASTE_BUFFER:
                e.cursor.document->insertFromBuffer(e.cursor, e.buffer);
                break;
            }

            e.cursor.document->update();
        }
    }

    for (auto e : last) {
        e.cursor.document->setCursor(e.cursor);
        break;
    }
}
    
void history_t::initialize(struct document_t *document)
{
    initialState = document->blocks;
    edits.clear();
}