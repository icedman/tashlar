#include "command.h"
#include "editor.h"
#include "document.h"
#include "statusbar.h"

bool processCommand(command_e cmd, struct app_t *app)
{
    struct editor_t* editor = app->currentEditor;
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // global
    //-----------------
    switch(cmd) {
    case CMD_SAVE:
        doc->save();
        app->statusbar->setStatus("saved", 2000);
        return true;
    
    case CMD_PASTE:
        app->inputBuffer = app->clipBoard;
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
    switch(cmd) {

    case CMD_ADD_CURSOR_AND_MOVE_UP:
        doc->addCursor(cursor);
        cursorMovePosition(&cursor, cursor_t::Up);
        doc->setCursor(cursor);
        doc->history.mark();
        return true;
        
    case CMD_ADD_CURSOR_AND_MOVE_DOWN:
        doc->addCursor(cursor);
        cursorMovePosition(&cursor, cursor_t::Down);
        doc->setCursor(cursor);
        doc->history.mark();
        return true;    
    
    case CMD_COPY:
        app->clipBoard = cursor.selectedText();
        if (app->clipBoard.length()) {
            app->statusbar->setStatus("text copied", 2000);
        }
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
            doc->history.mark();
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
        
    default:
        break;
    }

    //-----------------
    // multi cursor updates
    //-----------------
    bool handled = false;
    std::vector<struct cursor_t> cursors = doc->cursors;

    for (int i = 0; i < cursors.size(); i++) {
        struct cursor_t& cur = cursors[i];
        int advance = 0;
        bool update = false;

        switch(cmd) {
        case CMD_SELECT_LINE:
            cursorMovePosition(&cur, cursor_t::StartOfLine, false);
            cursorMovePosition(&cur, cursor_t::EndOfLine, true);
            handled = true;
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
        
    return handled;
}