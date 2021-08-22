#include "editor_view.h"
#include "render.h"
#include "app.h"
#include "util.h"

#include "view_controls.h"
#include "search.h"
#include "indexer.h"

#include <algorithm>

void editor_view_t::render()
{	
	if (!isVisible()) {
		return;
	}

    document_t *doc = &editor->document;
    cursor_list cursors = doc->cursors;
    cursor_t mainCursor = doc->cursor();

	editor->highlight(scrollY, rows);

	block_list::iterator it= doc->blocks.begin();
    if (scrollY > 0) {
        it += scrollY;
    }

    int l = 0;

    firstVisibleBlock = NULL;
    lastVisibleBlock = NULL;

    bool hlMainCursor = cursors.size() == 1 && !mainCursor.hasSelection();
    bool firstLine = true;
    while (it != doc->blocks.end()) {
        auto& b = *it++;
        if (l >= rows + 1)
            break;

        if (firstVisibleBlock == NULL) {
            firstVisibleBlock = b;
        }
        lastVisibleBlock = b;

        int colorPair = color_pair_e::NORMAL;
        int colorPairSelected = color_pair_e::SELECTED;

        size_t lineNo = b->lineNumber;
        struct blockdata_t* blockData = b->data.get();

        std::string text = b->text() + " ";

        char* line = (char*)text.c_str();
        for (int sl = 0; sl < b->lineCount; sl++) {
        	// printf("\n"); l++;
            _move(l, 0);
            _clrtoeol(cols);
            _move(l++, 0);

            if (text.length() < scrollX) {
                continue;
            }

            if (blockData && blockData->folded && !blockData->foldable) {
                l--;
                editor->_foldedLines += b->lineCount;
                continue;
            }

            if (l >= rows + 1)
                break;

            // log("%s", line);
            int col = 0;
            for (int i = scrollX;; i++) {
                size_t pos = i + (sl * cols);
                if (col++ >= cols) {
                    break;
                }

                char ch = line[pos];
                if (ch == 0)
                    break;

                bool hl = false;
                bool ul = false;
                bool bl = false; // bold
                bool il = false; // italic

                colorPair = color_pair_e::NORMAL;
                colorPairSelected = color_pair_e::SELECTED;

                // syntax here
                if (blockData) {
                    struct span_info_t span = spanAtBlock(blockData, pos);
                    bl = span.bold;
                    il = span.italic;
                    if (span.length) {
                        colorPair = pairForColor(span.colorIndex, false);
                        colorPairSelected = pairForColor(span.colorIndex, true);
                    }
                }

                // bracket
                if (editor->cursorBracket1.bracket != -1 && editor->cursorBracket2.bracket != -1) {
                    if ((pos == editor->cursorBracket1.position && lineNo == editor->cursorBracket1.line) ||
                    	(pos == editor->cursorBracket2.position && lineNo == editor->cursorBracket2.line)) {
                        _underline(true);
                    }
                }

                // cursors
                for (auto& c : cursors) {
                    if (pos == c.position() && b == c.block()) {
                        hl = true;
                        ul = c.hasSelection();
                        bl = c.hasSelection();

                        // the completer
                        if (c.position() == mainCursor.position()) {
                            int offsetCol = col - (mainCursor.position() - completerCursor.position());
                            int ww = (3 + completerItemsWidth);
                            if (ww > 20) {
                                ww = 20;
                            }
                            int hh = completerView->items.size() + 1;
                            if (hh > 10) {
                                hh = 10;
                            }
                            int pad = (getRenderer()->isTerminal() ? 0 : (padding * 2));
                            ww = ww * getRenderer()->fw - pad;
                            hh = hh * getRenderer()->fh - (pad * 3);
                            int yy = this->y + (l * getRenderer()->fh) + pad; 
                            if (yy > this->height / 1.8) {;
                                yy = this->y + ((l - 1) * getRenderer()->fh) - hh;
                            }
                            offsetCol --;
                            if (completerView && completerView->isVisible()) {
                                completerView->layout(
                                    this->x + (offsetCol * getRenderer()->fw),
                                    yy,
                                    ww,
                                    hh
                                );
                            }
                        }
                    }
                    if (!c.hasSelection())
                        continue;

                    cursor_position_t start = c.selectionStart();
                    cursor_position_t end = c.selectionEnd();
                    if (b->lineNumber < start.block->lineNumber || b->lineNumber > end.block->lineNumber)
                        continue;
                    if (b == start.block && pos < start.position)
                        continue;
                    if (b == end.block && pos > end.position)
                        continue;
                    hl = true;
                    break;
                }

                if (hl) {
                    colorPair = colorPairSelected;
                }
                if (ul) {
                    _underline(true);
                }
                if (bl) {
                    _bold(true);
                }
                if (il) {
                    _italic(true);
                }

                _attron(_color_pair(colorPair));
                _addch(ch);
                _attroff(_color_pair(colorPair));
                _bold(false);
                _italic(false);
                _underline(false);
            }
        }
    }

    while (l < rows) {
        _move(l, 0);
        _clrtoeol(cols);
        // _move(l, 0);
        // _addch('~');
        l++;
    }

    view_t::render();
}

editor_view_t::editor_view_t(editor_ptr editor)
    : view_t("editor")
    , editor(editor)
    , targetX(-1)
    , targetY(-1)
    , completerView(0)
{
    editor->view = this;
    editor->view->inputListener = this;

    completerView = new list_view_t();
    completerView->backgroundColor = 1;
    completerView->inputListener = this;
    addView(completerView);
}

editor_view_t::~editor_view_t()
{
    if (completerView) {
        delete completerView;
    }
}

void editor_view_t::update(int delta)
{
    if (animating) {
        int d = targetY - scrollY;
        scrollY += d/3;
        if (d * d > 4) {
            app_t::instance()->refresh();
        } else {
            animating = false;
        }
    }
}

void editor_view_t::layout(int _x, int _y, int _w, int _h)
{
	document_t *doc = &editor->document;
    if (!doc->blocks.size()) {
        return;
    }

    x = _x;
    y = _y;
    width = _w;
    height = _h;

    rows = height / getRenderer()->fh;
    cols = width / getRenderer()->fw;

    maxScrollX = 0;
    maxScrollY = doc->lastBlock()->lineNumber - (rows * 2 / 3);

    if (maxScrollY < 0) {
        scrollY = 0;
    }
}

void editor_view_t::preRender()
{
    if (editor->_scrollToCursor) {
        ensureVisibleCursor();
        editor->_scrollToCursor = false;
    }	
}
void editor_view_t::scrollToCursor(cursor_t cursor, bool animate)
{
    if (width == 0 || height == 0 || !isVisible()) {
        return;
    }

    // update scroll
    block_ptr cursorBlock = cursor.block();
    document_t *doc = cursorBlock->document;

    int _scrollX = scrollX;
    int _scrollY = scrollY;

    int adjust = 0;
    if (config_t::instance()->lineWrap) {
        block_list::iterator it = doc->blocks.begin();
        if (scrollY > doc->blocks.size()) scrollY = doc->blocks.size() - 1;
        if (scrollY > 0) {
            it += scrollY;
        }
        int l = 0;
        while (it != doc->blocks.end()) {
            block_ptr b = *it++;
            if (b == cursorBlock)
                break;
            if (l > rows)
                break;

            if (b->lineCount > 1) {
                adjust += (b->lineCount - 1);
            }
        }
    }

    // scrollY
    int lookAheadY = 0;
    int screenY = cursorBlock->lineNumber;
    if (screenY - scrollY < lookAheadY) {
        scrollY = screenY - lookAheadY;
        if (scrollY < 0)
            scrollY = 0;
    } else if (screenY - scrollY + 1 + adjust > (rows + editor->_foldedLines)) {
        scrollY = -((rows + editor->_foldedLines) - screenY) + 1 + adjust;
    }

    if (scrollY < 0)
        scrollY = 0;

    // scrollX
    int lookAheadX = (cols / 3);
    int screenX = cursor.position();
    if (screenX - scrollX < lookAheadX) {
        scrollX = screenX - lookAheadX;
        if (scrollX < 0)
            scrollX = 0;
    }
    if (screenX - scrollX + 2 > cols) {
        scrollX = -(cols - screenX) + 2;
    }

    if (config_t::instance()->lineWrap) {
        scrollX = 0;
    }

    if (scrollY > cursorBlock->lineNumber) {
        scrollY = cursorBlock->lineNumber - (rows * 2 / 3);
        if (scrollY < 0) {
            scrollY = 0;
        }
    }

    if (animate) {
        targetX = scrollX;
        targetY = scrollY;
        scrollX = _scrollX;
        scrollY = _scrollY;
        animating = true;
    } else {
        targetX = -1;
        targetY = -1;
        animating = false;
    }
}

void editor_view_t::ensureVisibleCursor(bool animate)
{
    if (width == 0 || height == 0 || !isVisible()) {
        return;
    }

    document_t *doc = &editor->document;

    doc->setColumns(cols);

    cursor_t mainCursor = doc->cursor();
    scrollToCursor(mainCursor);
}

bool editor_view_t::input(char ch, std::string keys)
{
	if (!isVisible() || !isFocused()) {
		return false;
	}

    if (completerView->isVisible() && completerView->items.size()) {
        if (completerView->input(0, keys)) {
            return true;
        }
    }

    completerView->setVisible(false);
	bool res = editor->input(ch, keys);
	editor->runAllOps();
	return res;
}

void editor_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);

    for (auto view : views) {
        view->colorPrimary = colorPrimary;
        view->colorIndicator = colorIndicator;
        view->backgroundColor = 2;
    }
}

void editor_view_t::mouseDown(int x, int y, int button, int clicks)
{
    document_t *doc = &editor->document;

    int fw = getRenderer()->fw;
    int fh = getRenderer()->fh;

    int col = (x - this->x - padding) / fw;
    int row = (y - this->y - padding) / fh;

    // scroll
    if (clicks == 0) {
        if (row + 2 >= rows) {
            if (scrollY < maxScrollY) {
                scrollY++;
                return;
            }
        }

        if (row < 2) {
            if (scrollY > 0) {
                scrollY--;
                return;
            }
        }
    }

    block_list::iterator it = doc->blocks.begin();
    if (scrollY > 0) {
        it += scrollY;
    }
    int l = 0;
    bool rowHit = false;
    while (it != doc->blocks.end()) {
        block_ptr b = *it++;
        for (int i = 0; i < b->lineCount; i++) {
            if (l == row) {
                rowHit = true;
                // log(">%d %d", b->lineNumber, 0);
                std::ostringstream ss;
                ss << (b->lineNumber + 1);
                ss << ":";
                ss << col + (i * cols);
                if (clicks == 0 || _keyMods()) {
                    editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
                } else if (clicks == 1) {
                    editor->pushOp(MOVE_CURSOR, ss.str());
                } else {
                    editor->pushOp(MOVE_CURSOR, ss.str());
                    editor->pushOp(SELECT_WORD, "");
                }
            }
            l++;
        }
        if (l > rows)
            break;
    }

    // log("click %d %d %d", rowHit, clicks, l);
    if (!rowHit && clicks == 1) {
        if (_keyMods()) {
            editor->pushOp(MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED, "");
        } else {
            editor->pushOp(MOVE_CURSOR_END_OF_DOCUMENT, "");
        }
    }

    if (clicks > 0) {
        view_t::setFocus(this);
        // popup_t::instance()->hide();
    }
}

void editor_view_t::mouseDrag(int x, int y, bool within)
{
    if (!within)
        return;
    mouseDown(x, y, 1, 0);
}

void editor_view_t::onFocusChanged(bool focused)
{
    if (focused) {
        app_t::instance()->currentEditor = editor;
    }
}

void editor_view_t::onInput()
{
    if (!editor->indexer || !completerView) {
        return;
    }

    completerView->items.clear();
    completerView->setVisible(false);

    struct document_t* doc = &editor->document;
    if (doc->cursors.size() > 1) {
        return;
    }

    std::string prefix;

    cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();
    if (cursor.position() < 3)
        return;

    if (cursor.moveLeft(1)) {
        cursor.selectWord();
        prefix = cursor.selectedText();
        cursor.cursor = cursor.anchor;
        completerCursor = cursor;
    }

    if (prefix.length() < 2) {
        return;
    }

    completerItemsWidth = 0;
    std::vector<std::string> words = editor->indexer->findWords(prefix);
    for(auto w : words) {
        if (w.length() <= prefix.length()) {
            continue;
        }
        int score = levenshtein_distance((char*)prefix.c_str(), (char*)(w.c_str()));
        completerView->items.push_back({ w, "", "", score, 0, "" });
        if (completerItemsWidth < w.length()) {
            completerItemsWidth = w.length();
        }
    }
    sort(completerView->items.begin(), completerView->items.end(), compareListItem);
    completerView->setVisible(completerView->items.size());
    
}

void editor_view_t::onSubmit()
{
    if (!editor->indexer || !completerView) {
        return;
    }

    if (completerView->currentItem >= 0 && completerView->currentItem < completerView->items.size()) {
        cursor_t cur = editor->document.cursor();
        struct item_t item = completerView->items[completerView->currentItem];
        std::ostringstream ss;
            ss << (completerCursor.block()->lineNumber + 1);
            ss << ":";
            ss << completerCursor.position();
            log(">>%s", ss.str().c_str());
            editor->pushOp(MOVE_CURSOR, ss.str());
            ss.str("");
            ss.clear();
            ss << (cur.block()->lineNumber + 1);
            ss << ":";
            ss << cur.position();
            log(">>%s", ss.str().c_str());
            editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
            editor->pushOp(INSERT, item.name);
            editor->runAllOps();
            ensureVisibleCursor(true);
        // log(">>%s", item.name.c_str());
    }

    completerView->items.clear();
    completerView->setVisible(false);

}