#ifndef ITEM_H
#define ITEM_H

#include "view.h"

#include <string>

struct item_view_t : view_t {
    item_view_t();
    ~item_view_t();

    std::string text;

    void update(int delta) override;
    void render() override;
    void preLayout() override;
};

#endif // ITEM_H
