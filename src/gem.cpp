#include "gem.h"
#include "app.h"
#include "keybinding.h"
#include "render.h"

gem_t::gem_t()
    : view_t("gutter-editor-minimap")
    , split(false)
{
    editor = std::make_shared<editor_t>();
    gutter.editor = editor;
    minimap.editor = editor;
    addView(&gutter);
    addView(editor.get());
    addView(&minimap);
    addView(&scrollbar);
}

gem_t::~gem_t()
{
}

bool gem_t::isVisible()
{
    return split || view_t::isVisible();
}

bool gem_t::input(char ch, std::string keys)
{
    if (!editor->isFocused()) {
        return false;
    }

    operation_e op = operationFromKeys(keys);
    switch (op) {
    case SPLIT_VIEW:
        split = true;
        //        app_t::log("split!");
        return true;
    default:
        break;
    }

    return view_t::input(ch, keys);
}

void gem_t::update(int delta)
{
    if (editor->isFocused()) {
        app_t::instance()->currentEditor = editor;
    }

    view_t::update(delta);
    scrollbar.setVisible(render_t::instance()->fh > 10 && editor->maxScrollY > editor->rows);
    scrollbar.scrollY = editor->scrollY;
    scrollbar.maxScrollY = editor->maxScrollY;
    scrollbar.colorPrimary = minimap.colorPrimary;
}
