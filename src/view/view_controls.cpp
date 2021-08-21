#include "view_controls.h"
#include "app.h"
#include "util.h"
#include "render.h"

#define POPUP_WIDTH 24
#define POPUP_HEIGHT 1
#define POPUP_MAX_WIDTH 40
#define POPUP_MAX_HEIGHT 12

list_view_t::list_view_t()
	: view_t("list")
	, currentItem(-1)
{
	backgroundColor = 3;
    canFocus = true;
}

void list_view_t::onInput()
{}

void list_view_t::onSubmit()
{}

void list_view_t::update(int delta)
{}

void list_view_t::preLayout()
{
	view_t::preLayout();

    int inputOffset = 1;

    // int inputOffset = 2;
    // if (type == POPUP_COMPLETION) {
    //     inputOffset -= 1;
    // }

    maxScrollY = 0;
    if (items.size() > 1) {
        maxScrollY = (items.size() - rows + inputOffset) * getRenderer()->fh;
    }
}

void list_view_t::layout(int _x, int _y, int w, int h)
{
    x = _x;
    y = _y;
	width = w > 0 ? w : POPUP_WIDTH;
    height = (h > 0 ? h : POPUP_HEIGHT) + items.size();

    if (width > POPUP_MAX_WIDTH) {
        width = POPUP_MAX_WIDTH;
    }
    if (height > POPUP_MAX_HEIGHT) {
        height = POPUP_MAX_HEIGHT;
    }

    int fw = getRenderer()->fw;
    int fh = getRenderer()->fh;

    width *= fw;
    height *= fh;
    if (!getRenderer()->isTerminal()) {
        width += padding * 2;
        height += padding * 2;
    }

    cols = width / fw;
    rows = height / fh;
}

static void renderLine(const char* line, int offsetX, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        if (offsetX-- > 0) {
            continue;
        }
        _addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}


void list_view_t::render()
{
    if (!isVisible()) {
        return;
    }

    app_t* app = app_t::instance();
    log("%d %d %d %d ", x, y, width, height);

    _attron(_color_pair(colorPrimary));

    for (int i = 0; i < height - 1 || i == 0; i++) {
        _move(0 + i, 0);
        _clrtoeol(cols);
    }

    int inputOffset = 1;

    _move(0, 0);
    int offsetX = 0;
    int x = 0;

    if (currentItem < 0) {
        currentItem = 0;
    }
    if (currentItem >= items.size()) {
        currentItem = items.size() - 1;
    }

    int skip = (scrollY / getRenderer()->fh);
    int idx = 0;
    int y = (inputOffset - 1);
    for (auto& item : items) {
        idx++;
        if (skip-- > 0) {
            continue;
        }
        _move(y++, 0);
        x = 1;
        if (idx - 1 == currentItem) {
            _reverse(true);
            // if (type == POPUP_FILES) {
                // app_t::instance()->statusbar->setStatus(item.fullPath, 8000);
            // }
        }
        _attron(_color_pair(colorPrimary));
        renderLine(item.name.c_str(), offsetX, x, cols);

        for (int i = x; i < cols + 1; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(colorPrimary));
        _reverse(false);
        if (y >= rows) {
            break;
        }
    }

    _attroff(_color_pair(colorIndicator));
    _attroff(_color_pair(colorPrimary));
}

bool list_view_t::input(char ch, std::string keys)
{
    if (!isVisible()) {
        return false;
    }

    bool _scrollToCursor = false;
    operation_e cmd = operationFromKeys(keys);

    struct app_t* app = app_t::instance();

    std::string s;
    s += (char)ch;

    if (currentItem < 0) {
        currentItem = 0;
    }

    switch (cmd) {
    case ENTER:
        // searchDirection = 0;
        onSubmit();
        return true;
    case MOVE_CURSOR_UP:
        // searchDirection = 1;
        currentItem--;
        _scrollToCursor = true;
        break;
    case MOVE_CURSOR_DOWN:
        // searchDirection = 0;
        currentItem++;
        _scrollToCursor = true;
        break;
    case MOVE_CURSOR_RIGHT:
        // hide();
        return true;
    case MOVE_CURSOR_LEFT:
        return true;
    default:
        break;
    };

    if (_scrollToCursor) {
        ensureVisibleCursor();
    }

    return true;
}

void list_view_t::ensureVisibleCursor()
{
	int inputOffset = 1;
	
    // int inputOffset = 2;
    // if (type == POPUP_COMPLETION) {
    //     inputOffset -= 1;
    // }

    int fh = getRenderer()->fh;

    if (items.size()) {
        if (currentItem < 0)
            currentItem = 0;
        if (currentItem >= items.size())
            currentItem = items.size() - 1;
        int viewportHeight = rows - inputOffset;

        while (items.size() > 4) {
            int blockVirtualLine = currentItem;
            int blockScreenLine = blockVirtualLine - (scrollY / fh);

            // log("b:%d v:%d s:%d %d", blockScreenLine, viewportHeight, scrollY / fh, currentItem);

            if (blockScreenLine > viewportHeight) {
                scrollY += fh;
            } else if (blockScreenLine <= inputOffset) {
                scrollY -= fh;
            } else {
                break;
            }
        }
    } else {
        scrollY = 0;
    }
}

void list_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("function");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

inputtext_view_t::inputtext_view_t()
	: view_t("inputtext")
{
    backgroundColor = 2;
}

void inputtext_view_t::layout(int _x, int _y, int w, int h)
{
    x = _x;
    y = _y;
    width = w > 0 ? w : POPUP_WIDTH;
    height = 1; // (h > 0 ? h : POPUP_HEIGHT) + items.size();

    if (width > POPUP_MAX_WIDTH) {
        width = POPUP_MAX_WIDTH;
    }
    // if (height > POPUP_MAX_HEIGHT) {
    //     height = POPUP_MAX_HEIGHT;
    // }

    int fw = getRenderer()->fw;
    int fh = getRenderer()->fh;

    width *= fw;
    height *= fh;
    if (!getRenderer()->isTerminal()) {
        width += padding * 2;
        height += padding * 2;
    }

    cols = width / fw;
    rows = height / fh;
}

void inputtext_view_t::render()
{
    _move(0, 0);

    _attron(_color_pair(colorPrimary));

    int offsetX = 0;
    int x = 0;

    char* str = (char*)text.c_str();
    if (text.length() > cols - 3) {
        offsetX = text.length() - (cols - 3);
    };

    renderLine(str, offsetX, x, cols);

    _attron(_color_pair(colorIndicator));
    _addch('|');
    _attroff(_color_pair(colorIndicator));
    _attron(_color_pair(colorPrimary));

    if (!text.length()) {
        _move(0, 0);
        _addstr(placeholder.c_str());
        return;
    }

}

bool inputtext_view_t::input(char ch, std::string keys)
{
    if (!isVisible()) {
        return false;
    }

    bool _scrollToCursor = false;
    operation_e cmd = operationFromKeys(keys);

    struct app_t* app = app_t::instance();

    std::string s;
    s += (char)ch;

    switch (cmd) {
    case PASTE:
        text = app_t::instance()->clipboard();
        return true;
    // case RESIZE:
    case UNDO:
    case CANCEL:
        text = "";
        // hide();
        return true;
    case ENTER:
        // searchDirection = 0;
        onSubmit();
        return true;
    case MOVE_CURSOR_RIGHT:
    case MOVE_CURSOR_LEFT:
        return true;
    case DELETE:
    case BACKSPACE:
        if (text.length()) {
            text.pop_back();
            onInput();
        }
        return true;
    default:
        if (isprint(ch)) {
            text += s;
            onInput();
        }
        break;
    };

    return true;
}

void inputtext_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("function");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}