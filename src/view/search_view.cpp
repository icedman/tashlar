#include "search_view.h"
#include "render.h"
#include "app.h"
#include "util.h"
#include "editor.h"

#include "popup_view.h"
#include "editor_view.h"

search_view_t::search_view_t()
	: view_t("search")
	, searchDirection(0)
    , _findNext(false)
{
	viewLayout = LAYOUT_VERTICAL;

	textfind.text = "Find:";
    textfind.flex = 7;
    inputtext.flex = POPUP_WIDTH - textfind.flex;

    wrapfind.viewLayout = LAYOUT_HORIZONTAL;
    wrapfind.addView(&textfind);
    wrapfind.addView(&inputtext);

    list.inputListener = this;
    inputtext.inputListener = this;

    backgroundColor = 1;

	addView(&wrapfind);
    addView(&list);

    view_t::setFocus(&inputtext);

    // list.setVisible(false);

    /*
	for(int i=0; i<20; i++) {
		std::string text = "command ";
		text += ('a' + i);
		struct item_t item = {
            .name = text,
            .description = "",
            .fullPath = text,
            .score = i,
            .depth = 0,
            .script = text
        };
        list.items.push_back(item);
    }
    list.items.push_back({ "main", "", "", 0, 0, "" });
    */
}

search_view_t::~search_view_t()
{}

void search_view_t::update(int delta)
{
	view_t::update(delta);
}

void search_view_t::preLayout()
{
    wrapfind.preferredHeight = getRenderer()->fh;
    if (!getRenderer()->isTerminal()) {
        wrapfind.preferredHeight += (padding * 2);
    }
    list.setVisible(list.items.size());
}

void search_view_t::layout(int _x, int _y, int w, int h)
{
    int ww = POPUP_WIDTH * getRenderer()->fw;
    int hh = getRenderer()->fh;
    if (list.items.size() && list.isVisible()) {
        hh += list.items.size() * getRenderer()->fh;
    }

    if (!getRenderer()->isTerminal()) {
        hh += (padding * 2);
    }
    // view_t::layout(w / 2 - ww /2, _y, ww, hh);
    // view_t::layout(w / 2 - ww /2, getRenderer()->fh * 3, ww, hh);
    view_t::layout(w - ww, _y, ww, hh);
}

void search_view_t::render()
{
	view_t::render();
}

void search_view_t::onInput()
{
    if (inputtext.text.length() < 2) {
        return;
    }

	struct app_t* app = app_t::instance();

    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

	std::string text = inputtext.text;

    // goto line
    if (inputtext.text[0] == ':') {
        // std::string sInput = R"(AA #-0233 338982-FFB /ADR1 2)";
        // text = std::regex_replace(text, std::regex(R"([\D])"), "");
        if (inputtext.text.length()) {
            char lastChar = inputtext.text.back();
            if (lastChar < '0' || lastChar > '9') {
                inputtext.text.pop_back();
            }
        } else {
            return;
        }

        text = (inputtext.text.c_str() + 1);
        std::ostringstream ss;
        ss << text;
        ss << ":";
        ss << 0;
        editor->pushOp(MOVE_CURSOR, ss.str());
        editor->runAllOps();
        ((editor_view_t*)editor->view)->ensureVisibleCursor(true);
        return;
    }

    // searchHistory.push_back(text);
    // historyIndex = 0;

    if (text.length() < 3) {
        return;
    }

    struct cursor_t cur = cursor;
    if (!_findNext) {
        cur.moveLeft(text.length());
    }
    _findNext = false;

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
        editor->runAllOps();
        ((editor_view_t*)editor->view)->ensureVisibleCursor(true);
        return;
    } else {
        // statusbar_t::instance()->setStatus("no match found", 2500);
    }
}

void search_view_t::onSubmit()
{
    if (list.isFocused() && list.currentItem > 0) {
        inputtext.text = list.items[list.currentItem].name;
        _findNext = true;
        onInput();
        return;
    }

    _findNext = true;
    onInput();
}

void search_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);

    view_t::applyTheme();
}

bool search_view_t::input(char ch, std::string keys)
{
    if (!isVisible()) {
        return false;
    }

    operation_e cmd = operationFromKeys(keys);

    switch(cmd) {
        case MOVE_CURSOR_UP: {
            if (list.isFocused() && list.currentItem <= 0) {
                view_t::setFocus(&inputtext);
            }
            break;
        }
        case MOVE_CURSOR_DOWN: {
            if (!list.isFocused()) {
                view_t::setFocus(&list);
            }
            break;
        }
    }
    bool res = view_t::input(ch, keys);

    if (list.isFocused() && !res) {
        view_t::setFocus(&inputtext);
        return view_t::input(ch, keys);
    }
    return res;
}