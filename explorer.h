#ifndef EXPLORER_H
#define EXPLORER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"

struct filelist_t {

    filelist_t();
    filelist_t(std::string path);
    
    std::string name;
    std::string path;
    std::vector<std::shared_ptr<struct filelist_t>> files;

    bool isDirectory;
    bool canLoadMore;
    int depth;

    void setPath(std::string path);
    void load(std::string path);
};

struct explorer_t {

    explorer_t()
        : win(0)
        , colorPair(0) {
    }

    void render();
    void renderLine(const char* line);
    
    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;

    WINDOW* win;

    theme_ptr theme;
    int colorPair;

    filelist_t files;

    std::vector<struct filelist_t*> renderList;
};

void renderExplorer(struct explorer_t& explorer);
    
#endif // EXPLORER_H