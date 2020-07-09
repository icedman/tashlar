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
#include "statusbar.h"
#include "keybinding.h"
#include "command.h"
#include "util.h"

#include "extension.h"

// todo ... find colors used by theme.. and pair each with selection backgound
void setupColors(theme_ptr theme)
{
    style_t s = theme->styles_for_scope("comment");
    std::cout << theme->colorIndices.size() << std::endl;

    int bg = -1;
    int fg = COLOR_WHITE;
    int selBg = COLOR_WHITE;
    int selFg = COLOR_BLACK;
    color_info_t clr;
    theme->theme_color("editor.background", clr);
    if (!clr.is_blank()) {
        // bg = clr.index;
    }
    
    theme->theme_color("editor.foreground", clr);
    if (!clr.is_blank()) {
        fg = clr.index;
        // selFg = fg;
    }
    if (!s.foreground.is_blank()) {
        fg = s.foreground.index;
        // selFg = fg;
    }
    theme->theme_color("editor.selectionBackground", clr);
    if (!clr.is_blank()) {
        // selBg = clr.index;
    }

    use_default_colors();
    start_color();
    for (int i = 0; i < 255; i++) {
        // init_pair(i, i, !(i % 2) ? bg : selBg);
        init_pair(i, i, bg);
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

void renderStatus(struct statusbar_t& statusbar, struct editor_t& editor)
{
    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // calculate view
    //-----------------
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    statusbar.viewX = 0;
    statusbar.viewY = w.ws_row - 1;
    statusbar.viewWidth = w.ws_col;
    statusbar.viewHeight = 1;

    if (!statusbar.win) {
        statusbar.win = newwin(statusbar.viewHeight, statusbar.viewWidth, 0, 0);
    }

    mvwin(statusbar.win, statusbar.viewY, statusbar.viewX);
    wresize(statusbar.win, statusbar.viewHeight, statusbar.viewWidth);

    static char tmp[512];
    sprintf(tmp, "Line: %d Col: %d",
        1 + (int)(block.lineNumber),
        1 + (int)(cursor.position - block.position));

    statusbar.setText(doc->fileName, 0);
    statusbar.setText(tmp, -2);
    if (editor.lang) {
        statusbar.setText(editor.lang->id, -1);
    } else {
        statusbar.setText("", -1);
    }
    statusbar.render();
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
    struct statusbar_t statusbar;

    struct app_t app;
    app.statusbar = &statusbar;
    
    //-------------------
    // initialize extensions
    //-------------------
    std::vector<struct extension_t> extensions;
    load_extensions("/opt/visual-studio-code/resources/app/extensions/", extensions);
    load_extensions("~/.ashlar/extensions/", extensions);

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

    app.theme = theme_from_name(theme, extensions);
    editor.theme = app.theme;
    statusbar.theme = app.theme;
    
    //-------------------
    // keybinding
    //-------------------
    bindDefaults();
    
    //-------------------
    // ncurses
    //-------------------
    initscr();
    // cbreak();
    raw();
    noecho();
    
    setupColors(editor.theme);

    // endwin();
    // return 0;
    
    clear();
    
    editor.document.open(filename);

    int ch = 0;
    bool end = false;

    std::string previousKeySequence;        
    while (!end) {
        
        struct editor_t* currentEditor = &editor;
        struct document_t* doc = &editor.document;
        struct cursor_t cursor = doc->cursor();
        struct block_t block = doc->block(cursor);

        bool disableRefresh = app.commandBuffer.size() || app.inputBuffer.length();
        
        if (!disableRefresh) {    
            renderEditor(editor);
            renderStatus(statusbar, editor);
            renderCursor(editor);
    
            curs_set(0);
            wrefresh(statusbar.win);
            wrefresh(editor.win);
            curs_set(1);
        }
        
        //-----------------
        // get input
        //-----------------
        command_e cmd = CMD_UNKNOWN;
        std::string keySequence;
        while(true) {
            if (app.inputBuffer.length()) {
                break;
            }
            if (app.commandBuffer.size()) {
                cmd = app.commandBuffer.front();
                app.commandBuffer.erase(app.commandBuffer.begin());
                break;
            }
            
            ch = readKey(keySequence);
            if (keySequence.length()) {
                statusbar.setStatus(keySequence, 2000);
                cmd = commandKorKeys(keySequence);

                if (cmd == CMD_UNKNOWN && previousKeySequence.length()) {
                    cmd = commandKorKeys(previousKeySequence + "+" + keySequence);
                }

                if (cmd != CMD_UNKNOWN) {
                    previousKeySequence = "";
                } else {
                    previousKeySequence = keySequence;
                }
                
                keySequence = ""; // always consume
                ch = 0;
            }
            
            if (ch != -1) {
                break;
            }
        }

        doc->history.paused = app.inputBuffer.length();
        if (app.inputBuffer.length()) {
            ch = app.inputBuffer[0];
            if (ch == '\n') {
                ch = ENTER;
            }
            
            app.inputBuffer.erase(0,1);
            if (!app.inputBuffer.length()) {
                doc->history.end();
            }
        }

        std::string s;
        s += (char)ch;
								
        if (cmd == CMD_ENTER || ch == ENTER || s == "\n") {
            cmd = CMD_SPLIT_LINE;
        }

        
        if ((cmd == CMD_UNKNOWN || cmd == CMD_CANCEL) && ch >= ALT_ && ch <= CTRL_SHIFT_ALT_) {
            // drop unhandled;
            continue;
        }

        //-----------------
        // app commands
        //-----------------
        switch (cmd) {
        case CMD_CANCEL:
            continue;
            
        case CMD_QUIT:
            end = true;
            break;
        }

        app.currentEditor = &editor;
        if (processCommand(cmd, &app, ch)) {
            continue;
        }

        if (ch < 1) {
            continue;
        }

        switch (ch) {
        case ESC:
            doc->clearCursors();
            continue;
        }

        //-------------------
        // update keystrokes on cursors 
        //-------------------        
        if (s.length() != 1) {
            continue;
        }

        std::vector<struct cursor_t> cursors = doc->cursors;
        for (int i = 0; i < cursors.size(); i++) {
            struct cursor_t& cur = cursors[i];
            
            int advance = 0;

            doc->history.addInsert(cur, s);
            if (cur.hasSelection()) {
                advance -= cursorDeleteSelection(&cur);
            }
            cursorInsertText(&cur, s);
            cursorMovePosition(&cur, cursor_t::Right, false, s.length());
            if (s == " " || s == "\t") {
                doc->history.mark();
            }
            advance += s.length();

            doc->updateCursor(cur);
            doc->update();

            for (int j = 0; j < cursors.size(); j++) {
                struct cursor_t& c = cursors[j];
                if (c.position > 0 && c.position + advance > cur.position && c.uid != cur.uid) {
                    c.position += advance;
                    c.anchorPosition += advance;
                }
                doc->updateCursor(c);
            }
        }
    }

    endwin();

    return 0;
}
