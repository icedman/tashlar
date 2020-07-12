#ifndef EXPLORER_H
#define EXPLORER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "theme.h"
#include "window.h"

struct fileitem_t {

    fileitem_t();
    fileitem_t(std::string path);

    std::string name;
    std::string path;
    std::string fullPath;
    std::vector<std::shared_ptr<struct fileitem_t>> files;

    bool expanded;

    bool isDirectory;
    bool canLoadMore;

    int depth;
    int lineNumber;

    void setPath(std::string path);
    void load(std::string path = "");
};

struct explorer_t : public window_t {

    explorer_t()
        : window_t(true)
        , colorPair(0)
        , currentItem(-1)
    {
        focusable = true;
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void renderLine(const char* line);
    void setRootFromFile(std::string path);

    theme_ptr theme;
    int colorPair;

    fileitem_t files;
    std::vector<struct fileitem_t*> renderList;

    int currentItem;
};

void renderExplorer(struct explorer_t& explorer);

#endif // EXPLORER_H