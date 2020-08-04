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

    editor_ptr editor;
};

struct tabbar_t : view_t {

    tabbar_t();

    void render() override;
    void applyTheme() override;
    bool input(char ch, std::string keys) override;

    std::vector<struct tabitem_t> tabs;
};

#endif // TABBAR_H
