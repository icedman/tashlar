#include "history.h"
#include "app.h"
#include "cursor.h"
#include "document.h"

void history_t::begin()
{
    paused = true;
    didMark = false;
}

void history_t::end()
{
    paused = false;
    if (didMark) {
        mark();
    }
}

void history_t::mark()
{
    if (paused) {
        didMark = true;
        return;
    }

    if (editBatch.size()) {
        edits.push_back(editBatch);
        editBatch.clear();
    }
}

void history_t::_addInsert(struct cursor_t& cur, std::string t)
{
    if (inReplay) {
        return;
    }

    editBatch.push_back({ .blockUid = cur.block()->uid,
        .cursor = cur,
        .text = t,
        .edit = EDIT_INSERT });

    if (t == " ")
        mark();
}

void history_t::_addSplit(struct cursor_t& cur)
{
    if (inReplay) {
        return;
    }

    editBatch.push_back({ .blockUid = cur.block()->uid,
        .cursor = cur,
        .edit = EDIT_SPLIT });
}

void history_t::_addDelete(struct cursor_t& cur, int c)
{
    if (inReplay) {
        return;
    }

    editBatch.push_back({ .blockUid = cur.block()->uid,
        .cursor = cur,
        .count = c,
        .edit = EDIT_DELETE });
}

void history_t::_addDeleteSelection(struct cursor_t& cur, struct cursor_t& curEnd)
{
    if (inReplay) {
        return;
    }

    // todo.. fix this somewhere
    curEnd._document = cur._document;
    app_t::instance()->log("add %d %d", cur.position(), curEnd.position());

    editBatch.push_back({ .blockUid = cur.block()->uid, 
        .cursor = cur,
        .blockEndUid = curEnd.block()->uid,
        .cursorEnd = curEnd, 
        .edit = EDIT_DELETE_SELECTION });

    app_t::instance()->log("%d", editBatch.size());
}

void history_t::_addDeleteBlock(struct cursor_t& cur, size_t blockUid, size_t count)
{
    if (inReplay) {
        return;
    }

    editBatch.push_back({ .blockUid = blockUid,
        .cursor = cur,
        .count = count, 
        .edit = EDIT_DELETE_BLOCK });
}

void history_t::_addBlockSnapshot(struct cursor_t& cur)
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

    editBatch.push_back({ .blockUid = cur.block()->uid,
        .cursor = cur,
        .edit = EDIT_PASTE_BUFFER,
        .buffer = buffer });
}

static bool rebaseCursor(size_t blockUid, struct cursor_t& cur)
{
    struct document_t* doc = cur.document();

    for (auto& b : doc->blocks) {
        if (b.uid == blockUid) {
            cur.setPosition(&b, cur.relativePosition());
            cur._anchor = cur._position;
            return true;
        }
    }

    app_t::instance()->log("unable to rebase");
    return false;
}

static int blockLineNumber(size_t blockUid, struct cursor_t& cur)
{
    struct document_t* doc = cur.document();

    for (auto& b : doc->blocks) {
        if (b.uid == blockUid) { 
            return b.lineNumber;
        }
    }

    app_t::instance()->log("unable to find block");
    return -1;
}

void history_t::replay()
{
    if (!edits.size()) {
        return;
    }

    inReplay = true;
    edit_batch_t last = edits.back();
    edits.pop_back();

    for (auto batch : edits) {
        for (auto e : batch) {
            if (!rebaseCursor(e.blockUid, e.cursor)) {
                edits.clear();
                inReplay = false;
                return;
            }

            switch (e.edit) {

            case EDIT_INSERT:
                cursorInsertText(&e.cursor, e.text);
                break;
            case EDIT_DELETE:
                cursorEraseText(&e.cursor, e.count);
                break;
            case EDIT_DELETE_SELECTION:
                if (!rebaseCursor(e.blockEndUid, e.cursorEnd)) {
                    edits.clear();
                    inReplay = false;
                    return;
                }
                e.cursor._anchor = e.cursorEnd._position; 
                cursorDeleteSelection(&e.cursor);
                break;
            case EDIT_DELETE_BLOCK: {
                int lineNumber = blockLineNumber(e.blockUid, e.cursor);
                if (lineNumber != -1){
                    e.cursor.document()->removeBlockAtLineNumber(lineNumber, e.count);
                } 
                break;
            }
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
            e.cursor.document()->update(true);
            e.cursor.document()->setCursor(e.cursor);
        }
    }

    inReplay = false;
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
