#ifndef GUTTER_H
#define GUTTER_H

#include "editor.h"
#include "view.h"

struct gutter_t : view_t {

    gutter_t();
    ~gutter_t();

    // view
    void applyTheme() override;
    void render() override;
    void preLayout() override;
    bool isVisible() override;

    editor_ptr editor;
};

#endif // GUTTER_H
