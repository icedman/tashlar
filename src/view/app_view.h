#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "view.h"
#include "app.h"

struct app_view_t : view_t {
    app_view_t();
    ~app_view_t();

    void update(int delta);
    bool input(char ch, std::string keys) override;
    void applyTheme() override;
};

#endif // APP_VIEW_H