#include "command.h"
#include "app.h"
#include "document.h"
#include "editor.h"
#include "statusbar.h"

// 20K
// beyond this threshold, paste will use an additional file buffer
#define SIMPLE_PASTE_THRESHOLD 20000

bool processCommand(command_e cmd, struct app_t* app, char ch)
{
    struct editor_t* editor = app->currentEditor;
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // global
    //-----------------
    switch (cmd) {
    case CMD_CANCEL:
        return true;

    case CMD_SAVE:
        doc->save();
        app->statusbar->setStatus("saved", 2000);
        return true;

    case CMD_PASTE:
        if (app->clipBoard.length() && app->clipBoard.length() < SIMPLE_PASTE_THRESHOLD) {
            app->inputBuffer = app->clipBoard;
        } else {
            break;
        }
        return true;

    case CMD_UNDO:
        doc->undo();
        return true;

    default:
        break;
    }

    //-----------------
    // main cursor
    //-----------------
    switch (cmd) {

    case CMD_PASTE:
        // large buffer paste!
        doc->addSnapshot();
        doc->history().begin();
        doc->addBufferDocument(app->clipBoard);
        app->clipBoard = "";

        cursorInsertText(&cursor, "/* WARNING: pasting very large buffer is not yet ready */");

        doc->insertFromBuffer(cursor, doc->buffers.back());
        doc->history().addPasteBuffer(cursor, doc->buffers.back());
        
        doc->history().end();
        doc->addSnapshot();
        doc->clearCursors();

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
        for (int i = 0; i < editor->viewHeight + 1; i++) {
            if (cursorMovePosition(&cursor, cursor_t::Up)) {
                editor->highlightBlock(doc->block(cursor));
                doc->setCursor(cursor);
            } else {
                break;
            }
        }
        return true;

    case CMD_MOVE_CURSOR_NEXT_PAGE:
        doc->clearCursors();
        for (int i = 0; i < editor->viewHeight + 1; i++) {
            if (cursorMovePosition(&cursor, cursor_t::Down)) {
                editor->highlightBlock(doc->block(cursor));
                doc->setCursor(cursor);
            } else {
                break;
            }
        }
        return true;

    // some macros
    case CMD_DUPLICATE_LINE:
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_COPY);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        return true;

    case CMD_DELETE_LINE:
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_COPY);
        app->commandBuffer.push_back(CMD_DELETE);
        return true;

    case CMD_MOVE_LINE_UP:
        doc->clearCursors();
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_CUT);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_UP);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        return true;

    case CMD_MOVE_LINE_DOWN:
        doc->clearCursors();
        app->commandBuffer.push_back(CMD_SELECT_LINE);
        app->commandBuffer.push_back(CMD_CUT);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_DOWN);
        app->commandBuffer.push_back(CMD_MOVE_CURSOR_START_OF_LINE);
        app->commandBuffer.push_back(CMD_PASTE);
        return true;

    default:
        break;
    }

    //-----------------
    // multi cursor updates
    //-----------------
    bool handled = false;
    bool markHistory = false;
    std::vector<struct cursor_t> cursors = doc->cursors;

    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        int advance = 0;
        bool update = false;

        switch (cmd) {

        //-----------
        // navigation
        //-----------
        case CMD_MOVE_CURSOR_PREVIOUS_WORD:
        case CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED:
            cursorMovePosition(&cur, cursor_t::WordLeft, cmd == CMD_MOVE_CURSOR_PREVIOUS_WORD_ANCHORED);
            doc->history().mark();
            handled = true;
            break;

        case CMD_MOVE_CURSOR_NEXT_WORD:
        case CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED:
            cursorMovePosition(&cur, cursor_t::WordRight, cmd == CMD_MOVE_CURSOR_NEXT_WORD_ANCHORED);
            doc->history().mark();
            handled = true;
            break;

        case CMD_MOVE_CURSOR_START_OF_LINE:
        case CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED:
            cursorMovePosition(&cur, cursor_t::StartOfLine, cmd == CMD_MOVE_CURSOR_START_OF_LINE_ANCHORED);
            doc->history().mark();
            handled = true;
            break;

        case CMD_MOVE_CURSOR_END_OF_LINE:
        case CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED:
            cursorMovePosition(&cur, cursor_t::EndOfLine, cmd == CMD_MOVE_CURSOR_END_OF_LINE_ANCHORED);
            doc->history().mark();
            handled = true;
            break;

        case CMD_MOVE_CURSOR_UP:
        case CMD_MOVE_CURSOR_UP_ANCHORED:
            // editor->scrollY--;
            cursorMovePosition(&cur, cursor_t::Up, cmd == CMD_MOVE_CURSOR_UP_ANCHORED);
            doc->history().mark();
            handled = true;
            break;

        case CMD_MOVE_CURSOR_DOWN:
        case CMD_MOVE_CURSOR_DOWN_ANCHORED:
            // editor->scrollY++;
            cursorMovePosition(&cur, cursor_t::Down, cmd == CMD_MOVE_CURSOR_DOWN_ANCHORED);
            doc->history().mark();
            handled = true;
            break;

        case CMD_MOVE_CURSOR_LEFT:
        case CMD_MOVE_CURSOR_LEFT_ANCHORED:
            cursorMovePosition(&cur, cursor_t::Left, cmd == CMD_MOVE_CURSOR_LEFT_ANCHORED);
            handled = true;
            doc->history().mark();
            break;

        case CMD_MOVE_CURSOR_RIGHT:
        case CMD_MOVE_CURSOR_RIGHT_ANCHORED:
            cursorMovePosition(&cur, cursor_t::Right, cmd == CMD_MOVE_CURSOR_RIGHT_ANCHORED);
            doc->history().mark();
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
        case CMD_CUT:
            app->clipBoard = cur.selectedText();
            if (app->clipBoard.length()) {
                app->statusbar->setStatus("text copied", 2000);
            }
            if (cur.hasSelection()) {
                int count = cursorDeleteSelection(&cur);
                if (count) {
                    doc->history().addDelete(cur, count);
                }
                cur.clearSelection();
                advance -= count;
                update = true;
                doc->history().mark();
            }
            handled = true;
            break;

        case CMD_DELETE:
            if (cur.hasSelection()) {
                advance -= cursorDeleteSelection(&cur);
                cur.clearSelection();
            } else {
                doc->history().addDelete(cur, 1);
                cursorEraseText(&cur, 1);
                advance--;
            }
            update = true;
            handled = true;
            break;

        case CMD_BACKSPACE:
            if (cur.hasSelection()) {
                advance -= cursorDeleteSelection(&cur);
                cur.clearSelection();
            } else {
                if (cursorMovePosition(&cur, cursor_t::Left)) {
                    doc->history().addDelete(cur, 1);
                    cursorEraseText(&cur, 1);
                    advance--;
                }
            }
            update = true;
            handled = true;
            break;

        case CMD_SPLIT_LINE:
            doc->history().addSplit(cur);
            if (cur.hasSelection()) {
                advance -= cursorDeleteSelection(&cur);
                cur.clearSelection();
            }
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
            doc->update();

            for (int j = 0; j < cursors.size(); j++) {
                struct cursor_t& c = cursors[j];
                if (c.position > 0 && c.position + advance > cur.position && c.uid != cur.uid) {
                    c.position += advance;
                    c.anchorPosition += advance;
                    // std::cout << advance << std::endl;
                }
                doc->updateCursor(c);
            }
        }
    }

    if (markHistory) {
        doc->history().mark();
    }

    return handled;
}
