#include "gem_view.h"
#include "render.h"
#include "app.h"
#include "block.h"

#include "editor_view.h"
#include "gutter_view.h"

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

