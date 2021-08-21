#include "app_view.h"
#include "render.h"
#include "app.h"
#include "gem_view.h"

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
        view_t::setFocus(NULL);
        view_t::setHovered(NULL);
        bool found = false;

        app_t *app = app_t::instance();
        view_t *mainContainer = view_t::getMainContainer();

        view_list::iterator it = mainContainer->views.begin();
        while (it != mainContainer->views.end()) {
            gem_view_t* gem = (gem_view_t*)*it;
            if (gem->editor == app->currentEditor) {
                found = true;
                mainContainer->views.erase(it);

                editor_list::iterator eit = app->editors.begin();
                editor_ptr prev = 0;
                while(eit != app->editors.end()) {
                    if (*eit == app->currentEditor) {
                        removeGem(app->currentEditor);
                        app->editors.erase(eit);
                        break;
                    }
                    prev = *eit;
                    eit++;
                }

                if (app->editors.size() == 0) {
                    app->end = true;
                    return true;
                }
                break;
            }

            it++;
        }

        if (found) {
            gem_view_t *gem = 0;
            for(auto g : gemList()) {
                if (g->isVisible()) {
                    gem = g.get();
                    break;
                }
            }

            if (!gem && gemList().size()) {
                gem = gemList().back().get();
            }

            if (gem) {
                gem->setVisible(true);
                app_t::instance()->currentEditor = gem->editor;
                view_t::setFocus(gem->editor->view);
            }
        }

        return true;
    }

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
{
    view_t::applyTheme();
}

void app_view_t::update(int delta) {
    gatherGems();
    view_t::update(delta);
}
