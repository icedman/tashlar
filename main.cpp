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
#include "util.h"

#include "extension.h"

#include "statusbar.h"
#include "gutter.h"
#include "explorer.h"

static void renderEditor(struct editor_t& editor)
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
    editor.viewHeight = w.ws_row;

    if (app_t::instance()->showStatusBar) {
        editor.viewHeight--;
    }
    if (app_t::instance()->showSidebar) {
        int explorerWidth = app_t::instance()->explorer->viewWidth;
        editor.viewWidth -= explorerWidth;
        editor.viewX += explorerWidth;
    }

    if (app_t::instance()->showGutter) {
        int gutterWidth = app_t::instance()->gutter->viewWidth;
        editor.viewWidth -= gutterWidth;
        editor.viewX += gutterWidth;
    }

    if (!editor.win) {
        editor.win = newwin(editor.viewHeight, editor.viewWidth, 0, 0);
    }

    mvwin(editor.win, editor.viewY, editor.viewX);
    wresize(editor.win, editor.viewHeight, editor.viewWidth);

    int offsetX = 0;
    int offsetY = 0;
    editor.cursorScreenX = 0;
    editor.cursorScreenY = 0;

    // layout blocks
    // update block positions only when needed
    // TODO: perpetually improve (update only changed)
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
    // TODO: use math not loops
    while (true) {
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
        if (app_t::instance()->lineWrap)
            break;
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

static void renderCursor(struct editor_t& editor)
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
    struct editor_t _editor;
    struct gutter_t gutter;
    struct statusbar_t statusbar;
    struct explorer_t explorer;

    struct app_t app;
    app.statusbar = &statusbar;
    app.gutter = &gutter;
    app.explorer = &explorer;

    char* filename = 0;
    if (argc > 1) {
        filename = argv[argc - 1];
    } else {
        return 0;
    }

    app.configure(argc, argv);

    _editor.lang = language_from_file(filename, app.extensions);
    _editor.theme = app.theme;
    _editor.document.open(filename);

    explorer.files.load(filename);

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

    int ch = 0;
    bool end = false;

    std::string previousKeySequence;
    while (!end) {

        app.currentEditor = &_editor;
        
        struct editor_t& editor = *app.currentEditor;
        struct document_t* doc = &editor.document;
        struct cursor_t cursor = doc->cursor();
        struct block_t block = doc->block(cursor);

        bool disableRefresh = app.commandBuffer.size() || app.inputBuffer.length();

        if (!disableRefresh) {
            doc->update();
            
            renderExplorer(explorer);
            renderGutter(gutter);
            renderEditor(editor);
            renderStatus(statusbar);
            renderCursor(editor);

            curs_set(0);
            wrefresh(statusbar.win);
            wrefresh(explorer.win);
            wrefresh(gutter.win);
            wrefresh(editor.win);
            curs_set(1);
        }

        //-----------------
        // get input
        //-----------------
        command_e cmd = CMD_UNKNOWN;
        std::string keySequence;
        std::string expandedSequence;
        int sequenceTick = 0;
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

                if (previousKeySequence.length()) {
                    expandedSequence = previousKeySequence + "+" + keySequence;
                    command_e exCmd = commandKorKeys(expandedSequence);
                    statusbar.setStatus(expandedSequence, 2000);
                    if (exCmd != CMD_UNKNOWN) {
                        cmd = exCmd;
                    }
                }

                previousKeySequence = keySequence;
                keySequence = ""; // always consume
                sequenceTick = 2500;
                ch = 0;
            }

            if (ch != -1) {
                break;
            }

            if (statusbar.tick(150)) {
                break;
            }

            if (sequenceTick > 0 && (sequenceTick -= 150) < 0) {
                previousKeySequence = "";
            }
        }

        doc->history().paused = app.inputBuffer.length() || app.commandBuffer.size();
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

        if (ch == ESC) {
            cmd = CMD_CANCEL;
        }
        
        //-----------------
        // app commands
        //-----------------
        switch (cmd) {
        case CMD_TOGGLE_EXPLORER:
            app.showSidebar = !app.showSidebar;
            renderEditor(editor);
            continue;
            
        case CMD_CANCEL:
            doc->clearCursors();
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

            doc->history().addInsert(cur, s);
            if (cur.hasSelection()) {
                advance -= cursorDeleteSelection(&cur);
            }
            cursorInsertText(&cur, s);
            cursorMovePosition(&cur, cursor_t::Right, false, s.length());
            if (s == " " || s == "\t") {
                doc->history().mark();
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
