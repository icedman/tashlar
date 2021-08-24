#ifndef ICONS_H
#define ICONS_H

#include "extension.h"

struct icons_t {

    icons_t();
    ~icons_t();

    std::string iconForFile(std::string file);
    std::string iconForFolder(std::string folder);

    icon_theme_ptr icons;
};

#endif // ICONS_H
