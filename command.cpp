#include "command.h"
#include "app.h"
#include "document.h"
#include "editor.h"
#include "statusbar.h"

bool processEditorCommand(command_t cmdt, char ch)
{
    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
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
        doc->addSnapshot();
        return true;

    case CMD_TAB:
        if (!app->inputBuffer.length() && !app->commandBuffer.size()) {
            std::string tab = "";
            for (int i = 0; i < app->tabSize; i++) {
                tab += " ";
            }
            app->inputBuffer = tab;
        }
        return true;

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
        doc->setCursor(cursor);
        return true;

    case CMD_MOVE_CURSOR_END_OF_DOCUMENT:
    case CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED:
        cursorMovePosition(&cursor, cursor_t::EndOfDocument, cmd == CMD_MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED);
        doc->setCursor(cursor);
        return true;

    case CMD_ADD_CURSOR_AND_MOVE_UP:
        doc->addCursor(cursor);
        cursorMovePosition(&cursor, cursor_t::Up);
        doc->setCursor(cursor);
        doc->history().mark();
        return true;

    case CMD_ADD_CURSOR_AND_MOVE_DOWN:
        doc->addCursor(cursor);
        cursorMovePosition(&cursor, cursor_t::Down);
        doc->setCursor(cursor);
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
        doc->setCursor(cursor);
        return true;

    case CMD_SELECT_WORD:
    case CMD_ADD_CURSOR_FOR_SELECTED_WORD:

        if (cursor.hasSelection()) {
            if (cmd == CMD_ADD_CURSOR_FOR_SELECTED_WORD) {
                struct cursor_t c = cursor;
                c.selectionStart(); // normalizes the positions
                if (cursorFindWord(&cursor, cursor.selectedText())) {
                    doc->addCursor(c);
                    doc->updateCursor(cursor);
                    doc->setCursor(cursor);
                }
            }
        } else {
            cursorSelectWord(&cursor);
            doc->setCursor(cursor);
            doc->history().mark();
        }

        return true;

    case CMD_MOVE_CURSOR_PREVIOUS_PAGE:
        doc->clearCursors();
        cursorMovePosition(&cursor, cursor_t::Up, false, editor->viewHeight + 1);
        doc->setCursor(cursor);
        return true;

    case CMD_MOVE_CURSOR_NEXT_PAGE:
        doc->clearCursors();
        cursorMovePosition(&cursor, cursor_t::Down, false, editor->viewHeight + 1);
        doc->setCursor(cursor);
        return true;

    case CMD_DUPLICATE_LINE:
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_COPY);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        app->commandBuffer.push_back(CMD_SPLIT_LINE);
        // app->commandBuffer.push_back(CMD_HISTORY_SNAPSHOT); // TODO << wasteful of resources
        return true;

    case CMD_DELETE_LINE:
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        // app->commandBuffer.push_back(CMD_COPY);
        app->commandBuffer.push_back(CMD_DELETE);
        app->commandBuffer.push_back(CMD_HISTORY_SNAPSHOT); // TODO << wasteful of resources
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
        // app->commandBuffer.push_back(CMD_HISTORY_SNAPSHOT); // TODO << wasteful of resources
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
        // app->commandBuffer.push_back(CMD_HISTORY_SNAPSHOT); // TODO << wasteful of resources
        return true;

    default:
        break;
    }

    // morph some commands
    //-----------------
    // multi cursor updates
    //-----------------
    bool handled = false;
    bool markHistory = false;
    bool snapShot = false;
    std::vector<struct cursor_t> cursors = doc->cursors;

    struct block_t* targetBlock = 0;
    struct blockdata_t* targetBlockData = 0;

    //----------------
    // check for multiline selections
    //----------------
    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t cur = cursors[i];
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
        doc->addSnapshot();
    }

    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        int advance = 0;
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

            app_t::instance()->log("cursor down");

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
                cur._anchorPosition = cur._position;
            }
            int count = cursorIndent(&cur);
            if (count) {
                advance += count;
                update = true;
            }
            doc->history().addDelete(cur, 0);
            handled = true;
            break;
        }

        case CMD_UNINDENT: {
            //doc->addSnapshot();
            if (cursors.size() > 1 && cur.isMultiBlockSelection()) {
                // cur._position = cur.selectionStart();
                cur._anchorPosition = cur._position;
            }
            int count = cursorUnindent(&cur);
            if (count) {
                advance -= count;
                update = true;
            }
            doc->history().addDelete(cur, 0);
            handled = true;
            break;
        }

        case CMD_DELETE_SELECTION:
            if (cur.hasSelection()) {
                int count = cursorDeleteSelection(&cur);
                if (count) {
                    doc->history().addDelete(cur, count);
                }
                cur.clearSelection();
                advance -= count;
                update = true;
                break;
            }
            handled = true;
            break;

        case CMD_CUT:
            handled = true;
            break;

        case CMD_DELETE:
            doc->history().addDelete(cur, 1);
            cursorEraseText(&cur, 1);
            advance--;
            update = true;
            handled = true;
            break;

        case CMD_BACKSPACE:
            if (cursorMovePosition(&cur, cursor_t::Left)) {
                doc->history().addDelete(cur, 1);
                cursorEraseText(&cur, 1);
                advance--;
            }
            update = true;
            handled = true;
            break;

        case CMD_SPLIT_LINE:
            doc->history().addSplit(cur);
            cursorSplitBlock(&cur);
            cursorMovePosition(&cur, cursor_t::Right);
            advance++;
            update = true;
            handled = true;
            markHistory = true;
            break;

        default:
            break;
        }

        doc->updateCursor(cur);

        if (update) {
            doc->update(advance != 0);

            for (int j = 0; j < cursors.size(); j++) {
                struct cursor_t& c = cursors[j];
                if (c.position() > 0 && c.position() > cur.position() && c.uid != cur.uid) {
                    c._position += advance;
                    c._anchorPosition += advance;
                    c.update();
                }
                doc->updateCursor(c);
            }
        }
    }

    if (snapShot) {
        doc->addSnapshot();
    }

    if (markHistory) {
        doc->history().mark();
    }

    return handled;
}
