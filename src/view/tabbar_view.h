#ifndef TABBAR_H
#define TABBAR_H

#include <string>
#include <vector>

#include "editor.h"
#include "view.h"

struct tabitem_t {
    std::string name;

    int itemNumber;
    int width;
    int x;

    editor_ptr editor;
};

struct tabbar_view_t : view_t {

    tabbar_view_t();

    void render() override;
    void applyTheme() override;
    bool input(char ch, std::string keys) override;
    bool isVisible() override;
    void preLayout() override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseHover(int x, int y) override;

    std::vector<struct tabitem_t> tabs;
};

#endif // TABBAR_H
