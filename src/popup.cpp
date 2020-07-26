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
#include "scripting.h"
#include "search.h"
#include "statusbar.h"

#define POPUP_SEARCH_WIDTH 24
#define POPUP_PALETTE_WIDTH 48
#define POPUP_HEIGHT 3
#define POPUP_MAX_HEIGHT 12

static bool compareFile(struct item_t& f1, struct item_t& f2)
{
    if (f1.score == f2.score) {
        return f1.name < f2.name;
    }
    return f1.score < f2.score;
}

bool popup_t::processCommand(command_t cmdt, char ch)
{
    if (!isFocused()) {
        return false;
    }

    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();

    std::string s;
    s += (char)ch;

    switch (cmd) {
    case CMD_PASTE:
        text = app_t::instance()->clipBoard;
        return true;
    case CMD_RESIZE:
    case CMD_UNDO:
    case CMD_CANCEL:
        text = "";
        hide();
        return true;
    case CMD_ENTER:
        searchDirection = 0;
        onSubmit();
        return true;
    case CMD_MOVE_CURSOR_UP:
        searchDirection = 1;
        currentItem--;
        if (type == POPUP_SEARCH) {
            onSubmit();
        }
        return true;
    case CMD_MOVE_CURSOR_DOWN:
        searchDirection = 0;
        currentItem++;
        if (type == POPUP_SEARCH) {
            onSubmit();
        }
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
            const char* endCompletionChars = ";, ()[]{}-";
            char c;
            int idx = 0;
            while (endCompletionChars[idx]) {
                if (ch == endCompletionChars[idx]) {
                    hide();
                    return false;
                }
                idx++;
            }

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
    } else {
        if (items.size()) {
            viewHeight++;
        }
    }

    if (viewHeight > POPUP_MAX_HEIGHT) {
        viewHeight = POPUP_MAX_HEIGHT;
    }
    if (viewHeight > h) {
        viewHeight = h;
    }

    viewX = 0; // w - viewWidth;
    viewY = 0;

    if (true || type != POPUP_SEARCH) {
        viewX = w / 2 - viewWidth / 2;
    } else {
        viewX = w - viewWidth;
    }

    if (type == POPUP_COMPLETION && !cursor.isNull()) {
        struct editor_t* editor = app_t::instance()->currentEditor.get();
        viewX = editor->viewX + cursor.relativePosition();
        viewY = editor->viewY + cursor.block()->renderedLine + 1;

        bool reverse = false;
        if (viewY > (h * 2 / 3)) {
            viewY -= (viewHeight + 1);
            reverse = true;
            if (viewX + viewWidth >= w) {
                viewY += 2;
            }
        }

        while (viewX + viewWidth >= w) {
            viewX -= editor->viewWidth;
            viewY += reverse ? -1 : 1;
        }
    }
}

void popup_t::render()
{
    if (!isFocused()) {
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
    wattroff(win, COLOR_PAIR(colorPair));

    // box(win, ' ', ' ');

    if (!text.length()) {
        wmove(win, 1, 2);
        waddstr(win, placeholder.c_str());
    }

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
    wattron(win, COLOR_PAIR(colorPair));

    int inputOffset = 4;
    if (type == POPUP_COMPLETION) {
        inputOffset -= 2;
    }

    // app_t::instance()->log("items: %d inputOffset: %d", items.size(), inputOffset);
    // scroll to cursor
    // TODO: use math not loops
    if (items.size()) {
        int viewportHeight = viewHeight - inputOffset;

        // app_t::instance()->log("items:%d current:%d scroll:%d h:%d", items.size(), currentItem, scrollY, viewportHeight);
        while (true) {
            int blockVirtualLine = currentItem;
            int blockScreenLine = blockVirtualLine - scrollY;
            if (blockScreenLine >= viewportHeight) {
                scrollY++;
            } else if (blockScreenLine <= 0) {
                scrollY--;
            } else {
                break;
            }
            break;
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

    // wmove(win, 0, 0);
    // wattron(win, COLOR_PAIR(colorPair));
    // box(win, ACS_VLINE, ACS_HLINE);
    // box(win, ' ', '-');
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
        placeholder = "jump to line";
        text = "";
        type = POPUP_SEARCH_LINE;
    }

    items.clear();
    app_t::instance()->focused = this;
}

void popup_t::files()
{
    if (isFocused()) {
        hide();
    }
    text = "";
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

    if (!commandItems.size()) {

        for (auto ext : app_t::instance()->extensions) {
            Json::Value contribs = ext.package["contributes"];

            if (!contribs.isMember("themes")) {
                continue;
            }

            Json::Value themes = contribs["themes"];
            for (int i = 0; i < themes.size(); i++) {
                Json::Value theme = themes[i];
                struct item_t item = {
                    .name = "Theme: " + theme["label"].asString(),
                    .description = "",
                    .fullPath = theme["path"].asString(),
                    .score = 0,
                    .depth = 0,
                    .script = "app.theme(\"" + theme["label"].asString() + "\")"
                };

                commandItems.push_back(item);
            }
        }
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
    if (app_t::instance()->inputBuffer.length()) {
        return;
    }

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

    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;

    struct cursor_t request_cursor = cursor;
    cursor = doc->cursor();

    struct block_t& block = *cursor.block();

    if (doc->cursors.size() == 1) {
        if (cursorMovePosition(&cursor, cursor_t::Move::Left)) {
            cursorSelectWord(&cursor);
            // cursor.flipAnchor();
            prefix = cursor.selectedText();
            if (prefix.length() > 2) {
                // cursor.position = cursor.anchorPosition;
            } else {
                request = 0;
                hide();
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

    int prevSize = items.size();
    items.clear();
    std::vector<search_result_t> res = search_t::instance()->findCompletion(prefix);
    for (int j = 0; j < res.size(); j++) {
        auto r = res[j];
        struct item_t item = {
            .name = r.text,
            .description = "",
            .fullPath = "",
            .score = 0,
            .depth = 0,
            .script = ""
        };
        // item.name += std::to_string(item.score);
        items.push_back(item);
    }

    if (items.size()) {
        currentItem = -1;
        app->focused = this;
        if (prevSize != items.size()) {
            app->layout();
            app->render();
            app->refresh();
        }
    }
}

void popup_t::onInput()
{
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    items.clear();

    if (type == POPUP_SEARCH_LINE) {
        // std::string sInput = R"(AA #-0233 338982-FFB /ADR1 2)";
        // text = std::regex_replace(text, std::regex(R"([\D])"), "");
        if (text.length()) {
            char lastChar = text.back();
            if (lastChar < '0' || lastChar > '9') {
                text.pop_back();
            }
        }

        int line = 0;
        std::stringstream(text) >> line;
        for (auto& b : doc->blocks) {
            if (b.lineNumber == line - 1) {
                cursor.setPosition(&b, b.position);
                cursor.clearSelection();
                doc->setCursor(cursor);
            }
        }
    }

    if (type == POPUP_FILES) {
        if (text.length() > 2) {
            char* searchString = (char*)text.c_str();
            app->explorer->preloadFolders();
            std::vector<struct fileitem_t*> files = app->explorer->fileList();
            for (auto f : files) {
                if (f->isDirectory || f->name.length() < text.length()) {
                    continue;
                }
                int score = levenshtein_distance(searchString, (char*)(f->name.c_str()));
                if (score > 20) {
                    continue;
                }
                struct item_t item = {
                    .name = f->name,
                    .description = "",
                    .fullPath = f->fullPath,
                    .score = score + f->depth,
                    .depth = f->depth,
                    .script = ""
                };
                // item.name += std::to_string(item.score);
                items.push_back(item);
            }

            currentItem = -1;
            sort(items.begin(), items.end(), compareFile);
            app->refresh();
        }
    }

    if (type == POPUP_COMMANDS) {
        if (text.length() > 2 && text[0] != ':') {
            char* searchString = (char*)text.c_str();
            for (auto cmd : commandItems) {
                int score = levenshtein_distance(searchString, (char*)(cmd.name.c_str()));
                if (score > 20) {
                    continue;
                }
                struct item_t item = {
                    .name = cmd.name,
                    .description = "",
                    .fullPath = cmd.fullPath,
                    .score = score,
                    .depth = 0,
                    .script = cmd.script
                };
                // item.name += std::to_string(item.score);
                items.push_back(item);
            }

            currentItem = -1;
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
    struct block_t& block = *cursor.block();

    if (type == POPUP_SEARCH_LINE) {
        hide();
        return;
    }

    if (type == POPUP_SEARCH) {
        if (!text.length()) {
            hide();
            return;
        }

        struct cursor_t cur = cursor;
        bool found = cursorFindWord(&cursor, text, searchDirection);
        if (!found) {
            cursorMovePosition(&cursor, searchDirection == 0 ? cursor_t::Move::StartOfDocument : cursor_t::Move::EndOfDocument);
            found = cursorFindWord(&cursor, text);
        }

        if (found) {
            doc->updateCursor(cursor);
            cursor.flipAnchor();
            doc->setCursor(cursor);
            app->refresh();
            return;
        } else {
            app->statusbar->setStatus("no match found", 2500);
        }
    }

    if (type == POPUP_FILES && currentItem >= 0 && currentItem < items.size()) {
        if (!text.length()) {
            hide();
            return;
        }

        if (items.size() && currentItem >= 0 && currentItem < items.size()) {
            struct item_t& item = items[currentItem];
            // app->log("open file %s", item.fullPath.c_str());
            app->openEditor(item.fullPath);
            app->refresh();
        }
    }

    if (type == POPUP_COMPLETION && currentItem >= 0 && currentItem < items.size()) {
        struct item_t& item = items[currentItem];
        // app->log("insert %s", item.name.c_str());
        if (cursorMovePosition(&cursor, cursor_t::Move::Left)) {
            cursorSelectWord(&cursor);
            cursorDeleteSelection(&cursor);
            cursorInsertText(&cursor, item.name);
            cursorMovePosition(&cursor, cursor_t::Move::Right, false, item.name.length());
            doc->updateCursor(cursor);
            doc->addSnapshot(); // TODO << wasteful of resources
        }
        hide();
    }

    if (type == POPUP_COMMANDS) {
        if (text.length() && text[0] == ':') {
            std::string script = text;
            script.erase(script.begin(), script.begin() + 1);
            scripting_t::instance()->runScript(script);
        } else if (items.size() && currentItem >= 0 && currentItem < items.size()) {
            struct item_t& item = items[currentItem];
            scripting_t::instance()->runScript(item.script);
        }
        hide();
    }
}
