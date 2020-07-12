#ifndef EXPLORER_H
#define EXPLORER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"
#include "window.h"

struct filelist_t {

    filelist_t();
    filelist_t(std::string path);

    std::string name;
    std::string path;
    std::vector<std::shared_ptr<struct filelist_t>> files;

    bool expanded;

    bool isDirectory;
    bool canLoadMore;
    int depth;

    void setPath(std::string path);
    void load(std::string path);
};

struct explorer_t : public window_t {

    explorer_t()
        : window_t(true)
        , colorPair(0)
    {
        focusable = true;
    }

    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void renderLine(const char* line);

    theme_ptr theme;
    int colorPair;

    filelist_t files;

    std::vector<struct filelist_t*> renderList;
};

void renderExplorer(struct explorer_t& explorer);

#endif // EXPLORER_H