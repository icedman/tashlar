#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <regex>
#include <sstream>

#include "app.h"
#include "command.h"
#include "editor.h"
#include "explorer.h"
#include "keybinding.h"
#include "popup.h"
#include "search.h"
#include "statusbar.h"

#define POPUP_SEARCH_WIDTH 24
#define POPUP_PALETTE_WIDTH 48
#define POPUP_HEIGHT 3
#define POPUP_MAX_HEIGHT 12

static bool compareFile(struct item_t& f1, struct item_t& f2)
{
    if (f1.score == f2.score) {
        if (f1.depth < f2.depth) {
            return f1.name < f2.name;
        }
        return f1.depth < f2.depth;
    }
    return f1.score < f2.score;
}

bool popup_t::processCommand(command_e cmd, char ch)
{
    if (!isShown()) {
        return false;
    }

    struct app_t* app = app_t::instance();

    std::string s;
    s += (char)ch;

    switch (cmd) {
    case CMD_RESIZE:
    case CMD_UNDO:
    case CMD_CANCEL:
        text = "";
        hide();
        return true;
    case CMD_ENTER:
        onSubmit();
        return true;
    case CMD_MOVE_CURSOR_UP:
        currentItem--;
        return true;
    case CMD_MOVE_CURSOR_DOWN:
        currentItem++;
        return true;
    case CMD_MOVE_CURSOR_RIGHT:
        hide();
        return true;
    case CMD_MOVE_CURSOR_LEFT:
    case CMD_DELETE:
    case CMD_BACKSPACE:
        if (text.length()) {
            text.pop_back();
            onInput();
        }
        if (type == POPUP_COMPLETION) {
            hide();
            return false;
        }
        return true;
    default:
        if (type == POPUP_COMPLETION) {
            return false;
        }
        if (isprint(ch)) {
            text += s;
            onInput();
        }
        break;
    };
    return true;
}

void popup_t::layout(int w, int h)
{
    if (type == POPUP_SEARCH || type == POPUP_COMPLETION || type == POPUP_SEARCH_LINE) {
        viewWidth = POPUP_SEARCH_WIDTH;
    } else {
        viewWidth = POPUP_PALETTE_WIDTH;
    }

    if (viewWidth > w) {
        viewWidth = w;
    }

    viewHeight = POPUP_HEIGHT + items.size();
    if (type == POPUP_COMPLETION) {
        viewHeight--;
    }

    if (viewHeight > POPUP_MAX_HEIGHT) {
        viewHeight = POPUP_MAX_HEIGHT;
    }
    if (viewHeight > h) {
        viewHeight = h;
    }

    viewX = 0; // w - viewWidth;
    viewY = 0;

    if (true ||  type != POPUP_SEARCH) {
        viewX = w / 2 - viewWidth / 2;
    } else {
        viewX = w - viewWidth;
    }

    if (type == POPUP_COMPLETION && !cursor.isNull()) {
        struct editor_t* editor = app_t::instance()->currentEditor.get();
        viewX = editor->viewX + cursor.position - cursor.block->position;
        viewY = editor->viewY + cursor.block->renderedLine + 1;
        if (viewY > (h * 2 / 3)) {
            viewY -= (viewHeight + 1);
        }
    }
}

void popup_t::render()
{
    if (!isShown()) {
        return;
    }

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    wmove(win, 1, 1);
    wclrtoeol(win);

    wmove(win, 0, 0);
    wattron(win, COLOR_PAIR(colorPair));
    box(win, ACS_VLINE, ACS_HLINE);

    if (!text.length()) {
        wmove(win, 1, 2);
        waddstr(win, placeholder.c_str());
    }

    wattroff(win, COLOR_PAIR(colorPair));

    wmove(win, 1, 1);
    int offsetX = 0;
    int x = 1;

    if (currentItem < 0) {
        currentItem = 0;
    }
    if (currentItem >= items.size()) {
        currentItem = items.size() - 1;
    }
    char* str = (char*)text.c_str();
    if (text.length() > viewWidth - 3) {
        offsetX = text.length() - (viewWidth - 3);
    }
    renderLine(str, offsetX, x);
    wattron(win, COLOR_PAIR(colorPairIndicator));
    waddch(win, '|');
    wattroff(win, COLOR_PAIR(colorPairIndicator));

    int inputOffset = 4;
    if (type == POPUP_COMPLETION) {
        inputOffset -= 2;
    }

    // scroll to cursor
    // TODO: use math not loops
    if (items.size()) {
        int viewportHeight = viewHeight - inputOffset;
        while (true) {
            break;
            int blockVirtualLine = currentItem;
            int blockScreenLine = blockVirtualLine - scrollY;
            bool lineVisible = (blockScreenLine >= 0 & blockScreenLine < viewportHeight);
            if (lineVisible) {
                break;
            }
            if (blockScreenLine >= viewportHeight) {
                scrollY++;
            }
            if (blockScreenLine <= 0) {
                scrollY--;
            }
        }
    } else {
        scrollY = 0;
    }

    int skip = scrollY;

    int idx = 0;
    int y = (inputOffset - 1);
    for (auto& item : items) {
        idx++;
        if (skip-- > 0) {
            continue;
        }
        wmove(win, y++, 1);
        x = 1;
        if (idx - 1 == currentItem) {
            wattron(win, A_REVERSE);
            if (type == POPUP_FILES) {
                app_t::instance()->statusbar->setStatus(item.fullPath, 8000);
            }
        }
        wattron(win, COLOR_PAIR(colorPair));
        renderLine(item.name.c_str(), offsetX, x);
        for (int i = x; i < viewWidth - 1; i++) {
            waddch(win, ' ');
        }
        wattroff(win, COLOR_PAIR(colorPair));
        wattroff(win, A_REVERSE);
        if (y >= viewHeight - 1) {
            break;
        }
    }

    wmove(win, 0, 0);
    wattron(win, COLOR_PAIR(colorPair));
    box(win, ACS_VLINE, ACS_HLINE);
    wattroff(win, COLOR_PAIR(colorPair));
    wrefresh(win);
}

void popup_t::renderCursor()
{
}

void popup_t::renderLine(const char* line, int offsetX, int& x)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        waddch(win, c);
        if (++x >= viewWidth - 1) {
            break;
        }
    }
}

void popup_t::hide()
{
    struct app_t* app = app_t::instance();
    app->focused = app->currentEditor.get();
    app->refresh();
}

void popup_t::search(std::string t)
{
    if (isFocused()) {
        hide();
    }
    wclear(win);
    text = t;
    type = POPUP_SEARCH;
    placeholder = "search words";

    if (t == ":") {
       placeholder = "goto line";
       text = "";
       type =  POPUP_SEARCH_LINE;
    }

    items.clear();
    app_t::instance()->focused = this;
}

void popup_t::files()
{
    if (isFocused()) {
        hide();
    }
    wclear(win);
    type = POPUP_FILES;
    placeholder = "search files";
    items.clear();
    app_t::instance()->focused = this;
}

void popup_t::commands()
{
    if (isFocused()) {
        hide();
    }
    wclear(win);
    type = POPUP_COMMANDS;
    placeholder = "enter command";
    items.clear();
    app_t::instance()->focused = this;
}

void popup_t::update(int frames)
{
    if (request > 0) {
        if ((request -= frames) <= 0) {
            showCompletion();
        }
    }
}

void popup_t::completion()
{
    request = 10000;
    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    //cursor = doc->cursor();
}

void popup_t::showCompletion()
{
    if (isFocused()) {
        hide();
    }

    std::string prefix;

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;

    struct cursor_t request_cursor = cursor;
    cursor = doc->cursor();

    struct block_t block = doc->block(cursor);

    if (doc->cursors.size() == 1) {
        if (cursorMovePosition(&cursor, cursor_t::Move::Left)) {
            cursorSelectWord(&cursor);
            prefix = cursor.selectedText();
            if (prefix.length() > 2) {
                cursor.position = cursor.anchorPosition;
            } else {
                return;
            }
        }
    } else {
        return;
    }

    wclear(win);
    // text = prefix;
    type = POPUP_COMPLETION;
    placeholder = "";

    items.clear();
    std::vector<search_result_t> res = search_t::instance()->findCompletion(prefix);
    for (int j = 0; j < res.size(); j++) {
        auto r = res[j];
        struct item_t item = {
            .name = r.text,
            .description = "",
            .fullPath = "",
            .score = 0,
            .depth = 0
        };
        // item.name += std::to_string(item.score);
        items.push_back(item);
    }

    if (items.size()) {
        app_t::instance()->focused = this;
    }
}

void popup_t::onInput()
{
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    items.clear();

    if (type == POPUP_SEARCH_LINE) {
        std::string sInput = R"(AA #-0233 338982-FFB /ADR1 2)";
        text  = std::regex_replace(text, std::regex(R"([\D])"), "");
        int line = 0;
        std::stringstream(text) >> line;
        for(auto &b : doc->blocks) {
            if (b.lineNumber == line - 1) {
                cursor.setPosition(b.position);
                doc->setCursor(cursor);
            }
        }        
    }

    if (type == POPUP_FILES) {
        if (text.length() > 2) {
            char* searchString = (char*)text.c_str();
            std::vector<struct fileitem_t*> files = app->explorer->fileList();
            for (auto f : files) {
                if (f->isDirectory) {
                    continue;
                }
                int score = levenshtein_distance(searchString, (char*)(f->name.c_str()));
                if (score > 10) {
                    continue;
                }
                struct item_t item = {
                    .name = f->name,
                    .description = "",
                    .fullPath = f->fullPath,
                    .score = score,
                    .depth = f->depth
                };
                // item.name += std::to_string(item.score);
                items.push_back(item);
            }

            currentItem = 0;
            sort(items.begin(), items.end(), compareFile);
            app->refresh();
        }
    }
}

void popup_t::onSubmit()
{
    struct app_t* app = app_t::instance();

    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    if (type == POPUP_SEARCH_LINE) {
        hide();
        return;
    }

    if (type == POPUP_SEARCH) {
        if (!text.length()) {
            hide();
            return;
        }

        bool found = cursorFindWord(&cursor, text);
        if (!found) {
            cursor.setPosition(0);
            found = cursorFindWord(&cursor, text);
        }
        if (found) {
            cursorSelectWord(&cursor);
            doc->updateCursor(cursor);

            // reverse the anchor;
            size_t p = cursor.position;
            cursor.position = cursor.anchorPosition;
            cursor.anchorPosition = p;

            doc->setCursor(cursor);
            app->refresh();
            app_t::instance()->log("found %s", text.c_str());
            return;
        }
    }

    if (type == POPUP_FILES && currentItem >= 0 && currentItem < items.size()) {
        if (!text.length()) {
            hide();
            return;
        }

        struct item_t& item = items[currentItem];
        app->log("open file %s", item.fullPath.c_str());
        app->openEditor(item.fullPath);
        app->refresh();
    }

    if (type == POPUP_COMPLETION && currentItem >= 0 && currentItem < items.size()) {
        struct item_t& item = items[currentItem];
        app->log("insert %s", item.name.c_str());
        if (cursorMovePosition(&cursor, cursor_t::Move::Left)) {
            cursorSelectWord(&cursor);
            cursorDeleteSelection(&cursor);
            cursorInsertText(&cursor, item.name);
            cursorMovePosition(&cursor, cursor_t::Move::Right, false, item.name.length());
            doc->updateCursor(cursor);
        }
        hide();
    }
}


