#include "gem_view.h"
#include "render.h"
#include "app.h"
#include "block.h"

#include "editor_view.h"
#include "gutter_view.h"
#include "scrollbar_view.h"

static gem_view_list gems;

gem_view_t::gem_view_t(editor_ptr editor)
	: view_t("gem")
	, editor(editor)
{
	gutterView = std::make_shared<gutter_view_t>(editor);
	editorView = std::make_shared<editor_view_t>(editor);
	minimapView = std::make_shared<minimap_view_t>(editor);

	addView(gutterView.get());
	addView(editorView.get());
	addView(minimapView.get());

	createScrollbars();

	applyTheme();
}

gem_view_t::~gem_view_t()
{}

void gem_view_t::update(int delta)
{
    if (!isVisible()) {
        return;
    }
    
    view_t::update(delta);

    scrollbar_view_t *scrollbar = verticalScrollbar;
    if (scrollbar->scrollTo >= 0 && scrollbar->scrollTo < scrollbar->maxScrollY) {
        editor->view->scrollY = scrollbar->scrollTo;
        scrollbar->scrollTo = -1;
    }
    scrollbar->setVisible(!getRenderer()->isTerminal() && editor->view->maxScrollY > editor->view->rows);
    scrollbar->scrollY = editor->view->scrollY;
    scrollbar->maxScrollY = editor->view->maxScrollY;
    scrollbar->colorPrimary = minimapView->colorPrimary;

    scrollbar->thumbSize = rows / editor->document.blocks.size();
    if (scrollbar->thumbSize < 2) {
        scrollbar->thumbSize = 2;
    }
}

void gatherGems()
{
    for(auto e : app_t::instance()->editors) {
        if (e->view == 0) {
            gem_view_ptr gem = std::make_shared<gem_view_t>(e);
            gems.emplace_back(gem);
            view_t::getMainContainer()->addView(gem.get());
            view_t::setFocus(e->view);
        }
    }
}

void removeGem(editor_ptr editor)
{
    gem_view_list::iterator it = gems.begin();
    while(it != gems.end()) {
        gem_view_ptr gem = *it;
        if (gem->editor == editor) {
            gems.erase(it);
            return;
        }
        it++;
    }
}

gem_view_list gemList()
{
	return gems;
}

void focusGem(editor_ptr editor, bool solo)
{
	for(auto g : gemList()) {
		if (solo) {
			g->setVisible(false);
		}
		if (g->editor == editor) {
        	g->setVisible(true);
        	view_t::setFocus(g->editor->view);
    	}
    }
}
