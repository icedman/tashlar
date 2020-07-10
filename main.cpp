#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "app.h"

#include "command.h"
#include "cursor.h"
#include "document.h"
#include "editor.h"
#include "keybinding.h"
#include "statusbar.h"
#include "util.h"

#include "extension.h"

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
    statusbar.viewWidth =w.ws_col;
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
    
    int offsetX = 0;
    int offsetY = 0;
    editor.cursorScreenX = 0;
    editor.cursorScreenY = 0;

    // layout blocks
    // update block positions only when needed
    // todo!
    {
        struct block_t* prev = NULL;
        int l = 0;
        size_t pos = 0;
        for (auto& b : editor.document.blocks) {
            editor.layoutBlock(b);
            b.screenLine = 0;
            if (prev) {
                b.screenLine = prev->screenLine + prev->lineCount;
                prev->next = &b;
            }
            prev = &b;
        }
    }

    // scroll to cursor
    while(true) {
        int blockVirtualLine = block.screenLine > block.lineNumber ? block.screenLine : block.lineNumber;
        int blockScreenLine = blockVirtualLine - editor.scrollY;
        bool lineVisible = (blockScreenLine >= 0 & blockScreenLine < editor.viewHeight); 
        if (lineVisible) {
            break;
        }
        if (blockScreenLine >= editor.viewHeight) {
            editor.scrollY++;
        }
        if (blockScreenLine <= 0) {
            editor.scrollY--;
        }
    }
    
    while (cursor.position - block.position + 1 - editor.scrollX > editor.viewWidth) {
        if (app_t::instance()->lineWrap) break;
        if (editor.scrollX + 1 >= block.length) {
            editor.scrollX = 0;
            break;
        }
        editor.scrollX++;
    }
    while (editor.scrollX > 0 && cursor.position - block.position - editor.scrollX <= 0) {
        editor.scrollX--;
    }

    offsetY = editor.scrollY;
    offsetX = editor.scrollX;

    //-----------------
    // render the editor
    //-----------------
    // todo: jump to first visible block
    int y = 0;
    for (auto& b : doc->blocks) {
        if (offsetY > 0) {
            offsetY -= b.lineCount;
            continue;
        }
        editor.highlightBlock(b);
        editor.renderBlock(b, offsetX, y);
        y += b.lineCount;
        
        if (y >= editor.viewHeight) {
            break;
        }
    }
    while (y < editor.viewHeight) {
        wmove(editor.win, y++, 0);
        wclrtoeol(editor.win);
    }
}


void renderCursor(struct editor_t& editor)
{
    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (block.isValid()) {
        wmove(editor.win, editor.cursorScreenY, editor.cursorScreenX);
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
    app.lineWrap = true;

    app.configure();
    
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
    raw();
    noecho();

    app.setupColors();

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
            doc->update();
            
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
        std::string expandedSequence;
        while (true) {
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
                    expandedSequence = previousKeySequence + "+" + keySequence;
                    cmd = commandKorKeys(expandedSequence);
                    statusbar.setStatus(expandedSequence, 2000);
                    previousKeySequence = "";
                }

                if (cmd != CMD_UNKNOWN || expandedSequence.length()) {
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

            if (statusbar.tick(150)) {
                break;
            }
        }

        doc->history.paused = app.inputBuffer.length();
        if (app.inputBuffer.length()) {
            ch = app.inputBuffer[0];
            if (ch == '\n') {
                ch = ENTER;
            }

            app.inputBuffer.erase(0, 1);
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
