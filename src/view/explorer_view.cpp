#include "explorer_view.h"
#include "explorer.h"
#include "render.h"
#include "app.h"
#include "util.h"
#include "block.h"
#include "render.h"

#include "gem_view.h"
#include "scrollbar_view.h"

#define EXPLORER_WIDTH 20

explorer_view_t::explorer_view_t()
	: view_t("explorer")
{
    preferredWidth = EXPLORER_WIDTH;
    viewLayout = LAYOUT_HORIZONTAL;

	preferredHeight = 1;
    backgroundColor = 2;

    canFocus = true;
    
    addView(&spacer);
    createScrollbars();
}

explorer_view_t::~explorer_view_t()
{}

void explorer_view_t::update(int delta)
{
	explorer_t::instance()->update(delta);
    view_t::update(delta);
}

bool explorer_view_t::isVisible()
{
    return visible && app_t::instance()->showSidebar;
}

void explorer_view_t::preLayout()
{
    if (width == 0 || height == 0 || !isVisible())
        return;

	explorer_t *explorer = explorer_t::instance();

    view_t::preLayout();

    preferredWidth = EXPLORER_WIDTH * getRenderer()->fw;
    if (!getRenderer()->isTerminal()) {
        preferredWidth += (padding * 2);
    }

    scrollbar_view_t *scrollbar = verticalScrollbar;

    if (scrollbar->scrollTo >= 0 && scrollbar->scrollTo < scrollbar->maxScrollY && scrollbar->maxScrollY > 0) {
        scrollY = scrollbar->scrollTo;
        scrollbar->scrollTo = -1;
    }

    maxScrollY = (explorer->renderList.size() - rows + 1) * getRenderer()->fh;
    scrollbar->setVisible(maxScrollY > rows && !getRenderer()->isTerminal());
    scrollbar->scrollY = scrollY;
    scrollbar->maxScrollY = maxScrollY;
    scrollbar->colorPrimary = colorPrimary;

    int sz = explorer->renderList.size();
    if (sz == 0) sz = 1;
    scrollbar->thumbSize = rows / sz;
    if (scrollbar->thumbSize < 2) {
        scrollbar->thumbSize = 2;
    }

    if (!scrollbar->isVisible() && !getRenderer()->isTerminal()) {
        scrollY = 0;
    }
}


static void renderLine(const char* line, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        _addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}

void explorer_view_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();

    editor_ptr editor = app_t::instance()->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    std::vector<std::string> openPaths;

    for(auto gem : gemList()) {
        if (gem->isVisible()) {
            openPaths.push_back(gem->editor->document.fullPath);
        }
    }

    explorer_t *explorer = explorer_t::instance();

    _move(0, 0);

    bool isHovered = view_t::currentHovered() == this;
    bool hasFocus = (isFocused() || isHovered) && !view_t::currentDragged();

    int idx = 0;
    int skip = (scrollY / getRenderer()->fh);
    // log("??%d %d %d", scrollY, skip, getRenderer()->fh);

    int y = 0;
    for (auto file : explorer->renderList) {
        if (skip-- > 0) {
            idx++;
            continue;
        }

        int pair = colorPrimary;
        _move(y, 0);
        _attron(_color_pair(pair));

        if (hasFocus && explorer->currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _reverse(true);
            } else {
                _bold(true);
            }
            for (int i = 0; i < cols; i++) {
                _addch(' ');
            }
        }

        for(auto o : openPaths) {
            if (file->fullPath == o) {
                pair = colorIndicator;
            }
        }

        // if (file->fullPath == app_t::instance()->currentEditor->document.fullPath) {
        //     pair = colorIndicator;
        // }

        int x = 0;
        _move(y++, x);
        _attron(_color_pair(pair));
        int indent = file->depth;
        for (int i = 0; i < indent; i++) {
            _addch(' ');
            x++;
        }
        if (file->isDirectory) {
            _attron(_color_pair(colorIndicator));

#ifdef ENABLE_UTF8
            _addwstr(file->expanded ? L"\u2191" : L"\u2192");
#else
            _addch(file->expanded ? '-' : '+');
#endif

            _attroff(_color_pair(colorIndicator));
            _attron(_color_pair(pair));
        } else {
            _addch(' ');
        }
        _addch(' ');
        x += 2;

        renderLine(file->name.c_str(), x, cols);

        if (hasFocus && explorer->currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _bold(true);
            } else {
                _bold(true);
            }
        }
        for (int i = 0; i < cols - x; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(pair));
        _bold(false);
        _reverse(false);

        if (y >= rows + 1) {
            break;
        }

        idx++;
    }

    while (y < rows) {
        _move(y++, x);
        _clrtoeol(cols);
    }

    view_t::render();
}

void explorer_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->treeFg, false);

    for (auto view : views) {
        view->colorPrimary = colorPrimary;
        view->colorIndicator = colorIndicator;
    }

    view_t::applyTheme();
}

bool explorer_view_t::input(char ch, std::string keys)
{
    if (!isFocused()) {
        return false;
    }

    explorer_t *explorer = explorer_t::instance();

    struct app_t* app = app_t::instance();
    struct fileitem_t* item = NULL;

    if (explorer->currentItem != -1) {
        item = explorer->renderList[explorer->currentItem];
    }

    operation_e cmd = operationFromKeys(keys);

    bool _scrollToCursor = true;

    int keyMods = _keyMods();

    switch (cmd) {
    case MOVE_CURSOR_RIGHT:
    case ENTER:
        if (item && item->isDirectory) {
            item->expanded = !item->expanded || cmd == MOVE_CURSOR_RIGHT;
            log("expand/collapse folder %s", item->fullPath.c_str());
            if (item->canLoadMore) {
                item->load();
                item->canLoadMore = false;
            }
            explorer->regenerateList = true;
            return true;
        }
        if (item && cmd == ENTER) {
            log("open file %s", item->fullPath.c_str());
            focusGem(app->openEditor(item->fullPath), !_isShiftDown());

        }
        return true;
    case MOVE_CURSOR_LEFT:
        if (item && item->isDirectory && item->expanded) {
            item->expanded = false;
            explorer->regenerateList = true;
            return true;
        }
        if (item) {
            explorer->currentItem = parentItem(item, explorer->renderList)->lineNumber;
        }
        break;
    case MOVE_CURSOR_UP:
        explorer->currentItem--;
        if (explorer->currentItem < 0) {
            explorer->currentItem = 0;
        }
        break;
    case MOVE_CURSOR_DOWN:
        explorer->currentItem++;
        if (explorer->currentItem >= explorer->renderList.size()) {
            explorer->currentItem = explorer->renderList.size() - 1;
        }
        break;
    case MOVE_CURSOR_START_OF_DOCUMENT:
        explorer->currentItem = 0;
        break;
    case MOVE_CURSOR_END_OF_DOCUMENT:
        explorer->currentItem = explorer->renderList.size() - 1;
        break;
    case MOVE_CURSOR_PREVIOUS_PAGE:
        explorer->currentItem -= height;
        break;
    case MOVE_CURSOR_NEXT_PAGE:
        explorer->currentItem += height;
        if (explorer->currentItem >= explorer->renderList.size()) {
            explorer->currentItem = explorer->renderList.size() - 1;
        }
        break;
    default:
        _scrollToCursor = false;
        break;
    }

    if (_scrollToCursor) {
        ensureVisibleCursor();
    }

    if (explorer->currentItem != -1) {
        struct fileitem_t* nextItem = explorer->renderList[explorer->currentItem];
        if (nextItem != item) {
            // statusbar_t::instance()->setStatus(nextItem->fullPath, 3500);
            return true;
        }
    }

    return false;
}

void explorer_view_t::mouseDown(int x, int y, int button, int clicks)
{
	explorer_t *explorer = explorer_t::instance();

    int prevItem = explorer->currentItem;
    int fh = getRenderer()->fh;
    explorer->currentItem = ((y - this->y - padding) / fh) + (scrollY / fh);
    if (explorer->currentItem < 0 || explorer->currentItem >= explorer->renderList.size()) {
        explorer->currentItem = -1;
    }
    if (clicks > 0) {
        view_t::setFocus(this);
    }
    if (clicks > 1 || (clicks == 1 && prevItem == explorer->currentItem)) {
        input(0, "enter");
        view_t::setFocus(this);
    }
}

void explorer_view_t::mouseHover(int x, int y)
{
    mouseDown(x, y, 1, 0);
}

void explorer_view_t::onFocusChanged(bool focused)
{
	explorer_t *explorer = explorer_t::instance();
    if (focused && explorer->currentItem == -1 && explorer->renderList.size()) {
        explorer->currentItem = 0;
    }
}

void explorer_view_t::ensureVisibleCursor()
{
	explorer_t *explorer = explorer_t::instance();
    int fh = getRenderer()->fh;

    if (explorer->renderList.size()) {
        int current = explorer->currentItem;
        if (current < 0)
            current = 0;
        if (current >= explorer->renderList.size())
            current = explorer->renderList.size() - 1;
        int viewportHeight = rows - 0;

        while (explorer->renderList.size() > 4) {
            int blockVirtualLine = current;
            int blockScreenLine = blockVirtualLine - (scrollY / fh);

            log("b:%d v:%d s:%d %d", blockScreenLine, viewportHeight, scrollY / fh, current);

            if (blockScreenLine >= viewportHeight) {
                scrollY += fh;
            } else if (blockScreenLine <= 0) {
                scrollY -= fh;
            } else {
                break;
            }
        }
    } else {
        scrollY = 0;
    }
}

void explorer_view_t::scroll(int s)
{
    view_t::scroll(s * 8);
}
