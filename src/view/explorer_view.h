#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "view.h"
#include "explorer.h"

struct explorer_view_t : view_t {
    explorer_view_t();
    ~explorer_view_t();

    // view
    void update(int delta) override;
    void render() override;
    void applyTheme() override;
    bool input(char ch, std::string keys) override;
    void preLayout() override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseHover(int x, int y) override;
    void onFocusChanged(bool focused) override;

    void ensureVisibleCursor();
};

#endif // EXPLORER_VIEW_H