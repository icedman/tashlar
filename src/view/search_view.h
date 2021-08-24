#ifndef SEARCH_VIEW_H
#define SEARCH_VIEW_H

#include "view.h"
#include "view_controls.h"

struct search_view_t : view_t {
    search_view_t();
    ~search_view_t();

    void preLayout() override;
    void update(int delta) override;
    void layout(int x, int y, int w, int h) override;
    void render() override;
    void applyTheme() override;
    bool input(char ch, std::string keys) override;
    
    virtual void onInput() override;
    virtual void onSubmit() override;

    text_view_t textfind;
    view_t wrapfind;
    
    inputtext_view_t inputtext;
    list_view_t list;
    int searchDirection;
    bool _findNext;
};

#endif // SEARCH_VIEW_H