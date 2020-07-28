#include "command.h"
#include "app.h"
#include "document.h"
#include "editor.h"
#include "statusbar.h"

/*--------------------------
 * multi-cursor rules
 * 1. update farthest cursor first
 * 2. always operate using relative cursor position - relative to block
 * 3. new lines & line deletions require
*/

#include <algorithm>

bool compareCursor(struct cursor_t a, struct cursor_t b)
{
    return a.position() < b.position();
}

bool compareBlock(struct block_t a, struct block_t b)
{
    return a.position < b.position;
}

// todo cursor implementation is still a mess
bool processEditorCommand(command_t cmdt, char ch)
{
    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;

    std::vector<struct cursor_t> cursors = doc->cursors;
    struct cursor_t& mainCursor = cursors[0];
    struct block_t& block = *mainCursor.block();

    //-----------------
    // main cursor
    //-----------------
    switch (cmd) {
    case CMD_TOGGLE_FOLD:
        editor->toggleFold(block.lineNumber);
        return true;

    case CMD_HISTORY_SNAPSHOT:
        app->inputBuffer = "";
        doc->history().mark();
        doc->addSnapshot();
        return true;

    case CMD_TAB:
        if (!app->inputBuffer.length() && !app->commandBuffer.size()) {
            std::string tab = "";
            for (int i = 0; i < app->tabSize; i++) {
                tab += " ";
            }
            cmdt.cmd = CMD_INSERT;
            cmdt.args = tab;
            cmd = CMD_INSERT;
        }
        break;

    case CMD_PASTE:
        if (app->clipBoard.length() && app->clipBoard.length() < SIMPLE_PASTE_THRESHOLD) {
            app->inputBuffer = app->clipBoard;
        } else if (app->clipBoard.length()) {
            doc->history().mark();
            doc->history().begin();
            doc->addBufferDocument(app->clipBoard);
            app->clipBoard = "";

            doc->insertFromBuffer(mainCursor, doc->buffers.back());

            cursorMovePosition(&mainCursor, cursor_t::EndOfDocument);
            doc->history().addPasteBuffer(mainCursor, doc->buffers.back());

            doc->history().end();
            doc->addSnapshot();
            doc->clearCursors();
            doc->update(true);
        }
        return true;

    case CMD_MOVE_CURSOR_START_OF_DOCUMENT:
    case CMD_MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED:
        cursorMovePosition(&mainCursor, cursor_t::StartOfDocument, cmd == CMD_MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED);
        doc->updateCursor(mainCursor);
        return true;

    case CMD_MOVE_CURSOR_END_OF_DOCUMENT:
    case CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED:
        cursorMovePosition(&mainCursor, cursor_t::EndOfDocument, cmd == CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED);
        doc->updateCursor(mainCursor);
        return true;

    case CMD_ADD_CURSOR_AND_MOVE_UP:
        doc->addCursor(mainCursor);
        cursorMovePosition(&mainCursor, cursor_t::Up);
        doc->updateCursor(mainCursor);
        doc->history().mark();
        return true;

    case CMD_ADD_CURSOR_AND_MOVE_DOWN:
        doc->addCursor(mainCursor);
        cursorMovePosition(&mainCursor, cursor_t::Down);
        doc->updateCursor(mainCursor);
        doc->history().mark();
        return true;

    case CMD_COPY:
        app->clipBoard = mainCursor.selectedText();
        if (app->clipBoard.length()) {
            app->statusbar->setStatus("text copied", 2000);
        }
        return true;

    case CMD_SELECT_ALL:
        cursorMovePosition(&mainCursor, cursor_t::StartOfDocument);
        cursorMovePosition(&mainCursor, cursor_t::EndOfDocument, true);
        doc->updateCursor(mainCursor);
        return true;

    case CMD_SELECT_WORD:
    case CMD_ADD_CURSOR_FOR_SELECTED_WORD:

        if (mainCursor.hasSelection()) {
            if (cmd == CMD_ADD_CURSOR_FOR_SELECTED_WORD) {
                struct cursor_t c = mainCursor;
                c.uid = 0;
                if (cursorFindWord(&mainCursor, mainCursor.selectedText())) {
                    doc->addCursor(c);
                    doc->updateCursor(mainCursor);
                    app_t::instance()->log("! %d %d", c.position(), mainCursor.position());
                }
            }
        } else {
            cursorSelectWord(&mainCursor);
            doc->updateCursor(mainCursor);
        }

        return true;

    case CMD_MOVE_CURSOR_PREVIOUS_PAGE:
        doc->clearCursors();
        cursorMovePosition(&mainCursor, cursor_t::Up, false, editor->viewHeight + 1);
        doc->updateCursor(mainCursor);
        return true;

    case CMD_MOVE_CURSOR_NEXT_PAGE:
        doc->clearCursors();
        cursorMovePosition(&mainCursor, cursor_t::Down, false, editor->viewHeight + 1);
        doc->updateCursor(mainCursor);
        return true;

    case CMD_DUPLICATE_LINE:
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_COPY);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        app->commandBuffer.push_back(CMD_SPLIT_LINE);
        return true;

    case CMD_DELETE_LINE:
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_DELETE);
        return true;

    case CMD_MOVE_LINE_UP:
        doc->clearCursors();
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_CUT);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_UP);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        app->commandBuffer.push_back(CMD_SPLIT_LINE);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_UP);
        return true;

    case CMD_MOVE_LINE_DOWN:
        doc->clearCursors();
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_CUT);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_DOWN);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        app->commandBuffer.push_back(CMD_SPLIT_LINE);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_UP);
        return true;

    default:
        break;
    }

    std::sort(cursors.begin(), cursors.end(), compareCursor);
    /*
    for (auto _c : cursors) {
        app->log("%d", _c.position());
    }
    */

    bool handled = false;
    bool markHistory = false;
    bool snapShot = false;

    //-----------------
    // multi cursor updates
    //-----------------

    struct block_t* targetBlock = 0;
    struct blockdata_t* targetBlockData = 0;

    //----------------
    // check need for snapshot
    //----------------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];

        if (cur.isMultiBlockSelection()) {
            switch (cmd) {
            case CMD_INDENT:
            case CMD_UNINDENT:
            case CMD_CUT:
            case CMD_DELETE:
            case CMD_BACKSPACE:
            case CMD_SPLIT_LINE:
                // this becomes too complicated for our simplistic undo.. make a snapshot then
                snapShot = true;
                break;
            default:
                break;
            }
            break;
        }
    }

    if (snapShot) {
        doc->history().mark();
        doc->addSnapshot();
    }

    std::vector<struct block_t*> multiCursorBlocks;
    bool hasPendingMultiCursorEdits = false;

    //-----------
    // find multi-cursors blocks .. these will be handled separately
    //-----------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        cur.blockUid = cur.block()->uid; // << save some cursor data
        cur.isMultiCursorBlock = false;
        cur.idx = i;

        for (int j = 0; j < cursors.size(); j++) {
            struct cursor_t& cur2 = cursors[j];
            if (cur2.uid == cur.uid || cur2.blockUid != cur.blockUid) {
                continue;
            }

            if (std::find(multiCursorBlocks.begin(), multiCursorBlocks.end(), cur.block()) == multiCursorBlocks.end()) {
                multiCursorBlocks.insert(multiCursorBlocks.begin(), cur.block());
                // multiCursorBlocks.push_back(cur.blockUid);
            }
            cur.isMultiCursorBlock = true;
            cur2.isMultiCursorBlock = true;
        }
    }

    //-----------
    // deal with cursors having selections .. delete selected text if need be
    //-----------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        bool update = false;

        if (!cur.hasSelection() || cur.isMultiCursorBlock) {
            continue;
        }

        switch (cmd) {
        case CMD_SPLIT_LINE:
        case CMD_DELETE:
        case CMD_BACKSPACE: {
            int count = cursorDeleteSelection(&cur);
            cur.clearSelection();
            update = true;
            cmd = CMD_INSERT;
            cmdt.args = "";
            break;
        }
        case CMD_INSERT: {
            int count = cursorDeleteSelection(&cur);
            cur.clearSelection();
            update = true;
            break;
        }
        default:
            break;
        }

        doc->update(update);
        doc->updateCursor(cur);
    }

    //-------------------------
    // now finally begin cursor updates
    //-------------------------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];

        bool update = false;

        if (cur.hasSelection()) {
            switch (cmd) {
            case CMD_CUT:
                // latest cursor will prevail
                app->clipBoard = cur.selectedText();
                if (app->clipBoard.length()) {
                    app->statusbar->setStatus("text copied", 2000);
                }
                cmd = CMD_DELETE_SELECTION;
                break;
            case CMD_DELETE:
            case CMD_BACKSPACE:
            case CMD_SPLIT_LINE:
                cmd = CMD_DELETE_SELECTION;
                break;
            default:
                break;
            }
        }

        //-------------------------
        // refresh cursor position
        //-------------------------
        switch (cmd) {
        case CMD_BACKSPACE:
        case CMD_SPLIT_LINE: {
            for (auto& b : doc->blocks) {
                if (b.uid == cur.blockUid) {
                    cur.setPosition(&b, cur.relativePosition());
                    cur._anchor = cur._position;
                    break;
                }
            }
            break;
        }
        default:
            break;
        }

        switch (cmd) {

        //-----------
        // navigation
        //-----------
        case CMD_MOVE_CURSOR_PREVIOUS_WORD:
        case CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED:
            cursorMovePosition(&cur, cursor_t::WordLeft, cmd == CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED);
            markHistory = true;
            handled = true;
            break;

        case CMD_MOVE_CURSOR_NEXT_WORD:
        case CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED:
            cursorMovePosition(&cur, cursor_t::WordRight, cmd == CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED);
            markHistory = true;
            handled = true;
            break;

        case CMD_MOVE_CURSOR_START_OF_LINE:
        case CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED:
            cursorMovePosition(&cur, cursor_t::StartOfLine, cmd == CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED);
            markHistory = true;
            handled = true;
            break;

        case CMD_MOVE_CURSOR_END_OF_LINE:
        case CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED:
            cursorMovePosition(&cur, cursor_t::EndOfLine, cmd == CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED);
            markHistory = true;
            handled = true;
            break;

        case CMD_MOVE_CURSOR_UP:
        case CMD_MOVE_CURSOR_UP_ANCHORED:
            cursorMovePosition(&cur, cursor_t::Up, cmd == CMD_MOVE_CURSOR_UP_ANCHORED);
            markHistory = true;
            handled = true;

            targetBlock = cur.block();
            targetBlockData = targetBlock->data.get();
            if (targetBlockData && targetBlockData->folded && !targetBlockData->foldable && targetBlock->previous) {
                // repeat the command -- to skip folded block
                app->commandBuffer.clear();
                app->commandBuffer.push_back(cmd);
            }
            break;

        case CMD_MOVE_CURSOR_DOWN:
        case CMD_MOVE_CURSOR_DOWN_ANCHORED:
            cursorMovePosition(&cur, cursor_t::Down, cmd == CMD_MOVE_CURSOR_DOWN_ANCHORED);
            markHistory = true;
            handled = true;

            targetBlock = cur.block();
            targetBlockData = targetBlock->data.get();
            if (targetBlockData && targetBlockData->folded && !targetBlockData->foldable && targetBlock->next) {
                // repeat the command -- to skip folded block
                app->commandBuffer.clear();
                app->commandBuffer.push_back(cmd);
            }
            break;

        case CMD_MOVE_CURSOR_LEFT:
        case CMD_MOVE_CURSOR_LEFT_ANCHORED:
            cursorMovePosition(&cur, cursor_t::Left, cmd == CMD_MOVE_CURSOR_LEFT_ANCHORED);
            markHistory = true;
            handled = true;
            break;

        case CMD_MOVE_CURSOR_RIGHT:
        case CMD_MOVE_CURSOR_RIGHT_ANCHORED:
            cursorMovePosition(&cur, cursor_t::Right, cmd == CMD_MOVE_CURSOR_RIGHT_ANCHORED);
            markHistory = true;
            handled = true;
            break;

        case CMD_SELECT_LINE:
            cursorMovePosition(&cur, cursor_t::StartOfLine, false);
            cursorMovePosition(&cur, cursor_t::EndOfLine, true);
            handled = true;
            break;

        //-----------
        // document edits
        //-----------
        case CMD_INDENT: {
            if (cursors.size() > 1 && cur.isMultiBlockSelection()) {
            }
            int count = cursorIndent(&cur);
            if (count) {
                update = true;
            }
            handled = true;
            break;
        }

        case CMD_UNINDENT: {
            if (cursors.size() > 1 && cur.isMultiBlockSelection()) {
            }
            int count = cursorUnindent(&cur);
            if (count) {
                update = true;
            }
            handled = true;
            break;
        }

        case CMD_DELETE_SELECTION:
            if (cur.isMultiCursorBlock) {
                hasPendingMultiCursorEdits = true;
                handled = true;
                break;
            }

            app_t::instance()->log("del %d", cur.blockUid);

            if (cur.hasSelection()) {
                cursorDeleteSelection(&cur);
                cur.clearSelection();
                update = true;
                break;
            }
            handled = true;
            break;

        case CMD_CUT:
            handled = true;
            break;

        case CMD_INSERT: {
            if (cur.hasSelection() && cur.isMultiCursorBlock) {
                hasPendingMultiCursorEdits = true;
                handled = true;
                break;
            }

            int count = cmdt.args.length();
            cursorInsertText(&cur, cmdt.args);
            cur._position.position += count;
            cur._anchor = cur._position;
            update = true;
            handled = true;
            if (cmdt.args == " ") {
                markHistory = true;
            }

            // update cursors on the same block
            for (int j = 0; j < cursors.size(); j++) {
                struct cursor_t& cur2 = cursors[j];
                if (cur2.uid == cur.uid || cur2.blockUid != cur.blockUid) {
                    continue;
                }
                if (cur2.relativePosition() > cur.relativePosition()) {
                    cur2._position.position += count;
                    cur2._anchor = cur2._position;
                }
            }
            break;
        }
        case CMD_DELETE:

            if (cur.isMultiCursorBlock && cur.hasSelection()) {
                hasPendingMultiCursorEdits = true;
                handled = true;
                break;
            }

            cursorEraseText(&cur, 1);
            update = true;
            handled = true;

            // update cursors on the same block
            for (int j = 0; j < cursors.size(); j++) {
                struct cursor_t& cur2 = cursors[j];
                if (cur2.uid == cur.uid || cur2.blockUid != cur.blockUid) {
                    continue;
                }
                if (cur2.relativePosition() > cur.relativePosition()) {
                    cur2._position.position -= 1;
                    cur2._anchor = cur2._position;
                }
            }
            break;

        case CMD_BACKSPACE: {

            if (cur.isMultiCursorBlock && cur.hasSelection()) {
                hasPendingMultiCursorEdits = true;
                handled = true;
                break;
            }

            bool repositionMainCursor = cur.relativePosition() == 0; // << this instance, a line will be deleted
            if (cursorMovePosition(&cur, cursor_t::Left)) {
                cursorEraseText(&cur, 1);
            }
            update = true;
            handled = true;

            doc->update(i > 0 && repositionMainCursor);

            // update cursors on the same block
            for (int j = 0; j < cursors.size(); j++) {
                struct cursor_t& cur2 = cursors[j];
                if (cur2.uid == cur.uid || cur2.blockUid != cur.blockUid) {
                    continue;
                }

                if (repositionMainCursor) {
                    cursorMovePosition(&cur2, cursor_t::Move::PrevBlock, false);
                    cur2._position.position += cur.relativePosition();
                    cur2._anchor = cur2._position;
                } else {
                    if (cur2.relativePosition() > cur.relativePosition()) {
                        cur2._position.position -= 1;
                        cur2._anchor = cur2._position;
                    }
                }
            }

            break;
        }

        case CMD_SPLIT_LINE: {
            if (cur.isMultiCursorBlock) {
                hasPendingMultiCursorEdits = true;
                handled = true;
                break;
            }

            cursorSplitBlock(&cur);
            update = true;
            handled = true;
            markHistory = true;

            doc->update(true);

            break;
        }

        default:
            break;
        }

        doc->update(update);
        doc->updateCursor(cur);
    }

    // app_t::instance()->log("edits:%d size:%d cmd:%d cmdt:%d", hasPendingMultiCursorEdits, multiCursorBlocks.size(), cmd, cmdt.cmd);

    //-----------
    // deal with multi-cursor blocks (the messy part of multi-cursor)
    //-----------
    if (hasPendingMultiCursorEdits) {
        std::map<size_t, int> cursorAdjust;
        for (auto b : multiCursorBlocks) {
            size_t blockUid = b->uid;
            int linesSplit = 0;
            app_t::instance()->log("multicursor %d", blockUid);
            std::vector<struct cursor_t> curs;
            bool hasSelection = false;

            for (int i = 0; i < cursors.size(); i++) {
                struct cursor_t& cur = cursors[i];
                // app_t::instance()->log("cur.blockuUid:%d blockUid:%d", cur.blockUid, blockUid);
                if (cur.blockUid == blockUid) {
                    curs.push_back(cur);
                    hasSelection = hasSelection || cur.hasSelection();
                    // app_t::instance()->log("multicursor sel %d %d", hasSelection, cmd);
                }
            }

            if (!curs.size()) {
                continue;
            }

            // delete selection
            if (hasSelection) {
                std::string text = curs[0].block()->text();

                for (auto& c : curs) {
                    struct cursor_t start = c.selectionStartCursor();
                    struct cursor_t end = c.selectionEndCursor();
                    int s = start.relativePosition();
                    int l = end.relativePosition() - s;
                    for (int i = 0; i <= l; i++) {
                        text[s + i] = '\r'; // dummy character
                    }
                }

                std::string newText = "";
                int offset = 0;
                for (int i = 0; i < text.length(); i++) {
                    if (text[i] == '\r') {
                        for (auto& c : curs) {
                            if (cursors[c.idx]._position.position + offset > i) {
                                cursors[c.idx]._position.position--;
                                cursors[c.idx].clearSelection();
                                c._position = cursors[c.idx]._position;
                                c.clearSelection();
                            }
                        }
                        offset++;
                        continue;
                    }
                    newText += text[i];
                }

                curs[0].block()->setText(newText);
                // doc->update(true);

                if (cmdt.cmd == CMD_INSERT) {
                    app_t::instance()->commandBuffer.insert(app_t::instance()->commandBuffer.begin(), cmdt);
                    continue;
                }
            }

            if (cmdt.cmd == CMD_SPLIT_LINE) {
                std::vector<struct cursor_t> curs2 = curs;
                // std::sort(curs2.begin(), curs2.end(), compareCursor);

                struct cursor_t& firstCursor = curs[0];
                std::string text = firstCursor.block()->text() + " ";

                int r = 0;
                for (int i = 0; i < curs.size(); i++) {
                    firstCursor._position.position = curs2[i].relativePosition() - r;
                    r = curs2[i].relativePosition();
                    cursorSplitBlock(&firstCursor);
                    linesSplit++;
                }

                cursorAdjust[b->lineNumber] = linesSplit;
                doc->update(true);

                for (int i = 0; i < curs.size(); i++) {
                    cursors[curs[i].idx].setPosition(firstCursor.block(), 0);
                    cursors[curs[i].idx].clearSelection();
                    for (int j = 0; j < i; j++) {
                        //if (curs[i].uid == mainCursor.uid) continue;
                        cursorMovePosition(&cursors[curs[i].idx], cursor_t::Move::StartOfLine, false);
                        cursorMovePosition(&cursors[curs[i].idx], cursor_t::Move::PrevBlock, false);
                    }
                }

                // firstCursor.setPosition(doc->lastAddedBlock, 0);
                // firstCursor.clearSelection();
            }

            doc->clearSelections();
            doc->update(true);
        }

        if (multiCursorBlocks.size() > 1 && cmdt.cmd == CMD_SPLIT_LINE) {
            doc->clearCursors(); // limitation
        }
    }

    if (markHistory) {
        doc->history().mark();
    }

    if (snapShot) {
        doc->addSnapshot();
    }

    for (auto m : cursors) {
        doc->updateCursor(m);
    }
    return handled;
}
