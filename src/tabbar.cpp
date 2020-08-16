#include "tabbar.h"
#include "app.h"
#include "editor.h"
#include "keybinding.h"

#include <algorithm>

#define TABBAR_HEIGHT 1

tabbar_t::tabbar_t()
    : view_t("tabbar")
{
    preferredHeight = 1;
    canFocus = true;
}

void tabbar_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
}

bool tabbar_t::input(char ch, std::string keys)
{
    struct app_t* app = app_t::instance();

    operation_e cmd = operationFromKeys(keys);

    switch (cmd) {
    case TAB_1:
    case TAB_2:
    case TAB_3:
    case TAB_4:
    case TAB_5:
    case TAB_6:
    case TAB_7:
    case TAB_8:
    case TAB_9: {
        int idx = cmd - TAB_1;
        if (idx < tabs.size()) {
            app->openEditor(tabs[idx].editor->document.fullPath);
        }
        return true;
    }
    case NEW_TAB:
        app->openEditor("./");
        return true;
    default:
        break;
    }

    // proceed only if got focus
    if (!isFocused()) {
        return false;
    }

    switch (cmd) {
    case MOVE_CURSOR_LEFT:
    case MOVE_CURSOR_RIGHT: {
        std::vector<struct tabitem_t>::iterator it = tabs.begin();
        std::vector<struct tabitem_t>::iterator prev = it;
        std::vector<struct tabitem_t>::iterator next = it;
        while (it != tabs.end()) {
            if (it != tabs.begin()) {
                prev = it - 1;
            }
            if (it + 1 != tabs.end()) {
                next = it + 1;
            }
            if (it->editor == app->currentEditor) {
                if (cmd == MOVE_CURSOR_LEFT) {
                    it = prev;
                } else {
                    it = next;
                }
                if (it != tabs.end()) {
                    app->openEditor(it->editor->document.fullPath);
                    view_t::setFocus(this);
                }
                return true;
            }
            it++;
        }
        break;
    };
    case ENTER:
    case MOVE_CURSOR_UP:
    case MOVE_CURSOR_DOWN:
        view_t::setFocus(app->currentEditor.get());
        return true;
    default:
        break;
    }

    return false;
}

bool tabbar_t::isVisible()
{
    return visible && app_t::instance()->showTabbar;   
}
