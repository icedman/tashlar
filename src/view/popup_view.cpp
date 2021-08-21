#include "popup_view.h"
#include "operation.h"
#include "app.h"
#include "util.h"

#include "render.h"
#include "view_controls.h"

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
}

bool popup_root_view_t::hasPopups()
{
	return popups.size() > 0;
}

bool popup_root_view_t::input(char ch, std::string keys)
{
    operation_e cmd = operationFromKeys(keys);

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

    case POPUP_FILES: {
        log("files");
        std::shared_ptr<list_view_t> files = std::make_shared<list_view_t> ();

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
            files->items.push_back(item);
        }

        pushPopup(files);
        return true;
    }
    case POPUP_SEARCH: {
        log("search");
        std::shared_ptr<inputtext_view_t> search = std::make_shared<inputtext_view_t> ();
        search->placeholder = "input";
        pushPopup(search);
        return true;
    }

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
