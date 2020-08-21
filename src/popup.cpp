#include <algorithm>
#include <regex>
#include <sstream>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "keybinding.h"
#include "keyinput.h"
#include "popup.h"
#include "render.h"
#include "scripting.h"
#include "search.h"
#include "statusbar.h"

#define POPUP_SEARCH_WIDTH 24
#define POPUP_PALETTE_WIDTH 40
#define POPUP_HEIGHT 1
#define POPUP_MAX_HEIGHT 12

static bool compareItem(struct item_t& f1, struct item_t& f2)
{
    if (f1.score == f2.score) {
        return f1.name < f2.name;
    }
    return f1.score < f2.score;
}

static struct popup_t* popupInstance = 0;

struct popup_t* popup_t::instance()
{
    return popupInstance;
}

popup_t::popup_t()
    : view_t("popup")
    , currentItem(-1)
    , historyIndex(0)
    , request(0)
{
    setVisible(false);
    canFocus = true;
    floating = true;
    flex = 0;

    popupInstance = this;
    backgroundColor = 3;
}

void popup_t::layout(int _x, int _y, int w, int h)
{

    if (type == POPUP_SEARCH || type == POPUP_COMPLETION || type == POPUP_SEARCH_LINE) {
        width = POPUP_SEARCH_WIDTH;
    } else {
        width = POPUP_PALETTE_WIDTH;
    }

    height = POPUP_HEIGHT + items.size();
    if (type == POPUP_COMPLETION) {
        height--;
    }

    if (height > POPUP_MAX_HEIGHT) {
        height = POPUP_MAX_HEIGHT;
    }

    cols = width;
    rows = height;
    
    app_t::log(">>%d", rows);

    width *= render_t::instance()->fw;
    height *= render_t::instance()->fh;

    x = 0;
    y = 0;

    if (true || type != POPUP_SEARCH) {
        x = w / 2 - width / 2;
    } else {
        x = w - width;
    }

    if (type == POPUP_COMPLETION && !cursor.isNull()) {
        struct editor_t* editor = app_t::instance()->currentEditor.get();
        x = cursor.position();
        y = cursor.block()->screenLine + 1;
        y -= editor->scrollY;

        y *= render_t::instance()->fh;
        x += editor->x;
        y += editor->y;
        bool reverse = false;
        if (y > (editor->height * 2 / 3)) {
            y -= height;
            reverse = true;
            if (x + width >= w) {
                y += 2;
            }
        }

        while (x + width >= w) {
            x -= editor->width;
            y += reverse ? -1 : 1;
        }
    }
}

void popup_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    style_t style = theme->styles_for_scope("function");
    colorPrimary = pairForColor(style.foreground.index, true);
    colorIndicator = pairForColor(app->tabActiveBorder, true);
}

bool popup_t::input(char ch, std::string keys)
{
    if (!isVisible()) {
        return false;
    }

    operation_e cmd = operationFromKeys(keys);

    struct app_t* app = app_t::instance();

    std::string s;
    s += (char)ch;

    if (currentItem < 0) {
        currentItem = 0;
    }

    switch (cmd) {
    case PASTE:
        text = app_t::instance()->clipboard();
        return true;
    // case RESIZE:
    case UNDO:
    case CANCEL:
        text = "";
        hide();
        return true;
    case ENTER:
        searchDirection = 0;
        onSubmit();
        return true;
    case MOVE_CURSOR_UP:
        searchDirection = 1;
        currentItem--;
        if (type == POPUP_SEARCH) {
            onSubmit();
        }
        return true;
    case MOVE_CURSOR_DOWN:
        searchDirection = 0;
        currentItem++;
        if (type == POPUP_SEARCH) {
            onSubmit();
        }
        return true;
    case MOVE_CURSOR_RIGHT:
        hide();
        return true;
    case MOVE_CURSOR_LEFT:
    case DELETE:
    case BACKSPACE:
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
            const char* endCompletionChars = ";, ()[]{}-?!.";
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

        if (ch == K_ESC) {
            hide();
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

void popup_t::hide()
{
    if (!isVisible())
        return;
    view_t::setFocus(app_t::instance()->currentEditor.get());
    setVisible(false);
}

void popup_t::prompt(std::string _placeholder)
{
    if (isFocused()) {
        hide();
    }

    text = "";
    type = POPUP_PROMPT;
    placeholder = _placeholder;

    items.clear();
    view_t::setFocus(this);
    setVisible(true);
}

void popup_t::search(std::string t)
{
    if (isFocused()) {
        hide();
    }

    text = t;
    type = POPUP_SEARCH;
    placeholder = "search words";

    if (t == ":") {
        placeholder = "jump to line";
        text = "";
        type = POPUP_SEARCH_LINE;
    }

    items.clear();
    view_t::setFocus(this);
    setVisible(true);
}

void popup_t::files()
{
    if (isFocused()) {
        hide();
    }
    text = "";

    type = POPUP_FILES;
    placeholder = "search files";
    items.clear();
    view_t::setFocus(this);
    setVisible(true);
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

    type = POPUP_COMMANDS;
    placeholder = "enter command";
    items.clear();
    setVisible(true);
}

void popup_t::update(int delta)
{
    if (request > 0) {
        // app_t::log(">%d" ,request);
        if ((request -= delta) <= 0) {
            showCompletion();
            app_t::instance()->refresh();
        }
    }
}

void popup_t::completion()
{
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    request = 1000;
    // showCompletion();
}

void popup_t::showCompletion()
{
    if (isVisible()) {
        hide();
    }

    std::string prefix;

    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;

    if (doc->cursors.size() > 1) {
        return;
    }

    cursor = doc->cursor();

    struct block_t& block = *cursor.block();

    if (cursor.position() < 3)
        return;

    if (cursor.moveLeft(1)) {
        cursor.selectWord();
        prefix = cursor.selectedText();
        cursor.cursor = cursor.anchor;
        cursor.cursor.position--;
    }

    if (prefix.length() < 2) {
        hide();
        return;
    }

    // text = prefix;
    type = POPUP_COMPLETION;
    placeholder = "";

    int prevSize = items.size();
    items.clear();
    currentItem = 0;

    app_t::log("prefix: %s", prefix.c_str());

    std::vector<std::string> res = editor->completer.findWords(prefix);
    for (auto s : res) {
        if (s.length() <= prefix.length() + 1)
            continue;

        int score = levenshtein_distance((char*)prefix.c_str(), (char*)(s.c_str()));
        struct item_t item = {
            .name = s,
            .description = "",
            .fullPath = "",
            .score = score,
            .depth = 0,
            .script = ""
        };

        // item.name += std::to_string(item.score);
        items.push_back(item);
    }

    if (items.size()) {
        sort(items.begin(), items.end(), compareItem);
        setVisible(true);
    }
}

void popup_t::onInput()
{
    struct app_t* app = app_t::instance();
    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    block_ptr block = cursor.block();

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
        for (auto b : doc->blocks) {
            if (b->lineNumber == line - 1) {
                cursor.setPosition(b, 0);
                cursor.clearSelection();
                doc->setCursor(cursor);
                editor->preLayout();
                break;
            }
        }
    }

    if (type == POPUP_FILES) {
        if (text.length() > 2) {
            char* searchString = (char*)text.c_str();
            explorer_t::instance()->preloadFolders();
            std::vector<struct fileitem_t*> files = explorer_t::instance()->fileList();
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

            currentItem = 0;
            sort(items.begin(), items.end(), compareItem);
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

            currentItem = 0;
            sort(items.begin(), items.end(), compareItem);
        }
    }

    if (type == POPUP_PROMPT) {
        // hide();
    }
}

void popup_t::onSubmit()
{
    struct app_t* app = app_t::instance();

    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    if (currentItem < 0) {
        currentItem = 0;
    }

    if (type == POPUP_SEARCH_LINE) {
        hide();
        return;
    }

    if (type == POPUP_SEARCH) {
        if (!text.length()) {
            hide();
            return;
        }

        searchHistory.push_back(text);
        historyIndex = 0;

        struct cursor_t cur = cursor;
        bool found = cur.findWord(text, searchDirection);
        if (!found) {
            if (searchDirection == 0) {
                cur.moveStartOfDocument();
            } else {
                cur.moveEndOfDocument();
            }
            found = cur.findWord(text, searchDirection);
        }

        if (found) {
            std::ostringstream ss;
            ss << (cur.block()->lineNumber + 1);
            ss << ":";
            ss << cur.anchorPosition();
            editor->pushOp(MOVE_CURSOR, ss.str());
            ss.str("");
            ss.clear();
            ss << (cur.block()->lineNumber + 1);
            ss << ":";
            ss << cur.position();
            editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
            return;
        } else {
            statusbar_t::instance()->setStatus("no match found", 2500);
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
            hide();
        }
    }

    if (type == POPUP_COMPLETION && currentItem >= 0 && currentItem < items.size()) {
        struct item_t& item = items[currentItem];
        editor->pushOp(MOVE_CURSOR_LEFT);
        editor->pushOp(SELECT_WORD);
        editor->pushOp(DELETE_SELECTION);
        editor->pushOp(INSERT, item.name.c_str());
        hide();
    }

    if (type == POPUP_COMMANDS) {

        if (text.length()) {

            commandHistory.push_back(text);
            historyIndex = 0;

            if (text[0] == ':') {
                std::string script = text;
                script.erase(script.begin(), script.begin() + 1);
                scripting_t::instance()->runScript(script);
            } else if (items.size() && currentItem >= 0 && currentItem < items.size()) {
                struct item_t& item = items[currentItem];
                scripting_t::instance()->runScript(item.script);
            }
        }
        hide();
    }

    if (type == POPUP_PROMPT) {
        app_t::log("on prompt");
        doc->fileName = text;
        doc->filePath += text;
        editor->highlighter.lang = language_from_file(text, app_t::instance()->extensions);
        for (auto b : doc->blocks) {
            b->data = nullptr;
        }
        doc->save();
        hide();
    }
}

bool popup_t::isCompletion()
{
    return type == POPUP_COMPLETION && isVisible();
}
