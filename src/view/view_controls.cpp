#include "view_controls.h"
#include "app.h"
#include "util.h"
#include "render.h"

bool compareListItem(struct item_t& f1, struct item_t& f2)
{
    if (f1.score == f2.score) {
        return f1.name < f2.name;
    }
    return f1.score < f2.score;
}

list_view_t::list_view_t()
	: view_t("list")
	, currentItem(-1)
{
	backgroundColor = 2;
}

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
    view_t::layout(_x, _y, w, h);
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
    // log("%d %d %d %d ", x, y, width, height);

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
    if ((!isVisible() || !isFocused()) && ch != 0) {
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
    if (currentItem >= items.size()) {
        currentItem = 0;
    }

    switch (cmd) {
    case ENTER:
        onSubmit();
        if (inputListener)
            inputListener->onSubmit();
        items.clear();
        setVisible(false);
        return true;
    case MOVE_CURSOR_UP:
        currentItem--;
        _scrollToCursor = true;
        break;
    case MOVE_CURSOR_DOWN:
        currentItem++;
        _scrollToCursor = true;
        break;
    case MOVE_CURSOR_RIGHT:
        items.clear();
        setVisible(false);
        return true;
    case MOVE_CURSOR_LEFT:
        return true;
    default:
        break;
    };

    if (_scrollToCursor) {
        ensureVisibleCursor();
        return true;
    }

    return false;
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

text_view_t::text_view_t()
    : view_t("text")
{
    backgroundColor = 3;
}

void text_view_t::preLayout()
{
    preferredWidth = text.length() * getRenderer()->fw;
    preferredHeight = getRenderer()->fh;
    if (!getRenderer()->isTerminal()) {
        preferredHeight += (padding * 2);
    }
}

void text_view_t::layout(int _x, int _y, int w, int h)
{
    int fw = getRenderer()->fw;
    int fh = getRenderer()->fh;

    int ww = w > preferredWidth * fw ? w : preferredWidth * fw;
    view_t::layout(_x, _y, ww, preferredHeight);
}

void text_view_t::render()
{
    _move(0, 0);

    _attron(_color_pair(colorPrimary));

    int offsetX = 0;
    int x = 0;

    char* str = (char*)text.c_str();

    renderLine(str, offsetX, x, cols);
}

void text_view_t::applyTheme()
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

void inputtext_view_t::preLayout()
{
    preferredHeight = getRenderer()->fh;
    if (!getRenderer()->isTerminal()) {
        preferredHeight += (padding * 2);
    }
}

void inputtext_view_t::layout(int _x, int _y, int w, int h)
{
    int fw = getRenderer()->fw;
    int fh = getRenderer()->fh;

    int ww = w > preferredWidth * fw ? w : preferredWidth * fw;
    view_t::layout(_x, _y, ww, preferredHeight);
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
    if (!isVisible() || !isFocused()) {
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
        if (inputListener)
            inputListener->onSubmit();
        return true;
    case MOVE_CURSOR_RIGHT:
        return true;
    case MOVE_CURSOR_LEFT:
    case DELETE:
    case BACKSPACE:
        if (text.length()) {
            text.pop_back();
            onInput();
            if (inputListener)
                inputListener->onInput();
        }
        return true;
    default:
        if (isprint(ch)) {
            text += s;
            onInput();
            if (inputListener)
                inputListener->onInput();
        }
        break;
    };

    return true;
}

void inputtext_view_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}