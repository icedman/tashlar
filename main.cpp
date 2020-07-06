#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "cursor.h"
#include "document.h"
#include "editor.h"
#include "util.h"

#include "extension.h"

void setupColors(theme_ptr theme)
{
    int bg = -1;
    int fg = COLOR_GREEN;
    int selBg = COLOR_GREEN;
    int selFg = -1;
    color_info_t clr;
    theme->theme_color("editor.background", clr);
    if (!clr.is_blank()) {
        // bg = clr.index;
    }
    theme->theme_color("editor.foreground", clr);
    if (!clr.is_blank()) {
        fg = clr.index;
    }
    theme->theme_color("editor.selectionBackground", clr);
    if (!clr.is_blank()) {
        selBg = clr.index;
    }

    use_default_colors();
    start_color();
    for (int i = 0; i < 255; i++) {
        init_pair(i, i, !(i % 2) ? bg : selBg);
        // init_pair(i, i, bg);
    }

    init_pair(color_pair_e::SELECTED, selFg, selBg);
    init_pair(color_pair_e::NORMAL, fg, bg);
}

void renderEditor(struct editor_t& editor)
{
    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // calculate view
    //-----------------
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    editor.viewX = 0;
    editor.viewY = 0;
    editor.viewWidth = w.ws_col;
    editor.viewHeight = w.ws_row - 1;

    if (!editor.win) {
        editor.win = newwin(editor.viewHeight, editor.viewWidth, 0, 0);
    }

    wresize(editor.win, editor.viewHeight, editor.viewWidth);

    // wclear(editor.win);

    int offsetX = 0;
    int offsetY = 0;
    editor.cursorScreenX = 0;
    editor.cursorScreenY = 0;

    // scroll
    while (block.lineNumber + 1 - editor.scrollY > editor.viewHeight) {
        editor.scrollY++;
    }
    while (editor.scrollY > 0 && block.lineNumber - editor.scrollY <= 0) {
        editor.scrollY--;
    }
    while (cursor.position - block.position + 1 - editor.scrollX > editor.viewWidth) {
        editor.scrollX++;
    }
    while (editor.scrollX > 0 && cursor.position - block.position - editor.scrollX <= 0) {
        editor.scrollX--;
    }

    editor.cursorScreenY = editor.scrollY;
    offsetY = editor.scrollY;
    editor.cursorScreenX = editor.scrollX;
    offsetX = editor.scrollX;

    //-----------------
    // render the editor
    //-----------------
    // todo: jump to first visible block
    int y = 0;
    for (auto& b : doc->blocks) {
        if (offsetY-- > 0) {
            continue;
        }
        wmove(editor.win, y++, 0);
        wclrtoeol(editor.win);
        editor.highlightBlock(b);
        editor.renderBlock(b, offsetX);
        if (y >= editor.viewHeight) {
            break;
        }
    }
    while (y < editor.viewHeight) {
        wmove(editor.win, y++, 0);
        wclrtoeol(editor.win);
    }
}

void renderStatus(WINDOW* win, struct editor_t& editor)
{
    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    int sel = cursor.anchorPosition - cursor.position;
    if (sel < 0) {
        sel *= -1;
    }
    wmove(win, 0, 0);
    wclrtoeol(win);
    char tmp[512];
    sprintf(tmp, "%s   Line: %d Col: %d   %s",
        doc->fileName.c_str(),
        1 + (int)(block.lineNumber),
        1 + (int)(cursor.position - block.position),
        editor.status.c_str());

    char c;
    int idx = 0;
    while (c = tmp[idx++]) {
        waddch(win, c);
    }
}

void renderCursor(struct editor_t& editor)
{
    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (block.isValid()) {
        int cy = block.lineNumber;
        int cx = cursor.position - block.position;
        wmove(editor.win, cy - editor.cursorScreenY, cx - editor.cursorScreenX);
    } else {
        wmove(editor.win, 0, 0);
    }
}

int main(int argc, char** argv)
{
    struct editor_t editor;

    std::vector<struct extension_t> extensions;
    load_extensions("/opt/visual-studio-code/resources/app/extensions/", extensions);
    load_extensions("/home/iceman/.ashlar/extensions/", extensions);

    char* filename = 0;
    char* theme = "Dracula";
    if (argc > 1) {
        filename = argv[argc - 1];
    } else {
        return 0;
    }

    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            theme = argv[i + 1];
        }
    }

    editor.lang = language_from_file(filename, extensions);
    editor.theme = theme_from_name(theme, extensions);

    editor.document.open(filename);

    /* initialize curses */
    initscr();
    // cbreak();
    raw();
    noecho();
    setupColors(editor.theme);

    clear();

    int ch = 0;
    bool end = false;

    char keyName[32] = "";

    while (!end) {

        curs_set(0);

        renderEditor(editor);
        renderCursor(editor);
        wrefresh(editor.win);

        // renderStatus(statusWin, editor);
        // wrefresh(statusWin);

        //-----------------
        // get input
        //-----------------
        curs_set(1);
        while (true) {
            ch = editor_read_key();
            if (ch != -1) {
                break;
            }
        }
        curs_set(0);

        std::string s;
        s += (char)ch;

        sprintf(keyName, "%s", keyname(ch));
        std::string k = keyName;
 
        struct editor_t* currentEditor = &editor;
        struct document_t* doc = &editor.document;
        struct cursor_t cursor = doc->cursor();
        struct block_t block = doc->block(cursor);
        
        //-----------------
        // app commands
        //-----------------
        
            int i = 0;
        switch (ch) {
        case ESC:
            doc->clearCursors();
            ch = 0;
            break;
        case CTRL_KEY('s'):
            // doc->save();
            for(auto c : doc->cursors) {    
                std::cout << "c" << (i++) << ": " << c.position << std::endl;
            }
            ch = 0;
            break;
        case CTRL_KEY('q'):
            end = true;
            ch = 0;
            break;
        }

        //-----------------
        // process keys (for editor)
        //-----------------
        // main cursor
        bool didAddCursor = false;
        
        switch (ch) {
        case CTRL_KEY('z'):
            doc->undo();
            ch = 0; // consume the key
            break;
        case CTRL_KEY('c'):
            currentEditor->clipBoard = cursor.selectedText().c_str();
            ch = 0;
            break;
        case CTRL_ALT_UP:
            doc->addCursor(cursor);
            didAddCursor = true;
            ch = KEY_UP;
            break;
        case CTRL_ALT_DOWN:
            doc->addCursor(cursor);
            didAddCursor = true;
            ch = KEY_DOWN;
            break;
        case PAGE_UP:
            doc->clearCursors();
            for (int i = 0; i < currentEditor->viewHeight + 1; i++) {
                if (cursorMovePosition(&cursor, cursor_t::Up, ch == KEY_SR)) {
                    currentEditor->highlightBlock(doc->block(cursor));
                    doc->setCursor(cursor);
                } else {
                    break;
                }
            }
            ch = 0;
            break;
        case PAGE_DOWN:
            doc->clearCursors();
            for (int i = 0; i < currentEditor->viewHeight + 1; i++) {
                if (cursorMovePosition(&cursor, cursor_t::Down, ch == KEY_SF)) {
                    currentEditor->highlightBlock(doc->block(cursor));
                    doc->setCursor(cursor);
                } else {
                    break;
                }
            }
            ch = 0;
            break;
        }

        std::vector<struct cursor_t> cursors = doc->cursors;
        
        // multi-cursor
        for(int i=0; i<cursors.size(); i++) {
            if (ch == 0) {
                break;
            }
            
            struct cursor_t &cur = cursors[i];

            int advance = 0;
            bool update = false;
            
            switch (ch) {
            case CTRL_SHIFT_LEFT:
            case CTRL_LEFT:
                cursorMovePosition(&cur, cursor_t::WordLeft, ch == CTRL_SHIFT_LEFT);
                doc->history.mark();
                break;
            case CTRL_SHIFT_RIGHT:
            case CTRL_RIGHT:
                cursorMovePosition(&cur, cursor_t::WordRight, ch == CTRL_SHIFT_RIGHT);
                doc->history.mark();
                break;
            case CTRL_SHIFT_ALT_LEFT:
            case CTRL_ALT_LEFT:
                cursorMovePosition(&cur, cursor_t::StartOfLine, ch == CTRL_SHIFT_ALT_LEFT);
                doc->history.mark();
                break;
            case CTRL_SHIFT_ALT_RIGHT:
            case CTRL_ALT_RIGHT:
                cursorMovePosition(&cur, cursor_t::EndOfLine, ch == CTRL_SHIFT_ALT_RIGHT);
                doc->history.mark();
                break;
            case CTRL_SHIFT_UP:
            case KEY_SR:
            case KEY_UP:
                cursorMovePosition(&cur, cursor_t::Up, (ch == KEY_SR || ch == CTRL_SHIFT_UP));
                doc->history.mark();
                break;
            case CTRL_SHIFT_DOWN:
            case KEY_SF:
            case KEY_DOWN:
                cursorMovePosition(&cur, cursor_t::Down, (ch == KEY_SF || ch == CTRL_SHIFT_DOWN));
                doc->history.mark();
                break;
            case KEY_SLEFT:
            case KEY_LEFT:
                cursorMovePosition(&cur, cursor_t::Left, ch == KEY_SLEFT);
                doc->history.mark();
                break;
            case KEY_SRIGHT:
            case KEY_RIGHT:
                cursorMovePosition(&cur, cursor_t::Right, ch == KEY_SRIGHT);
                doc->history.mark();
                break;

            case KEY_RESIZE:
                clear();
                break;

            //---------------
            // these go to undo history
            //---------------
            case CTRL_KEY('v'):
                doc->history.addInsert(cur, currentEditor->clipBoard);
                cursorInsertText(&cur, currentEditor->clipBoard);
                cursorMovePosition(&cur, cursor_t::Right, false, currentEditor->clipBoard.length());
                doc->history.mark();
                update = true;
                ch = 0;
                break;
            case KEY_DC:
                doc->history.addDelete(cur, 1);
                cursorEraseText(&cur, 1);
                advance--;
                update = true;
                break;

            case BACKSPACE:
            case KEY_BACKSPACE:
                if (cursorMovePosition(&cur, cursor_t::Left)) {
                    doc->history.addDelete(cur, 1);
                    cursorEraseText(&cur, 1);
                    advance--;
                }
                update = true;
                break;
            case 10: // newline
            case ENTER:
                doc->history.addSplit(cur);
                cursorSplitBlock(&cur);
                cursorMovePosition(&cur, cursor_t::Right);
                doc->history.mark();
                advance++;
                update = true;
                break;

            default:
                doc->history.addInsert(cur, s);
                cursorInsertText(&cur, s);
                cursorMovePosition(&cur, cursor_t::Right, false, s.length());
                if (s == " " || s == "\t") {
                    doc->history.mark();
                }
                advance+=s.length();
                update = true;
                break;
            }

            if (!didAddCursor || i == 0) {
                doc->updateCursor(cur);
            }

            if (update) {
                doc->update();

                for(int j=0; j<cursors.size(); j++) {
                    struct cursor_t &c = cursors[j];
                    if (c.position> 0 && c.position + advance > cur.position && c.uid != cur.uid) {
                        c.position += advance;
                        c.anchorPosition += advance;
                        // std::cout << advance << std::endl;
                    }
                    doc->updateCursor(c);
                }
            }
        }
    }

    endwin();

    return 0;
}
