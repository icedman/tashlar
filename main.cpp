#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <set>
#include <string>
#include <vector>
#include <cstring>

#include "cursor.h"
#include "document.h"
#include "editor.h"

#include "extension.h"

#define CTRL_KEY(k) ((k)&0x1f)

enum KEY_ACTION {
    KEY_NULL = 0, /* NULL */
    ENTER = 13, /* Enter */
    ESC = 27, /* Escape */
    BACKSPACE = 127, /* Backspace */
    /* The following are just soft codes, not really reported by the
         * terminal directly. */
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

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
        init_pair(i, i, !(i%2) ? bg : selBg);
        // init_pair(i, i, bg);
    }

    init_pair(color_pair_e::SELECTED, selFg, selBg);
    init_pair(color_pair_e::NORMAL, fg, bg);
}

int editorReadKey()
{
    int fd = STDIN_FILENO;

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return -1;
    }

    if (c == ESC) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        /* ESC [ sequences. */
        if (seq[0] == '[') {

            if (seq[1] >= '0' && seq[1] <= '9') {

                /* Extended escape, read additional byte. */
                if (read(fd, &seq[2], 1) == 0)
                    return ESC;

                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '3':
                        return KEY_DC; // DEL_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    }
                }

                if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[0], 1) != 1)
                        return '\x1b';
                    if (read(STDIN_FILENO, &seq[1], 1) != 1)
                        return '\x1b';

                    if (seq[0] == '2') {
                        switch (seq[1]) {
                        case 'A':
                            return KEY_SR; // ARROW_UP;
                        case 'B':
                            return KEY_SF; // ARROW_DOWN;
                        case 'C':
                            return KEY_SRIGHT; // ARROW_RIGHT;
                        case 'D':
                            return KEY_SLEFT; // ARROW_LEFT;
                        }
                    }
                }

            } else {
                switch (seq[1]) {
                case 'A':
                    return KEY_UP; // ARROW_UP;
                case 'B':
                    return KEY_DOWN; // ARROW_DOWN;
                case 'C':
                    return KEY_RIGHT; // ARROW_RIGHT;
                case 'D':
                    return KEY_LEFT; // ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        }

        /* ESC O sequences. */
        else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }
    }

    return c;
}

void renderEditor(struct editor_t &editor)
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

void renderStatus(WINDOW *win, struct editor_t &editor)
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

void renderCursor(struct editor_t &editor)
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
        filename = argv[argc-1];
    } else {
        return 0;
    }

    for(int i=0;i<argc-1;i++) {
        if (strcmp(argv[i], "-t") == 0) {
            theme = argv[i+1];
        }
    }

    editor.lang = language_from_file(filename, extensions);
    editor.theme = theme_from_name(theme, extensions);

    editor.document.open(filename);

    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

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
            ch = editorReadKey();
            if (ch != -1) {
                break;
            }
            // pulse ..
        }
        curs_set(0);

        std::string s;
        s += (char)ch;

        sprintf(keyName, "%s", keyname(ch));
        std::string k = keyName;
        if (k == "(null)") {
            ch = 0;
        }

        //-----------------
        // commands
        //-----------------
        switch (ch) {
        case CTRL_KEY('s'):
            sprintf(keyName, "save");
            ch = 0;
            break;
        case CTRL_KEY('c'):
            editor.clipBoard = cursor.selectedText().c_str();
            sprintf(keyName, "copy");
            ch = 0;
            break;
        case CTRL_KEY('v'):
            sprintf(keyName, "paste");
            cursorInsertText(&cursor, editor.clipBoard);
            if (cursorMovePosition(&cursor, cursor_t::Right, false, editor.clipBoard.length())) {
                doc->setCursor(cursor);
            }
            doc->update();
            ch = 0;
            break;
        case CTRL_KEY('q'):
            end = true;
            ch = 0;
            break;
        }

        //-----------------
        // process keys
        //-----------------
        // main cursor
        switch (ch) {
        case PAGE_UP:
            for(int i=0;i<editor.viewHeight+1; i++) {
                if (cursorMovePosition(&cursor, cursor_t::Up, ch == KEY_SR)) {
                    doc->setCursor(cursor);
                } else {
                    break;
                }
                renderEditor(editor);
                wrefresh(editor.win);
            }
            ch = 0;
            break;
        case PAGE_DOWN:
            for(int i=0;i<editor.viewHeight+1; i++) {
                if (cursorMovePosition(&cursor, cursor_t::Down, ch == KEY_SF)) {
                    doc->setCursor(cursor);
                } else {
                    break;
                }
                renderEditor(editor);
                wrefresh(editor.win);
            }
            ch = 0;
            break;
        }

        // multi-cursor
        for (auto cursor : doc->cursors) {
            if (ch == 0) {
                break;
            }

            bool update = false;
            switch (ch) {
            case KEY_SR:
            case KEY_UP:
                if (cursorMovePosition(&cursor, cursor_t::Up, ch == KEY_SR)) {
                    doc->setCursor(cursor);
                }
                break;
            case KEY_SF:
            case KEY_DOWN:
                if (cursorMovePosition(&cursor, cursor_t::Down, ch == KEY_SF)) {
                    doc->setCursor(cursor);
                }
                break;
            case KEY_SLEFT:
            case KEY_LEFT:
                if (cursorMovePosition(&cursor, cursor_t::Left, ch == KEY_SLEFT)) {
                    doc->setCursor(cursor);
                }
                break;
            case KEY_SRIGHT:
            case KEY_RIGHT:
                if (cursorMovePosition(&cursor, cursor_t::Right, ch == KEY_SRIGHT)) {
                    doc->setCursor(cursor);
                }
                break;
            
            case KEY_RESIZE:
                clear();
                break;

            //---------------
            // these go to undo history
            //---------------
            case KEY_DC:
                cursorEraseText(&cursor, 1);
                update = true;
                break;

            case BACKSPACE:
            case KEY_BACKSPACE:
                if (cursorMovePosition(&cursor, cursor_t::Left)) {
                    cursorEraseText(&cursor, 1);
                    doc->setCursor(cursor);
                }
                update = true;
                break;
            case 10: // newline
            case ENTER:
                cursorSplitBlock(&cursor);
                if (cursorMovePosition(&cursor, cursor_t::Right)) {
                    doc->setCursor(cursor);
                }
                update = true;
                break;

            default:
                cursorInsertText(&cursor, s);
                if (cursorMovePosition(&cursor, cursor_t::Right)) {
                    doc->setCursor(cursor);
                }
                update = true;
                break;
            }

            if (update) {
                doc->update();
            }
        }
    }

    endwin();

    return 0;
}