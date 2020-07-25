#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <locale.h>

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
#include "scripting.h"
#include "search.h"
#include "util.h"

#include "extension.h"

#include "explorer.h"
#include "gutter.h"
#include "minimap.h"
#include "popup.h"
#include "statusbar.h"
#include "tabbar.h"
#include "window.h"

#include "dots.h"

void renderEditor(struct editor_t& editor)
{
    struct document_t* doc = &editor.document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    if (!editor.win) {
        editor.win = newwin(editor.viewHeight, editor.viewWidth, 0, 0);
    }

    editor.matchBracketsUnderCursor();

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
            b.lineNumber = l++;
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
        long blockVirtualLine = block.screenLine > block.lineNumber ? block.screenLine : block.lineNumber;
        long blockScreenLine = blockVirtualLine - editor.scrollY;

        // app_t::instance()->log(": c:%d line:%d scroll:%d text:%s", cursor.position(), block.lineNumber, editor.scrollY, block.text().c_str());
        if (block.lineNumber >= editor.document.blocks.size()) {
            app_t::instance()->log("invalid block");
            break;
        }

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

    while (cursor.position() - block.position + 1 - editor.scrollX > editor.viewWidth) {
        if (app_t::instance()->lineWrap)
            break;
        if (editor.scrollX + 1 >= block.length) {
            editor.scrollX = 0;
            break;
        }
        editor.scrollX++;
    }
    while (editor.scrollX > 0 && cursor.position() - block.position - editor.scrollX <= 0) {
        editor.scrollX--;
    }

    if (editor.viewHeight > editor.document.blocks.size()) {
        editor.scrollY = 0;
    }

    offsetY = editor.scrollY;
    offsetX = editor.scrollX;

    //-----------------
    // render the editor
    //-----------------
    // todo: jump to first visible block
    int y = 0;

    // app_t::instance()->log("render blocks");
    for (auto& b : doc->blocks) {
        if (offsetY > 0) {
            offsetY -= b.lineCount;
            continue;
        }

        editor.highlightBlock(b);
        struct blockdata_t* data = b.data.get();
        if (data && data->folded && data->foldedBy != 0) {
            // render folded indicator
        } else {
            b.renderedLine = y;
            editor.renderBlock(b, offsetX, y);
            y += b.lineCount;
        }

        if (y >= editor.viewHeight) {
            break;
        }
    }
    while (y < editor.viewHeight) {
        wmove(editor.win, y++, 0);
        wclrtoeol(editor.win);
    }

    wrefresh(editor.win);
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    struct editor_proxy_t _editor;
    struct tabbar_t tabbar;
    struct gutter_t gutter;
    struct statusbar_t statusbar;
    struct minimap_t minimap;
    struct explorer_t explorer;
    struct popup_t popup;

    struct search_t search;
    struct scripting_t scripting;
    struct app_t app;

    app.statusbar = &statusbar;
    app.gutter = &gutter;
    app.tabbar = &tabbar;
    app.explorer = &explorer;
    app.minimap = &minimap;
    app.popup = &popup;

    app.windows.push_back(&statusbar);
    app.windows.push_back(&explorer);
    app.windows.push_back(&tabbar);
    app.windows.push_back(&gutter);
    app.windows.push_back(&_editor);
    app.windows.push_back(&minimap);

    app.windows.push_back(&popup);

    const char* defaultFile = "./newfile";
    char* filename = (char*)defaultFile;
    if (argc > 1) {
        filename = argv[argc - 1];
    } else {
        return 0;
    }

    app.configure(argc, argv);

    app.openEditor(filename);
    explorer.setRootFromFile(filename);

    //-------------------
    // scripting
    //-------------------
    scripting_t::instance()->initialize();

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
    nodelay(stdscr, true);

    clear();

    app.setupColors();

    //-------------------
    // run script
    //-------------------
    if (app.scriptPath.size()) {
        app.layout();
        scripting_t::instance()->runFile(app.scriptPath);
    }

    bool end = false;
    std::string previousKeySequence;
    while (!end) {

        struct editor_t& editor = *app.currentEditor;
        struct document_t* doc = &editor.document;
        struct cursor_t cursor = doc->cursor();
        struct block_t& block = *cursor.block();

        bool disableRefresh = app.commandBuffer.size() || app.inputBuffer.length();
        if (!disableRefresh) {
            doc->update();
            app.layout();

            curs_set(0);

            if (popup.isFocused() && (popup.items.size() || popup.text.length() > 3) && app.refreshLoop <= 0) {
                popup.render();
            } else {
                app.render();
            }

            if (app.refreshLoop > 0) {
                app.refreshLoop--;
                continue;
            }
        }

        //-----------------
        // get input
        //-----------------
        command_t cmdt;
        command_e cmd = CMD_UNKNOWN;
        std::string keySequence;
        std::string expandedSequence;
        int sequenceTick = 0;
        int ch = -1;

        app.resetIdle();

        while (true) {
            if (app.inputBuffer.length()) {
                break;
            }
            if (app.commandBuffer.size()) {
                cmdt = app.commandBuffer.front();
                cmd = cmdt.cmd;
                app.commandBuffer.erase(app.commandBuffer.begin());
                /*
                if (cmdt.cmd == CMD_INSERT) {
                    app.inputBuffer = cmdt.args;
                }
                */
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

            app.update(150);
            // scripting.update(150);

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

        //-----------------
        // morph some keys
        //-----------------
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
        case CMD_UNDO:
            popup.hide();
            popup.request = -1;
            popup.items.clear();
            break;

        case CMD_CLOSE_TAB:
            app.close();
            if (app.editors.size() == 0) {
                end = true;
                continue;
            }
            app.refresh();
            continue;
        case CMD_FOCUS_WINDOW_LEFT:
            app.focused = &explorer;
            continue;
        case CMD_FOCUS_WINDOW_RIGHT:
            app.focused = app.currentEditor.get();
            continue;
        case CMD_FOCUS_WINDOW_UP:
            app.focused = &tabbar;
            continue;
        case CMD_FOCUS_WINDOW_DOWN:
            app.focused = app.currentEditor.get();
            continue;
        case CMD_TOGGLE_EXPLORER:
            app.showSidebar = !app.showSidebar;
            if (app.showSidebar) {
                app.focused = &explorer;
            } else {
                app.focused = app.currentEditor.get();
            }
            app.refresh();
            continue;
        case CMD_POPUP_FILES:
            popup.files();
            app.refresh();
            break;
        case CMD_POPUP_COMMANDS:
            popup.commands();
            app.refresh();
            break;
        case CMD_POPUP_SEARCH_LINE:
        case CMD_POPUP_SEARCH: {
            std::string selectedText = cursor.selectedText();
            if (cmd == CMD_POPUP_SEARCH_LINE) {
                selectedText = ":";
            }
            doc->clearCursors();
            popup.search(selectedText);
            app.refresh();
            break;
        }
        case CMD_OPEN:
            app.openEditor(cmdt.args.c_str());
            continue;

        case CMD_QUIT:
            end = true;
            continue;
        default:
            break;
        }

        //-------------------
        // update keystrokes on cursors
        //-------------------
        cmdt.cmd = cmd;
        bool commandHandled = app.processCommand(cmdt, ch);
        if (commandHandled || cmd == CMD_CANCEL) {
            continue;
        }

        if (ch < 1) {
            continue;
        }

        //-------------------
        // update keystrokes or input buffer
        //-------------------
        std::string s;
        s += (char)ch;

        if (s.length() != 1) {
            continue;
        }

        cmdt.cmd = CMD_INSERT;
        cmdt.args = s;
        app.processCommand(cmdt, ch);

        // popup completion
        if (doc->cursors.size() == 1 && s.length() > 0) {
            struct cursor_t c = doc->cursors[0];
            if (cursorMovePosition(&c, cursor_t::Move::Left)) {
                cursorSelectWord(&c);
                std::string prefix = c.selectedText();
                if (prefix.length() > 2) {
                    popup.completion();
                }
            }
        }

        continue;
    }

    endwin();

    return 0;
}
