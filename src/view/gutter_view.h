#ifndef GUTTER_VIEW_H
#define GUTTER_VIEW_H

#include "view.h"
#include "editor.h"

struct gutter_view_t : view_t {
    gutter_view_t(editor_ptr editor);
    ~gutter_view_t();

    void update(int delta) override;
    void preLayout() override;
    void render() override;
    void applyTheme() override;

    bool isVisible() override;

    editor_ptr editor;
};

#endif // GUTTER_VIEW_H