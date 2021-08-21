#include "gem_view.h"
#include "render.h"
#include "app.h"
#include "block.h"

#include "editor_view.h"
#include "gutter_view.h"

static gem_view_list gems;

gem_view_t::gem_view_t(editor_ptr editor)
	: view_t("gem")
	, editor(editor)
{
	gutterView = std::make_shared<gutter_view_t>(editor);
	editorView = std::make_shared<editor_view_t>(editor);

	addView(gutterView.get());
	addView(editorView.get());

	applyTheme();
}

gem_view_t::~gem_view_t()
{}

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
