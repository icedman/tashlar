#include "view.h"
#include "statusbar.h"

struct statusbar_view_t : view_t {
    statusbar_view_t();
    ~statusbar_view_t();

    void update(int delta) override;
    void preLayout() override;
    void render() override;
    void applyTheme() override;
};