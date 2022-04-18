#include "popup_view.h"
#include "operation.h"
#include "app.h"
#include "util.h"

#include "render.h"
#include "view_controls.h"
#include "search_view.h"

static struct popup_root_view_t* popupRootInstance = 0;

static bool compareItem(struct item_t& f1, struct item_t& f2)
{
    if (f1.score == f2.score) {
        return f1.name < f2.name;
    }
    return f1.score < f2.score;
}

popup_root_view_t::popup_root_view_t()
	: view_t("popup_root")
{
	popupRootInstance = this;
	viewLayout = LAYOUT_STACK;
}

struct popup_root_view_t* popup_root_view_t::instance()
{
    return popupRootInstance;
}

void popup_root_view_t::pushPopup(std::shared_ptr<view_t> p)
{
    for(auto v : views) {
        if (v->name == p->name) {
            return;
        }
    }

	addView(p.get());
	popups.emplace_back(p);

    p->applyTheme();
}

void popup_root_view_t::popPopup()
{
	if (!hasPopups()) {
		return;
	}

	std::shared_ptr<view_t> top = popups.back();
	removeView(top.get());
	popups.pop_back();

    view_t::setFocus(app_t::instance()->currentEditor->view);
}

bool popup_root_view_t::hasPopups()
{
	return popups.size() > 0;
}

bool popup_root_view_t::input(char ch, std::string keys)
{
    operation_e cmd = operationFromKeys(keys);

    if (ch == 27) {
        cmd = CANCEL;
        keys = "";
    }

    switch (cmd) {

    case QUIT:
    case CLOSE:
    case CANCEL:
    	log("cancel");
    	if (hasPopups()) {
        	popPopup();
        	log("close popup");
        	return true;
    	}
        return false;

    case POPUP_SEARCH_LINE:
    case POPUP_SEARCH: {
        log("search");
        std::shared_ptr<search_view_t> search = std::make_shared<search_view_t> ();
        if (cmd == POPUP_SEARCH_LINE) {
            search->inputtext.text = ":";
        }
        pushPopup(search);
        return true;
    }

        //     case INSERT: {
        //     return false;
        // }

    // case POPUP_SEARCH_LINE:
    //     popup.search(":");
    //     return true;
    // case POPUP_COMMANDS:
    //     popup.commands();
    //     return true;
    // case POPUP_FILES:
    //     popup.files();
    //     return true;

    }

    return view_t::input(ch, keys);
}

void popup_root_view_t::clear()
{
    while(hasPopups()) {
        popPopup();
    }
}
