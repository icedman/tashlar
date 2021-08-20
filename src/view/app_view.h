#include "view.h"
#include "app.h"

struct app_view_t : view_t {
    app_view_t();
    ~app_view_t();

    bool input(char ch, std::string keys) override;
    void applyTheme() override;
};