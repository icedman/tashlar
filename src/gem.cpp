#include "gem.h"

gem_t::gem_t()
    : view_t("gutter-editor-minimap")
{
    editor = std::make_shared<editor_t>();
    gutter.editor = editor;
    minimap.editor = editor;
    addView(&gutter);
    addView(editor.get());
    addView(&minimap);
}

gem_t::~gem_t()
{
}