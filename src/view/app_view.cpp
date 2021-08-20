#include "app_view.h"
#include "render.h"
#include "app.h"

app_view_t::app_view_t()
    : view_t("app")
{
	viewLayout = LAYOUT_VERTICAL;
	view_t::setRoot(this);
}

app_view_t::~app_view_t()
{}

bool app_view_t::input(char ch, std::string keys)
{
    operation_e cmd = operationFromKeys(keys);

    // process popups
    // if (!enablePopup) {
    //     switch(cmd) {
    //     case POPUP_SEARCH:
    //     case POPUP_SEARCH_LINE:
    //     case POPUP_COMMANDS:
    //     case POPUP_FILES:
    //         return true;
    //     }
    // }

    switch (cmd) {
    case QUIT:
        app_t::instance()->end = true;
        return true;
    case CLOSE: {
    	/*
        view_t::setFocus(NULL);
        view_t::setHovered(NULL);
        bool found = false;
        view_list::iterator it = tabContent.views.begin();
        while (it != tabContent.views.end()) {
            gem_t* gem = (gem_t*)*it;
            if (gem->editor == currentEditor) {
                found = true;
                tabContent.views.erase(it);
                if (!tabContent.views.size()) {
                    end = true;
                    return true;
                }
                break;
            }
            it++;
        }
        gem_list::iterator it2 = editors.begin();
        while (it2 != editors.end()) {
            gem_ptr gem = *it2;
            if (gem->editor == currentEditor) {
                editors.erase(it2);
                break;
            }
            it2++;
        }
        if (found) {
            gem_t* gem = (gem_t*)(tabContent.views.front());
            editor_ptr nextEditor = gem->editor;
            app_t::instance()->openEditor(nextEditor->document.filePath);
        }
        */
        return true;
    }

    // case CANCEL:
    //     popup.hide();
    //     return false;
    // case POPUP_SEARCH:
    //     popup.search("");
    //     return true;
    // case POPUP_SEARCH_LINE:
    //     popup.search(":");
    //     return true;
    // case POPUP_COMMANDS:
    //     popup.commands();
    //     return true;
    // case POPUP_FILES:
    //     popup.files();
    //     return true;

    case MOVE_FOCUS_LEFT:
        view_t::shiftFocus(-1, 0);
        return true;
    case MOVE_FOCUS_RIGHT:
        // if (explorer.isFocused()) {
        //     view_t::setFocus(currentEditor.get());
        // } else {
        //     view_t::shiftFocus(1, 0);
        // }
        return true;
    case MOVE_FOCUS_UP:
        view_t::shiftFocus(0, -1);
        return true;
    case MOVE_FOCUS_DOWN:
        view_t::shiftFocus(0, 1);
        return true;
    default:
        break;
    }

    return view_t::input(ch, keys);
}

void app_view_t::applyTheme()
{}
