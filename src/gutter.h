#ifndef GUTTER_H
#define GUTTER_H

#include "editor.h"
#include "view.h"

struct gutter_t : view_t {

    gutter_t();
    ~gutter_t();

    // view
    /*
    void update(int delta) override;
    void layout(int x, int y, int width, int height) override;
    bool input(char ch, std::string keys) override;
    */

    void applyTheme() override;
    void render() override;
    void preLayout() override;

    editor_ptr editor;
};

#endif // GUTTER_H
