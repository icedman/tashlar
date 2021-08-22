#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "view.h"
#include "editor.h"

struct list_view_t;
struct editor_view_t : view_t {
    editor_view_t(editor_ptr editor);
    ~editor_view_t();

    void ensureVisibleCursor(bool animate = false);
    void scrollToCursor(cursor_t c, bool animate = false);

    // view
    void update(int delta) override;
    void layout(int x, int y, int width, int height) override;
    void render() override;
    void preRender() override;
    bool input(char ch, std::string keys) override;
    void applyTheme() override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseDrag(int x, int y, bool within) override;

    void onInput() override;
    void onSubmit() override;
    void onFocusChanged(bool focused) override;

    editor_ptr editor;
    block_ptr firstVisibleBlock;
    block_ptr lastVisibleBlock;

    int targetX;
    int targetY;

    // todo move outside of editor
    list_view_t *completerView;
    int completerItemsWidth;
    cursor_t completerCursor;
};

#endif // EDITOR_VIEW_H