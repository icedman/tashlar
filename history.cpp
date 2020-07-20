#include "history.h"
#include "app.h"
#include "cursor.h"
#include "document.h"

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
    editBatch.push_back({ .block_uid = cur.block()->uid,
        .cursor = cur,
        .text = t,
        .edit = EDIT_INSERT });
}

void history_t::addDelete(struct cursor_t& cur, int c)
{
    editBatch.push_back({ .block_uid = cur.block()->uid,
        .cursor = cur,
        .count = c,
        .edit = EDIT_DELETE });
}

void history_t::addSplit(struct cursor_t& cur)
{
    editBatch.push_back({ .block_uid = cur.block()->uid,
        .cursor = cur,
        .edit = EDIT_SPLIT });
}

void history_t::addBlockSnapshot(struct cursor_t& cur)
{
    /*
    editBatch.push_back({ .cursor = cur,
        .text = cur.block->text(),
        .edit = EDIT_BLOCK_SNAPSHOT });
    */
}

void history_t::addPasteBuffer(struct cursor_t& cur, std::shared_ptr<document_t> buffer)
{
    paused = false;
    mark();

    editBatch.push_back({ .block_uid = cur.block()->uid,
        .cursor = cur,
        .edit = EDIT_PASTE_BUFFER,
        .buffer = buffer });
}

static bool rebaseCursor(cursor_edit_t edit, struct cursor_t& cur)
{
    struct document_t* doc = cur.document();
    struct block_t* block;
    for (auto& b : doc->blocks) {
        if (b.uid == edit.block_uid) {
            app_t::instance()->log("rebased %d", b.uid);
            cur._block = &b;
            cur._position = b.position + cur._relativePosition;
            cur._anchorPosition = cur._position;
            cur.update();
            return true;
        }
    }

    app_t::instance()->log("failed to rebase %d", edit.block_uid);
    return false;
}

void history_t::replay()
{
    if (!edits.size()) {
        return;
    }

    edit_batch_t last = edits.back();
    edits.pop_back();

    app_t::instance()->log("r1");

    for (auto batch : edits) {
        for (auto e : batch) {
            if (!rebaseCursor(e, e.cursor)) {
                edits.clear();
                return;
            }

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
                e.cursor.document()->insertFromBuffer(e.cursor, e.buffer);
                break;
            case EDIT_BLOCK_SNAPSHOT:
                // app_t::instance()->log("todo: implement block snapshot");
                break;
            }

            e.cursor.block()->data = nullptr;
            e.cursor.update();
            e.cursor.document()->setCursor(e.cursor);
        }
    }
}

void history_t::initialize(struct document_t* document)
{
    initialState = document->blocks;
    for (auto& b : initialState) {
        b.previous = 0;
        b.next = 0;
        if (b.data) {
            b.data = nullptr;
        }
    }
    edits.clear();
}
