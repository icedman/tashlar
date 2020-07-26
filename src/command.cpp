#include "command.h"
#include "app.h"
#include "document.h"
#include "editor.h"
#include "statusbar.h"

// todo cursor implementation is still a mess
bool processEditorCommand(command_t cmdt, char ch)
{
    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t& cursor = doc->cursors[0];
    struct block_t& block = *cursor.block();

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

            doc->insertFromBuffer(cursor, doc->buffers.back());

            cursorMovePosition(&cursor, cursor_t::EndOfDocument);
            doc->history().addPasteBuffer(cursor, doc->buffers.back());

            doc->history().end();
            doc->addSnapshot();
            doc->clearCursors();
            doc->update(true);
        }
        return true;

    case CMD_MOVE_CURSOR_START_OF_DOCUMENT:
    case CMD_MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED:
        cursorMovePosition(&cursor, cursor_t::StartOfDocument, cmd == CMD_MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED);
        return true;

    case CMD_MOVE_CURSOR_END_OF_DOCUMENT:
    case CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED:
        cursorMovePosition(&cursor, cursor_t::EndOfDocument, cmd == CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED);
        return true;

    case CMD_ADD_CURSOR_AND_MOVE_UP:
        doc->addCursor(cursor);
        cursorMovePosition(&cursor, cursor_t::Up);
        doc->history().mark();
        return true;

    case CMD_ADD_CURSOR_AND_MOVE_DOWN:
        doc->addCursor(cursor);
        cursorMovePosition(&cursor, cursor_t::Down);
        doc->history().mark();
        return true;

    case CMD_COPY:
        app->clipBoard = cursor.selectedText();
        if (app->clipBoard.length()) {
            app->statusbar->setStatus("text copied", 2000);
        }
        return true;

    case CMD_SELECT_ALL:
        cursorMovePosition(&cursor, cursor_t::StartOfDocument);
        cursorMovePosition(&cursor, cursor_t::EndOfDocument, true);
        return true;

    case CMD_SELECT_WORD:
    case CMD_ADD_CURSOR_FOR_SELECTED_WORD:

        if (cursor.hasSelection()) {
            if (cmd == CMD_ADD_CURSOR_FOR_SELECTED_WORD) {
                struct cursor_t c = cursor;
                if (cursorFindWord(&cursor, cursor.selectedText())) {
                    doc->addCursor(c);
                    doc->updateCursor(cursor);
                }
            }
        } else {
            cursorSelectWord(&cursor);
            doc->history().mark();
        }

        return true;

    case CMD_MOVE_CURSOR_PREVIOUS_PAGE:
        doc->clearCursors();
        cursorMovePosition(&cursor, cursor_t::Up, false, editor->viewHeight + 1);
        return true;

    case CMD_MOVE_CURSOR_NEXT_PAGE:
        doc->clearCursors();
        cursorMovePosition(&cursor, cursor_t::Down, false, editor->viewHeight + 1);
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

    //-----------------
    // multi cursor updates
    //-----------------
    bool handled = false;
    bool markHistory = false;
    bool snapShot = false;
    std::vector<struct cursor_t>& cursors = doc->cursors;

    struct block_t* targetBlock = 0;
    struct blockdata_t* targetBlockData = 0;

    std::vector<size_t> blockIds;

    //----------------
    // check for multiline selections
    //----------------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        blockIds.push_back(cur.block()->uid);
        if (cur.isMultiBlockSelection()) {
            switch (cmd) {
            case CMD_INDENT:
            case CMD_UNINDENT:
            case CMD_CUT:
            case CMD_DELETE:
            case CMD_BACKSPACE:
            case CMD_SPLIT_LINE:
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
 
    //-----------
    // deal with cursors having selections
    //-----------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        bool update = false;

        if (!cur.hasSelection()) {
            continue;
        }

        switch (cmd) {
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

        // refresh cursor position
        switch (cmd) {
        case CMD_BACKSPACE:
        case CMD_SPLIT_LINE: {
            for (auto& b : doc->blocks) {
                if (b.uid == blockIds[i]) {
                    cur.setPosition(&b, cur.relativePosition());
                    cur._anchor = cur._position;
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
            //doc->addSnapshot();
            if (cursors.size() > 1 && cur.isMultiBlockSelection()) {
                // cur._position = cur.selectionStart();
                // cur._anchorPosition = cur._position;
            }
            int count = cursorIndent(&cur);
            if (count) {
                update = true;
            }
            // doc->history().addDelete(cur, 0);
            handled = true;
            break;
        }

        case CMD_UNINDENT: {
            //doc->addSnapshot();
            if (cursors.size() > 1 && cur.isMultiBlockSelection()) {
                // cur._position = cur.selectionStart();
                // cur._anchorPosition = cur._position;
            }
            int count = cursorUnindent(&cur);
            if (count) {
                update = true;
            }
            // doc->history().addDelete(cur, 0);
            handled = true;
            break;
        }

        case CMD_DELETE_SELECTION:
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
            int count = cursorInsertText(&cur, cmdt.args);
            cur._position.position += cmdt.args.length();
            cur._anchor = cur._position;
            update = true;
            handled = true;
            if (cmdt.args == " ") {
                markHistory = true;
            }
            break;
        }
        case CMD_DELETE:
            cursorEraseText(&cur, 1);
            update = true;
            handled = true;
            break;

        case CMD_BACKSPACE: {
            bool repositionMainCursor = cur.relativePosition() == 0;
            if (cursorMovePosition(&cur, cursor_t::Left)) {
                cursorEraseText(&cur, 1);
            }
            update = true;
            handled = true;

            if (i > 0 && repositionMainCursor) {
                doc->update(true);
                if (doc->cursors[0].position() > cur.position()) {
                    cursorMovePosition(&doc->cursors[0], cursor_t::Move::PrevBlock, false);
                }
            }
            break;
        }

        case CMD_SPLIT_LINE: {; 
            cursorSplitBlock(&cur);
            update = true;
            handled = true;
            markHistory = true;

            if (i > 0) {
                doc->update(true);
                if (doc->cursors[0].position() > cur.position()) {
                    cursorMovePosition(&doc->cursors[0], cursor_t::Move::NextBlock, false);
                }
            }
            break;
        }

        default:
            break;
        }

        doc->update(update);
        doc->updateCursor(cur);
    }

    if (markHistory) {
        doc->history().mark(); 
    }

    if (snapShot) {
        doc->addSnapshot();
    } 

    return handled;
}
