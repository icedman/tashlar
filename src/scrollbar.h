#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "view.h"

struct scrollbar_t : view_t {
    scrollbar_t();
    ~scrollbar_t();

    void update(int delta) override;
    void render() override;
    void preLayout() override;
    void applyTheme() override;

    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseDrag(int x, int y, bool within) override;

    int scrollTo;
};

#endif // SCROLLBAR_H
