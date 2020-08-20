#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <string>
#include <vector>

#include "extension.h"
#include "view.h"

struct statusbar_t : public view_t {

    statusbar_t();

    static statusbar_t* instance();

    std::string status;
    std::map<int, std::string> text;
    std::map<int, int> sizes;

    void setText(std::string text, int pos = 0, int size = 12);
    void setStatus(std::string status, int frames = 2000);

    void update(int delta) override;
    void render() override;
    void applyTheme() override;
    bool isVisible() override;
    void preLayout() override;

    int frames; // x mseconds from last kbhit (corresponds to kbhit timeout)
    std::string prevStatus;
};

#endif // STATUSBAR_H
