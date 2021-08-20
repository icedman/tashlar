#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "view.h"
#include "editor.h"

struct editor_view_t : view_t {
    editor_view_t(editor_ptr editor);
    ~editor_view_t();

    void ensureVisibleCursor();

    // view
    void update(int delta) override;
    void layout(int x, int y, int width, int height) override;
    void render() override;
    void preRender() override;
    bool input(char ch, std::string keys) override;
    void applyTheme() override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseDrag(int x, int y, bool within) override;

    void onFocusChanged(bool focused);

    editor_ptr editor;
};

#endif // EDITOR_VIEW_H